#ifndef _PATH_PERFECT_H_
#define _PATH_PERFECT_H_

#include <iostream>
#include "BasePlan.h"

class PathPerfect {
public:
	static int DoPerfect(BasePlan* basePlan);
	static int RichAttach(BasePlan* basePlan, Json::Value& jViewDayList);
	static int AddTrafShow(BasePlan* basePlan);
private:
	static int overlapTimeWithRestSlot(int timeZone, int stime, int etime, RestaurantTime& restTime, int type);
	static int RealTimeTrafStat(BasePlan* basePlan);
	static int RichAttach(BasePlan* basePlan, PathView& path, std::vector<RestaurantTime>& restTimeList);
	static int RichAttachNodeMark(BasePlan* basePlan, PathView& path, int begIdx, int endIdx, RestaurantTime& restTime);
};
#endif
