#include "Helper.h"

queue<int> free_blocks;
map<TYPE, string> T;
int _g_relation_counter;

// initialize T map
void initMapT()
{
	T[SELECT] = "SELECT";
	T[PROJECT] = "PROJECT";
	T[PRODUCT] = "PRODUCT";
	T[JOIN] = "JOIN";
	T[THETA] = "THETA";
	T[DISTINCT] = "DISTINCT";
	T[SORT] = "SORT";
	T[LEAF] = "LEAF";
	T[DELETE] = "DELETE";
};

bool LqpTreeNode::isChildrenLoaded()
{
	for(int i = 0; i < children.size(); ++i){
		assert(children[i]);
		if(!(children[i]->view))  return false;
	}
	return true;
}

/*
string to_string(int num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}
*/

bool compare::operator()(const Tuple& leftTup, const Tuple& rightTup)
{
	if(leftTup.getNumOfFields() != rightTup.getNumOfFields()) return false;
	string s1, s2;
	for(int i = 0; i < leftTup.getNumOfFields(); i++){
		union Field f1 = leftTup.getField(i);
		union Field f2 = rightTup.getField(i);

		if(leftTup.getSchema().getFieldType(i) != rightTup.getSchema().getFieldType(i)){
			cerr<<"Fatal error! tuples with different schema!!"<<endl;
			exit(EXIT_FAILURE);
		}
		if(leftTup.getSchema().getFieldType(i) == 0){
			s1 += to_string(f1.integer);
			s2 += to_string(f2.integer);
		}
		else{
			s1 += *f1.str;
			s2 += *f2.str;
		}
	}
	return s1 < s2;
}

vector<int> getNeededFields(const Schema & old, const vector<string>& conditions)
{
	vector<int> indices;
	for(int i = 0; i < conditions.size(); ++i) {
		if (old.fieldNameExists(conditions[i])) {
			indices.push_back(old.getFieldOffset(conditions[i]));
		} else {
			vector<string> temp = split(conditions[i], ".");
			if (temp.size() == 2 && old.fieldNameExists(temp[1])){
				int ind = old.getFieldOffset(temp[1]);
				indices.push_back(ind);
			} else {
				cerr << "Cannot find the field name: " << conditions[i] << "!!" << endl;
				exit(EXIT_FAILURE);
			}
		}
	}
	return indices;
}

// cat(encode) the field of a given tuple into a string
string encodeFields(const Tuple& tuple, const vector<int> & indices)
{
	if(tuple.getNumOfFields() < indices.size()){
		cerr << "Function encodeFields. Too many indices given!" << endl;
		exit(EXIT_FAILURE);
	}
	string key;
	for(int i = 0; i < indices.size(); ++i){
		assert(indices[i] < tuple.getNumOfFields());
		union Field f = tuple.getField(indices[i]);
		if(tuple.getSchema().getFieldType(indices[i]) == 0)  
			key += to_string(f.integer);
		else 
			key += *f.str;
	}
	return key;
}

// AN example procedure of appending a tuple to the end of a relation
// using memory block "memory_block_index" as output buffer
void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, Tuple& tuple) 
{
	assert(!free_blocks.empty());
	// get a available mem block
	int memory_block_index = free_blocks.front();
	free_blocks.pop();
	Block* block_ptr;
	if (relation_ptr->getNumOfBlocks()==0) {
		//cout << "The relation is empty" << endl;
		//cout << "Get the handle to the memory block " << memory_block_index << " and clear it" << endl;
		block_ptr=mem.getBlock(memory_block_index);
		block_ptr->clear(); //clear the block
		block_ptr->appendTuple(tuple); // append the tuple
		//cout << "Write to the first block of the relation" << endl;
		relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index);
	} else {
		//cout << "Read the last block of the relation into memory block:" << endl;
		relation_ptr->getBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index);
		block_ptr=mem.getBlock(memory_block_index);

		if (block_ptr->isFull()) {
			//	cout << "(The block is full: Clear the memory block and append the tuple)" << endl;
			block_ptr->clear(); //clear the block
			block_ptr->appendTuple(tuple); // append the tuple
			//	cout << "Write to a new block at the end of the relation" << endl;
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index); //write back to the relation
		} else {
			//	cout << "(The block is not full: Append it directly)" << endl;
			block_ptr->appendTuple(tuple); // append the tuple
			//	cout << "Write to the last block of the relation" << endl;
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index); //write back to the relation
		}
	}  
	free_blocks.push(memory_block_index);
};

// strip table name from field name of 
vector<string> getAttributeNames(Relation* p_relation)
{
	vector<string> field_names = p_relation->getSchema().getFieldNames();
	for (int i = 0; i < field_names.size(); i++) {	
		vector<string> names = split(field_names[i], ".");
		if (names.size() == 2)
			field_names[i] = names[1];
	}
	return field_names;
}

bool compareTuples(const Tuple& leftTup, const Tuple& rightTup)
{
	if(leftTup.getNumOfFields() != rightTup.getNumOfFields()) return false;
	string s1, s2;
	for(int i = 0; i < leftTup.getNumOfFields(); i++){
		union Field f1 = leftTup.getField(i);
		union Field f2 = rightTup.getField(i);

		if(leftTup.getSchema().getFieldType(i) != rightTup.getSchema().getFieldType(i)){
			cerr<<"Fatal error! tuples with different schema!!"<<endl;
			exit(EXIT_FAILURE);
		}
		if(leftTup.getSchema().getFieldType(i) == 0){
			s1 += to_string(f1.integer);
			s2 += to_string(f2.integer);
		}
		else{
			s1 += *f1.str;
			s2 += *f2.str;
		}
	}
	return s1 == s2;

}



