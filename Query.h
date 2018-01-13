#ifndef QUERY_H
#define QUERY_H

#include "Helper.h"
#include "QueryPlan.h"

extern queue<int> free_blocks;

void preprocessing(const vector<string> &tables, vector<string> &words, SchemaManager &schema_manager);
Relation* execSelect(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem);
Relation* execCreate(vector<string> &words, SchemaManager &schema_manager);
Relation* execInsert(vector<string> &words, string &line, SchemaManager &schema_manager, MainMemory &mem);
Relation* execDelete(vector<string> &words, SchemaManager &schema_manager, MainMemory &mem);

#endif

