#include "Helper.h"
#include "QueryPlan.h"
#include "Algos.h"
#include <algorithm>
#include <stack>

/*
	Logical Query Planner
*/

// do this only for delete!
Relation* getLQPForDelete(vector<string> where_list, 
	string relation_name, SchemaManager & schema_manager, MainMemory & memory)
{
	LqpTreeNode *head = new LqpTreeNode(DELETE, where_list, 0);
	LqpTreeNode *node = new LqpTreeNode(LEAF, vector<string>(1, relation_name), 1);
	assert(head && node);
	head->children.push_back(node);

  	// change conditions to post fix expression
	convertLQPToPostfix(head);

	getDPQP(head, schema_manager, memory);

	assert(head->view);
	return head->view;
}

Relation* getLQP(bool has_distinct, vector<string> select_list, 
	vector<string> from_list, vector<string> where_list, 
	vector<string> order_list, SchemaManager &schema_manager, MainMemory &memory)
{
	initMapT();
	
	int level = 0;
	bool has_where = !where_list.empty();
	bool has_order = !order_list.empty();
	// construct lqp
	LqpTreeNode *dummy = new LqpTreeNode(HEAD, vector<string> (), -1);
	LqpTreeNode *N = dummy, *child;
	// eliminate dup
	if (has_distinct){
		child = new LqpTreeNode(DISTINCT);
		child->level = level++;
		N->children.push_back(child);
		N = child;
	}
	// project 

	child = new LqpTreeNode(PROJECT, select_list, level++);
	child->level = level++;
	N->children.push_back(child);
	N = child;

	// sort 
	if (has_order){
		child = new LqpTreeNode(SORT, order_list, level++);
		N->children.push_back(child);
		N = child;
	}

	// select
	child = new LqpTreeNode(SELECT, where_list, level++);
	N->children.push_back(child);
	N = child;
	// product
	int Size = from_list.size();
	int idx = 0;
	while (Size > 1){
		child = new LqpTreeNode(PRODUCT);
		child->level = level++;
		N->children.push_back(child);
		//LqpTreeNode *product = N;
		N = child;
		child = new LqpTreeNode(LEAF, vector<string> (1, from_list[idx]), level++);
		N->children.push_back(child);
		Size--; idx++;
	}
	// last child
	child = new LqpTreeNode(LEAF, vector<string> (1, from_list[idx]), level++);
	N->children.push_back(child);

	assert(dummy && dummy->children.size() == 1);
	LqpTreeNode* head = dummy->children[0];
	assert(head);

	optimizeLQP(head);
	convertLQPToPostfix(head);
	getPQP(head, schema_manager, memory);
	return head->view;
}

void convertLQPToPostfix(LqpTreeNode *N)
{
	N->param = infixToPostfix(N->param);
	for (int i = 0; i < N->children.size(); i++){
		convertLQPToPostfix(N->children[i]);
	}
}


