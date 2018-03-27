#ifndef _TRAF_ROUTE_H_
#define _TRAF_ROUTE_H_

#include <iostream>
#include <tr1/unordered_set>
#include "BasePlan.h"

class TrafRoute {
public:
	TrafRoute(){
		m_placeNum = 0;
		m_keyNum = 0;
		m_placeIdx = 0;
		m_keyIdx = 0;
		m_dbgDump = true;
		m_lastItem = NULL;
	}

public:
	int DoRoute(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path,bool needLog = true);
private:
	int ExpandNode(BasePlan* basePlan, const LYPlace* place, const DurS* durS, int nodeType, int cost, const std::string& arvID, const std::string& deptID, const std::vector<const OpenClose*> &openCloseList, PathView* path);
	int ExpandBlock(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path);
	int ExpandKeynode(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path);
public:
	int CheckRoute(BasePlan* basePlan, std::vector<PlaceOrder>& orderList);
private:
	std::vector<PlaceOrder> CompleteOrderList(BasePlan* basePlan, std::vector<PlaceOrder>& orderList);
	int AddBlockPoi(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, std::vector<PlaceOrder>& allPois);
private:
	int m_placeNum;
	int m_keyNum;
	int m_placeIdx;
	int m_keyIdx;
	bool m_dbgDump;
	PlanItem* m_lastItem;
};

#endif
