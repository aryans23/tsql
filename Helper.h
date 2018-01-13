#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <queue>
#include <ctype.h>
#include <cassert>
#include <queue>
#include <algorithm>
#include <cstdio>
#include <stack>
#include <sstream>
#include <utility>
#include "Block.h"
#include "Config.h"
#include "Disk.h"
#include "Field.h"
#include "MainMemory.h"
#include "Relation.h"
#include "Schema.h"
#include "SchemaManager.h"
#include "Tuple.h"

// list of LqpTreeNode types
enum TYPE
{
	SELECT,
	PROJECT, 
	PRODUCT, 
	JOIN,
	THETA, 
	DISTINCT, 
	SORT, 
	LEAF, 
	HEAD, 
	DELETE
}; 

struct LqpTreeNode
{
	TYPE type;
	vector<string> param;
	Relation* view;
	vector<LqpTreeNode*> children;
	int level;

	LqpTreeNode(TYPE t, vector<string> p, int l): type(t), param(p), view(NULL), level(l) {}
	LqpTreeNode(TYPE t): type(t), view(NULL), level(0){}
	bool isChildrenLoaded();
};

// string to_string(int num);

template <class T>
void print(vector<T> V)
{
	for (int i = 0; i < V.size(); i++){
		cout<<V[i]<<" ";
	}
	cout<<endl;
};

// wrapper for the operation evaluation 
struct compare
{
	bool operator()(const Tuple& l, const Tuple& r);
};

class Eval
{
public:
	Eval(const vector<string>& conditions);
	bool evalUnary(const Tuple & tuple);
	bool evalBinary(const Tuple & lt, const Tuple & rt);
	string findVal(string name, const Tuple &T);
private:
	vector<string> m_conditions;
	bool evalCond(const Tuple &left, const Tuple &right, bool isUnary);
	int opType(string x);
	string evalField(string name, const vector<Tuple> &tuples);
};

extern queue<int> free_blocks;	// free blocks in memory
extern map<TYPE, string> T;		// map of type and its name
extern int _g_relation_counter;	// counter

void initMapT();			// global utiltity functions
string strip(string &s);
vector<int> getNeededFields(const Schema & old, const vector<string>& conditions);
string encodeFields(const Tuple& tuple, const vector<int> & indices);
vector<string> split(string s, string separator);
void resetMemory();
void appendTupleToRelation(Relation* p_relation, MainMemory& memory, Tuple& tuple);
vector<string> getAttributeNames(Relation* p_relation);
bool compareTuples(const Tuple& l, const Tuple& r);

#endif