// Convert the infix notation to postfix
// Algorithm:
// 1. Scan the infix expression from left to right.
// 2. If the scanned character is an operand, output it.
// 3. Else,
// ....3.1 If the precedence of the scanned operator is greater than the 
// 		   precedence of the operator in the stack(or the stack is empty), push it.
// ....3.2 Else, Pop the operator from the stack until the precedence of the scanned 
// 		   operator is less-equal to the precedence of the operator residing on the 
// 		   top of the stack. Push the scanned operator to the stack.
// 4. If the scanned character is an ‘(‘, push it to the stack
// 5. If the scanned character is an ‘)’, pop and output from the stack until an ‘(‘ is encountered.
// 6. Repeat steps 2-6 until infix expression is scanned.
// 7. Pop and output from the stack until it is not empty.
vector<string> infixToPostfix(vector<string> infix)
{
	// seperate ()
	for (int i = 0; i < infix.size(); i++){
		// skip single char
		if (infix[i].size() == 1) continue;
		// left (
		if (infix[i][0] == '('){
			infix[i] = infix[i].substr(1);
			infix.insert(infix.begin() + i, "(");
		}
		// right )
		if (infix[i][infix[i].size()-1] == ')'){
			infix[i] = infix[i].substr(0, infix[i].size()-1);
			infix.insert(infix.begin() + i + 1, ")");
			i--;
		}
	}
	stack<string> stk;
	vector<string> postfix;
	for (int i = 0; i < infix.size(); i++){
		// is operands
		if (getOperatorPrecedence(infix[i]) == 0){
			postfix.push_back(infix[i]);
		}
		// is operator
		else{
			if (stk.empty() || infix[i] == "("){
				stk.push(infix[i]);
			}
			else if (infix[i] == ")"){
				while (stk.top() != "("){
					postfix.push_back(stk.top());
					stk.pop();
				}
				stk.pop();// pop (
			}
			else if (getOperatorPrecedence(stk.top()) < getOperatorPrecedence(infix[i])){
				stk.push(infix[i]);
			}
			else if (getOperatorPrecedence(stk.top()) >= getOperatorPrecedence(infix[i])){
				while (!stk.empty() && getOperatorPrecedence(stk.top()) >= getOperatorPrecedence(infix[i])){
					postfix.push_back(stk.top());
					stk.pop();
				}
				stk.push(infix[i]);
			}
		}
	}
	while (!stk.empty()){
		postfix.push_back(stk.top());
		stk.pop();
	}
	return postfix;
}

int getOperatorPrecedence(string op)
{
	if (op == "(" || op == ")")
		return -1;
	else if (op == "OR")
		return 1;
	else if (op == "AND")
		return 2;
	else if (op == "<" || op == ">" || op == "=" || op == "+" || op == "-" || op == "*")
		return 3;
	else 
		return 0;
}

// apply optimization to logic plan
void optimizeLQP(LqpTreeNode *head)
{
	map<string, vector<string> > destParam;	// <destination, params>
	traversePreorder(head, destParam);
}