Eval::Eval(const vector<string> & conditions): m_conditions(conditions){
}

bool Eval::evalUnary(const Tuple & tuple){
	//  only select is called here!
	return evalCond(tuple,  tuple, true);
}

bool Eval::evalBinary(const Tuple & lt, const Tuple & rt){
	// only theta should be here!
	return evalCond(lt, rt, false);
}


// evaluate the value of a postfix clause
bool Eval::evalCond(const Tuple &left, const Tuple &right, bool isUnary){
	if (m_conditions.empty()) return true;
	stack<string> stk;
	const string True = "_#true#_", False = "_#false#_";
	for (int i = 0; i < m_conditions.size(); i++){
		int type = opType(m_conditions[i]);
		// is column name 
		if (type == 0){
			string val;
			vector<Tuple> tuples;
			if (isUnary){
				tuples.push_back(left);
			} 
			else{
				tuples.push_back(left);
				tuples.push_back(right);
			}
			val = evalField(m_conditions[i], tuples);
			stk.push(val);
		}
		// is constant
		else if (type == -1){
			stk.push(m_conditions[i]);
		}
		// is operator
		else{
			string op2 = stk.top();
			stk.pop();
			string op1 = stk.top();
			stk.pop();
			// < >
			if (type == 4){
				int num1 = atoi(op1.c_str());
				int num2 = atoi(op2.c_str());
				bool ans = false;
				if (m_conditions[i] == "<"){
					ans = (num1 < num2);
				}
				else{
					ans = (num1 > num2);
				}
				stk.push(ans == true ? True : False);
			}
			// int
			else if (type == 3){
				int num1 = atoi(op1.c_str());
				int num2 = atoi(op2.c_str());
				int ans = 0;
				if (m_conditions[i] == "+"){
					ans = num1 + num2;
				}
				else if (m_conditions[i] == "-"){
					ans = num1 - num2;
				}
				else{
					ans = num1 * num2;
				}
				stringstream ss;
				ss << ans;
				string answer = ss.str(); 
				stk.push(string(answer));
			}
			// bool
			else if (type == 2){
				bool b1 = op1 == True;
				bool b2 = op2 == True;
				if (m_conditions[i] == "AND"){
					stk.push((b1 && b2) == true ? True : False);
				}
				else{
					stk.push((b1 || b2) == true ? True : False);
				}
			}
			// string
			else{
				stk.push(op1 == op2 ? True : False);
			}
		}
	}
	return stk.top() == True;
}

// find operator type
int Eval::opType(string x){
	// int -> bool
	if (x == "<" || x == ">"){
		return 4;
	}
	// int -> int
	if (x == "+" || x == "-" || x == "*")
		return 3;
	// bool -> bool
	else if (x == "AND" || x == "OR")
		return 2;
	// string -> bool
	else if (x == "=")
		return 1;
	// column name
	else if (x.find('.') != string::npos)
		return 0;
	// constant e.g. 5, "A"
	else 
		return -1;
}

string Eval::evalField(string name, const vector<Tuple> &tuples){
	const string null = "_#NULL#_";
	string val = null;

	// first pass, use postfix directly
	for (int i = 0; i < tuples.size(); i++){
		val = findVal(name, tuples[i]);
		if (val != null) return val;
	}

	assert(split(name, ".").size() == 2);

	// second pass, use only the field name
	string field_name = split(name, ".")[1];
	for (int i = 0; i < tuples.size(); i++){
		val = findVal(field_name, tuples[i]);
		if (val != null) return val;
	}

	cerr<<"No field name matches found for "<< name<<endl;
	exit(EXIT_FAILURE);
	return name;
}

string Eval::findVal(string name, const Tuple &T){
	int size = T.getNumOfFields();
	for (int i = 0; i < size; i++){
		if (T.getSchema().fieldNameExists(name)){
			union Field F = T.getField(name);
			if (T.getSchema().getFieldType(name) == INT){
				stringstream ss;
				ss << F.integer;
				return ss.str();
			}
			else{
				return *(F.str);
			}
		}
	}
	return "_#NULL#_";
}

void resetMemory()
{
	while (!free_blocks.empty()){
		free_blocks.pop();
	}
	for (int i = 0; i < NUM_OF_BLOCKS_IN_MEMORY; i++){
		free_blocks.push(i);
	}
};

vector<string> split(string s, string separator) 
{
	int prev_pos = 0, idx;
	vector<string> words;
	words.clear();
	while ((idx = s.find_first_of(separator, prev_pos)) != string::npos) {
		if (idx > prev_pos) {
			words.push_back(s.substr(prev_pos, idx-prev_pos));
		}
		prev_pos = idx+1;
	}
	if (prev_pos < s.length()) {
		words.push_back(s.substr(prev_pos, string::npos));
	}
	return words;
};

string strip(string &s)
{
	string trimmed_string;
	for (int i = 0; i < s.size(); i++){
		if (isalpha(s[i]) || isdigit(s[i]))
			trimmed_string.push_back(s[i]);
	}
	return trimmed_string;
};

