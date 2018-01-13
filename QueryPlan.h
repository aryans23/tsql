#ifndef QUERYPLAN_H
#define QUERYPLAN_H

/*
	Logical Query Planner
*/
class LqpTreeNode;
Relation* getLQPForDelete(vector<string> where_list, 
	string relation_name, SchemaManager & schema_manager, MainMemory & mem);
Relation* getLQP(bool has_distinct, vector<string> select_list, 
	vector<string> from_list, vector<string> where_list, 
	vector<string> order_list, SchemaManager &schema_manager, MainMemory &mem);
void printLQP(LqpTreeNode* head);
void optimizeLQP(LqpTreeNode *head);
void traversePreorder(LqpTreeNode *N, map<string, vector<string> > &select_opt);
void convertLQPToPostfix(LqpTreeNode *N);
vector<string> infixToPostfix(vector<string> infix);
int getOperatorPrecedence(string op);

/*
	Physical Query Planner
*/
extern queue<int> free_blocks;
void getDPQP(LqpTreeNode * head, SchemaManager& schema_manager, MainMemory& mem);
void getPQP(LqpTreeNode * head, SchemaManager& schema_manager, MainMemory& mem);
void traversePostOrderReverse(LqpTreeNode * N, SchemaManager& schema_manager, MainMemory& mem);
void getTablePtr(LqpTreeNode * N, SchemaManager& schema_manager);
Relation * unaryRW(LqpTreeNode* N, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);
Relation * binaryRW(LqpTreeNode* left_node, LqpTreeNode* right_node, const vector<string> & conditions, MainMemory& mem, SchemaManager& schema_mgr, TYPE opType, int level);

#endif