#include "Query.h"

void preprocessing(const vector<string> &relations, vector<string> &words, SchemaManager &schema_manager)
{
	for (int i = 0; i < words.size(); i++) {
		bool is_column = false;
		// has no "."
		for (int j = 0; j < relations.size(); j++){
			if (words[i].find('.') == string::npos){
				if (schema_manager.getSchema(relations[j]).fieldNameExists(words[i])){
					words[i] = relations[j] + "." + words[i];
					is_column = true;
					break;
				}
			}
			// term or value
			if (!is_column){
				string valid_word;
				string::iterator iter = words[i].begin();

				// removing spaces
				while(iter != words[i].end() && *iter == ' ') 
					words[i].erase(iter++);
				reverse(words[i].begin(), words[i].end());
				
				iter = words[i].begin();
				while(iter != words[i].end() && *iter == ' ') 
					words[i].erase(iter++);
				reverse(words[i].begin(), words[i].end());
				
				for (int k = 0; k < words[i].size(); k++)
					if (words[i][k] != '"')
						valid_word.push_back(words[i][k]);

				words[i] = valid_word;
			}
		}
	}
}

Relation* execSelect(vector<string> &words, SchemaManager &schema_manager, MainMemory &memory)
{
	bool has_distinct = false, has_where = false, has_orderby = false;
	vector<string> select_items, from_items, where_items, order_items;
	
	int i = 1;
	if (words[i] == "DISTINCT"){
		has_distinct = true;
		i++;
	}
	while (i < words.size() && words[i] != "FROM") {	// drop comma
		select_items.push_back(split(words[i], ",")[0]);
		i++;
	}
	i++; 												// skip FROM
	while ( i < words.size() && words[i] != "WHERE" && words[i] != "ORDER") {
		from_items.push_back(split(words[i], ",")[0]);
		i++;
	}
	if (i < words.size()) {
		if (words[i] == "WHERE") {
			has_where = true;
			i++; 										// skip WHERE
			while (i < words.size() && words[i] != "ORDER") {
				where_items.push_back(words[i]);
				i++;
			}
		}
		if (i < words.size() && words[i] == "ORDER") {
			has_orderby = true;
			i = i + 2; 									// skip ORDER BY
			order_items.push_back(words[i]);
			i++;
		}
	}

	// add table name to each column name
	preprocessing(from_items, select_items, schema_manager);
	preprocessing(from_items, where_items, schema_manager);
	preprocessing(from_items, order_items, schema_manager);
	
	Relation* lqp_relation =  getLQP(has_distinct, select_items, from_items, where_items, order_items, schema_manager, memory);
	cout << *lqp_relation << endl;
	return lqp_relation;
}

Relation* execCreate(vector<string> &words, SchemaManager &schema_manager)
{
	vector<string> attributes;
	vector<enum FIELD_TYPE> attributes_type;
	string table_name = words[2];

	for (int i = 3; i < words.size(); i += 2) {
		attributes.push_back(strip(words[i]));
		if (strip(words[i+1]) == "INT")
			attributes_type.push_back(INT);
		else
			attributes_type.push_back(STR20);
	}

	Schema schema(attributes, attributes_type); 
	Relation* p_relation = schema_manager.createRelation(table_name, schema);
	cout << *p_relation << endl;
	return p_relation;
}

