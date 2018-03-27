#ifndef _DATA_CHECKER_H_
#define _DATA_CHECKER_H_

#include <iostream>
#include "BasePlan.h"

const int kMealTime = 3600;
const int kMaxIntervalTime = 7200;

class DataChecker {
public:
	static int DoCheck(BasePlan* basePlan);
	static bool IsBadTraf(const TrafficItem* trafItem);
	static int GetAvgTraf(BasePlan* basePlan, std::vector<const LYPlace*>& placeList);
private:
	static int GetView(BasePlan* basePlan);
	static int GetTour(BasePlan* basePlan);
	static int SetHotMap(BasePlan* basePlan);
	static int CheckInfo(BasePlan* basePlan);
	static int GetTraffic(BasePlan* basePlan);
	static int GetRestaurant(BasePlan* basePlan);
	static int GetShop(BasePlan* basePlan);
	static int GetAvgTraf(BasePlan* basePlan);
	static int CheckTraf(BasePlan* basePlan, std::tr1::unordered_set<std::string>& idSet);
	static int SortUserPlace(BasePlan* basePlan);
	static int SortAllPlace(BasePlan* basePlan);
	template <typename T>
		static int SortPlaceList(const T& t, BasePlan* basePlan, std::vector<const LYPlace*>& sortList, bool isShopSortWithView);
	static int CalMiss(BasePlan* basePlan);
};
#endif
