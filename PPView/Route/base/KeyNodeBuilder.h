#ifndef _KEY_NODE_BUILDER_H_
#define _KEY_NODE_BUILDER_H_

#include <iostream>
#include "BasePlan.h"

class KeyNodeBuilder {
public:
	static int BuildKeyNode(Json::Value& req, BasePlan* basePlan);
private:
	static int BuildKeyNodeAll(Json::Value& req, BasePlan* basePlan);

	static int AddKeyNode(Json::Value& req, BasePlan* basePlan);
	static int MergeKeyNode(Json::Value& req, BasePlan* basePlan);
	static int MergeKeyNode(KeyNode* cur_node, KeyNode* next_node, bool keep_cur);
	static int MergeKeyNodeFaultToLerant(BasePlan* basePlan);
	static int FixTrafDate(BasePlan* basePlan, std::vector<KeyNode*>& sleepKeyList, KeyNode* reclaimLuggageKey, KeyNode* arvKey, KeyNode* deptKey);
	static int CheckNotConfictPerCity(BasePlan* basePlan);
	static int ChangeContinueForDayOneLast3HNotPlan(Json::Value& req, BasePlan* basePlan);
	static int NotPlanBetweenSomeKeynodeByDate(Json::Value& req, BasePlan* basePlan);
};

class BlockBuilder {
public:
	static int BuildBlock(Json::Value& req, BasePlan* basePlan);
};
#endif
