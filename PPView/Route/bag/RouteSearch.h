#ifndef _ROUTE_SEARCH_H_
#define _ROUTE_SEARCH_H_

#include <iostream>
#include "BagPlan.h"
#include "MJCommon.h"
#include "MyThreadPool.h"

class DayItem{
public:
	std::string m_date;
	int m_usedDur;
	std::vector<PlaceOrder> m_orderList;
public:
	DayItem(const std::string& date ="", int remainDur = 0, const std::vector<PlaceOrder>& orderList = std::vector<PlaceOrder>()) {
		m_date = date;
		m_usedDur = remainDur;
		m_orderList = orderList;
	}
};

class RouteSearch {
public:
	static int DoSearch(BagPlan* bagPlan, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool);
	static int DoEval(BagPlan* bagPlan, std::vector<PathView*>& routeList);
private:
	static int EvalObj(BasePlan* basePlan, PathView* path);
	static int AdjustTight(BagPlan* bagPlan, PathView* path);
	static int CalUsedDurPerDay(BagPlan *bagPlan, const PathView* path, std::vector<DayItem*>& dayList);
	static bool Compare(const DayItem* lItem, const DayItem* rItem);
	static int SwapTwoDays(std::vector<DayItem*>& dayList, int lDay, int rDay);
	static int Perm(BagPlan* bagPlan, PathView* newPath, std::vector<DayItem*>& dayList, int dayStart, int dayEnd, const std::vector<std::string>& dateList, int len);
	static const int m_maxWalkDay = 3000;
};

#endif