Relation* execInsert(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &memory)
{
	Relation* p_relation = schema_manager.getRelation(words[2]);
	if (!p_relation) {
		cout << "The entered relation is not found!" << endl;
		return NULL;
	}
	std::vector<string>::iterator iter = find(words.begin(), words.end(), "SELECT");

	// no select
	if (iter == words.end()) {
		// get insert values
		vector<string> content = split(line, "()");
		vector<string> fields = split(content[1], ", ");
		vector<string> values = split(content[3], ",");
		
		preprocessing(vector<string>(1, words[2]), values, schema_manager);
		assert(fields.size() == values.size());
		Tuple tuple = p_relation->createTuple();
		vector<string> attr_names = getAttributeNames(p_relation);

		for (int i = 0; i < fields.size(); i++) {	// comparing 
			for (int j = 0; j < attr_names.size(); j++) {
				if (fields[i] == attr_names[j]) {	// this is a match
					if (tuple.getSchema().getFieldType(j) == INT)
						tuple.setField(j, stoi(values[i]));
					else
						tuple.setField(j, values[i]);
					break;
				}
			}
		}
		appendTupleToRelation(p_relation, memory, tuple);
	} else {	// with SELECT
		vector<string> SelectFromWhere(iter, words.end());	
		Relation* new_table = execSelect(SelectFromWhere, schema_manager, memory);
		assert(new_table);
		vector<string> new_attr_names = getAttributeNames(new_table);
		vector<string> attributes = getAttributeNames(p_relation);
	
		vector<int> mapping(new_attr_names.size(), -1);
		for (int i = 0; i < new_attr_names.size(); i++) {
			for (int j = 0; j < attributes.size(); j++) {
				if (new_attr_names[i] == attributes[j]) {
					mapping[i] = j;
					break;
				}
			}
		}

		vector<Tuple> new_tuples;
		int new_attr_size = new_table->getSchema().getNumOfFields();

		for (int i = 0; i < new_table->getNumOfBlocks(); i++) {
			assert(!free_blocks.empty());
			int memory_block_index = free_blocks.front(); free_blocks.pop();
			
			new_table->getBlock(i, memory_block_index);
			Block* p_block = memory.getBlock(memory_block_index);
			assert(p_block);
			vector<Tuple> block_tuples = p_block->getTuples();
			new_tuples.insert(new_tuples.end(), block_tuples.begin(), block_tuples.end());
			if(new_tuples.empty())
				cerr << "Warning: Insert from Select-From-Where, No tuples in the current memory block!" << endl;
			free_blocks.push(memory_block_index);
		}

		for (int j = 0; j < new_tuples.size(); j++){
			Tuple tup = p_relation->createTuple();
			for (int k = 0; k < new_attr_size; k++){
				if (mapping[k] != -1){
					int idx = mapping[k];
					assert(idx < p_relation->getSchema().getNumOfFields() && idx >= 0);
					if (tup.getSchema().getFieldType(idx) == INT) {
						int val = new_tuples[j].getField(k).integer;
						tup.setField(attributes[idx], val);
					} else {
						string *s = new_tuples[j].getField(k).str;
						tup.setField(attributes[idx], *s);
					}
				}
			}
			appendTupleToRelation(p_relation, memory, tup);
		}
		cout << *p_relation << endl;
	}
	return p_relation;
}

Relation* execDelete(vector<string> &words, SchemaManager &schema_manager, MainMemory &memory) 
{
	Relation* p_relation = schema_manager.getRelation(words[2]);
	vector<string>::iterator iter = find(words.begin(), words.end(), "WHERE");
	
	if (iter == words.end()) {				// no WHERE, delete everything
		p_relation->deleteBlocks(0);	
	} else {								// with WHERE clause
		vector<string> where_items(iter, words.end());
		preprocessing(vector<string> (1, words[2]), where_items, schema_manager);
		Relation *new_table = getLQPForDelete(where_items, words[2], schema_manager, memory);
		schema_manager.deleteRelation(words[2]);
		Relation* new_relation = schema_manager.createRelation(words[2], new_table->getSchema());		
		assert(!free_blocks.empty());
		int memory_block_index = free_blocks.front();
		free_blocks.pop();
		
		int nBlocks = new_table->getNumOfBlocks();
		int size =  0;
		Block * p_block = NULL;

		while(size < nBlocks) {	// read the relation block by block
			new_table->getBlock(size, memory_block_index);
			p_block = memory.getBlock(memory_block_index);
			vector<Tuple> tuples = p_block->getTuples();
			if(tuples.empty()) {
				cerr<<"Warning In Delete: No tuples in the current memory block!"<<endl;
			}
			for(int i = 0; i < tuples.size(); ++i){
				Tuple t = tuples[i];
				appendTupleToRelation(new_relation, memory, t);
			}
			size++;
		}
		free_blocks.push(memory_block_index);
	}
	p_relation = schema_manager.getRelation(words[2]);
	cout << p_relation << endl;
	return p_relation;
}