void traversePreorder(LqpTreeNode *N, map<string, vector<string> > &destParam)
{
	// if both distinct and sort exist, set the order of distinct to be the same as sort!
	if (N->type == SORT){
		destParam["DISTINCT"].push_back(N->param[0]);	
	}
	else if (N->type == SELECT){
		vector<string> params = N->param;
		vector<string>::iterator it = find(params.begin(), params.end(), "OR");
		// no OR statement, safe to push down selection
		if (it == params.end()){
			string dest = "LEAF";
			vector<string> clause;

			for (int i = 0; i < params.size(); i++) {
				if (params[i] != "AND") {
					// collect one clause
					clause.push_back(params[i]);

					// find destination of this clause
					vector<string> decomp = split(params[i], ".()");
					// relation name is given
					if (decomp.size() == 2){
						if (dest == "LEAF"){
							// remember this relation name
							dest = decomp[0];
						}
						else{
							// if different, can only pushed down to product
							if (decomp[0] != dest){
								dest = "PRODUCT";
							}
						}
					}
				}
				// end of one clause, push to dest
				else {
					if (destParam.count(dest) != 0) destParam[dest].push_back("AND");
					destParam[dest].insert(destParam[dest].end(), clause.begin(), clause.end());
					clause.clear();
					dest = "LEAF";
				}
			}	
			// push the last one to dest
			if (destParam.count(dest) != 0) destParam[dest].push_back("AND");
			destParam[dest].insert(destParam[dest].end(), clause.begin(), clause.end());
		}
		// has OR, push all to product
		else{
			vector<string> clause;
			for (int i = 0; i < params.size(); i++){
				clause.push_back(params[i]);
			}	
			destParam["PRODUCT"].insert(destParam["PRODUCT"].end(), clause.begin(), clause.end());
		}
		// clear this select node
		N->param.clear();
	}
	else if (N->type == PRODUCT){
		// decide which condition to use for this PRODUCT node
		if (destParam.find("PRODUCT") != destParam.end()){
			vector<string> to_pro = destParam["PRODUCT"];
			// has OR
			if (find(to_pro.begin(), to_pro.end(), "OR") != to_pro.end()){
				N->param.insert(N->param.end(), to_pro.begin(), to_pro.end());
				destParam["PRODUCT"].clear();
			}
			// only has AND
			else{
				vector<string> remain;
				int start_pos = 0;
				int i = 0;
				while (i < to_pro.size()){
					vector<string> clause;
					bool match_relation = false;
					// colect a clause
					while (i < to_pro.size() && to_pro[i] != "AND"){
						clause.push_back(to_pro[i]);
						string relation_name = split(to_pro[i], ".")[0];
						// check if clause has the children's relation name 
						for (int j = 0; j < N->children.size(); j++){
							if (N->children[j]->type == LEAF){
								if (N->children[j]->param[0] == relation_name){
									match_relation = true;
								}
							}
						}
						i++;
					}
					// if this condition matches the children column name, leave it here
					if (match_relation){
						if (N->param.size() != 0) N->param.push_back("AND");
						N->param.insert(N->param.end(), clause.begin(), clause.end());
					}
					// otherwise, leave it in remain(destParam)
					else{
						if (remain.size() != 0) remain.push_back("AND");
						remain.insert(remain.end(), clause.begin(), clause.end());
					}
					i++;
				}
				destParam["PRODUCT"] = remain;
			}
		}
	}
	else if (N->type == LEAF){
		string relation_name = N->param[0];
		if (destParam.find(relation_name) != destParam.end()){
			// make a copy of N as L, set L as N's child
			LqpTreeNode *L = new LqpTreeNode(N->type, N->param, N->level+1);
			N->children.push_back(L);
			// change N into a select node
			N->type = SELECT;
			N->param = destParam[relation_name];
			N = L;
		}

	}

	for (int i = 0; i < N->children.size(); i++){
		traversePreorder(N->children[i], destParam);
	}

	// handle exceptions 1: no product node
	if (N->type == SELECT && !destParam["PRODUCT"].empty()){
		N->param.insert(N->param.end(), destParam["PRODUCT"].begin(), destParam["PRODUCT"].end());
	}
	// handle exceptions 2: both distinct and sort exist, set the order of distinct to be the same as sort!
	if (N->type == DISTINCT && !destParam["DISTINCT"].empty()){
		N->param.insert(N->param.end(), destParam["DISTINCT"].begin(), destParam["DISTINCT"].end());
	}
}

void printLQP(LqpTreeNode* head)
{
	cout<<"*****Start of LQP tree ******"<<endl;
	queue<LqpTreeNode*> Q;
	Q.push(head);
	while (!Q.empty()){
		vector<LqpTreeNode*> level;
		while (!Q.empty()){
			level.push_back(Q.front());
			Q.pop();
		}
		for (int i = 0; i < level.size(); i++){
			cout<<T[level[i]->type]<<level[i]->children.size()<<" [ ";
			for (int j = 0; j < level[i]->param.size(); j++){
				cout<<level[i]->param[j];
				if (j != level[i]->param.size()-1) cout<<" ";
			}
			cout<<" ] ";
			for (int k = 0; k < level[i]->children.size(); k++){
				Q.push(level[i]->children[k]);
			}
		}
		cout<<endl;
	}
	cout<<"*****End of LQP tree ******"<<endl;
}

/*
	Physical Query Planner
*/

// do this only for delete! fixed tree structure!!!
void getDPQP(LqpTreeNode * head, SchemaManager& schema_manager, MainMemory& memory)
{
    if(!head) {
		cout << "Physical query plan empty! " << endl;
		return;
	}
	
	assert(head->children.size() == 1);
	getTablePtr(head->children[0], schema_manager);		// load the child
	head->view = unaryRW(head->children[0], head->param, 
		memory, schema_manager, head->type, head->level);
}

// run a reverse-order travseral for the right most lqp
void getPQP(LqpTreeNode * head, SchemaManager& schema_manager, MainMemory& memory){
	if(!head){
		cout<<"The given physical query plan is empty!"<<endl;
		return;
	}
	traversePostOrderReverse(head, schema_manager, memory);
	//cout<<*(head->view)<<endl;
}

void traversePostOrderReverse(LqpTreeNode * N, SchemaManager& schema_manager, MainMemory& memory){
	assert(N);

	for(int i = N->children.size()-1; i >= 0; --i){
		traversePostOrderReverse(N->children[i], schema_manager, memory);
	}

	// handle from the parents of the leaf nodes
	if(N->type == LEAF){
		// assign the relation ptr to the leaf node:
		getTablePtr(N, schema_manager);
		return;
	}

	if(N->isChildrenLoaded()){
		if(N->children.size() == 1){ // unary operation
			// @param1: relation ptr, @param2: conditions for reading the table
			N->view = unaryRW(N->children[0], N->param, memory, schema_manager, N->type, N->level);
		}
		else if(N->children.size() == 2){ // binary operation
			N->view = binaryRW(N->children[0], N->children[1], N->param, memory, schema_manager, N->type, N->level);
		}
		else{
			cerr<<"Only binary/unary operation supported! But you have "<<N->children.size()<<"operands! "<<endl;
			exit(EXIT_FAILURE);
		}
	}
	else{
		cerr<<"You want to do the reverse_post_order but the children's relation ptr is not set!!"<<endl;
		exit(EXIT_FAILURE);
	}
}

// given the table name, load the relation ptr via schema manager
void getTablePtr(LqpTreeNode * N, SchemaManager& schema_mgr){
	assert(N && N->type == LEAF);
	// the leaf node only has one param which is the relation name!
	assert(N->param.size() == 1);

	string relation_name = N->param[0];
	if(N->view != NULL){
		cerr<<"The child has been loaded with the relation "<<relation_name<<endl;
		assert(!(N->view));
	}

	// do not need to do exception handle here since the library has that already.
	N->view = schema_mgr.getRelation(relation_name);
	assert(N->view);
}

// unary READ-WRITE
Relation* unaryRW(LqpTreeNode *N, const vector<string>& conditions, MainMemory& memory,  SchemaManager& schema_mgr, TYPE opType, int level){
	Relation* p_relation = N->view;
	assert(p_relation);

	if(conditions.empty() && T[opType] != "DISTINCT"){
		return p_relation;
	}

	int dBlocks = p_relation->getNumOfBlocks();
	if(dBlocks == 0){
		cerr<<"Warning, when doing the unary operation,  the relation "<<p_relation->getRelationName()<<" is empty!"<<endl;
		return p_relation;
	}
	bool isOnePass = (T[opType] == "SELECT" || T[opType] == "PROJECT" || T[opType] == "DELETE" || dBlocks < memory.getMemorySize()) ? true : false;

	Algorithm alg(isOnePass, conditions, opType, level);
	Relation * newRelation = alg.runUnary(p_relation, memory, schema_mgr, N->type == LEAF);
	assert(newRelation);
	return newRelation;
}

Relation * binaryRW(LqpTreeNode* left_node, LqpTreeNode* right_node, const vector<string> & conditions, MainMemory& memory, SchemaManager& schema_mgr, TYPE opType, int level){
	Relation* left = left_node->view, *right = right_node->view;
	assert(left && right);

	int dBlocks_left = left->getNumOfBlocks();
	int dBlocks_right = right->getNumOfBlocks();

	if(dBlocks_left == 0 || dBlocks_right == 0){
		// either one of them is empty, handle here
		if(dBlocks_left == 0)
			cerr<<"Warning, when doing the binary operation, the relation "<<left->getRelationName()<<" is empty!"<<endl;
		if(dBlocks_right == 0)
			cerr<<"Warning, when doing the binary operation, the relation "<<right->getRelationName()<<" is empty!"<<endl;
		return dBlocks_left == 0 ? left : right; // always return the empty one
	}
	bool isOnePass = (T[opType] == "PRODUCT") ? true : false;

	Algorithm alg(isOnePass, conditions, opType, level);
	Relation * newRelation = alg.runBinary(left, right, memory, schema_mgr, left_node->type == LEAF, right_node->type == LEAF);
	assert(newRelation);
	return newRelation;
}

