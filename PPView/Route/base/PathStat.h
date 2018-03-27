#ifndef _PATH_STAT_H_
#define _PATH_STAT_H_

// 统计路线的各种指标

#include <iostream>
#include "BasePlan.h"

class PathStat {
public:
	static int GetViewNum(Json::Value& jDayList);
	static int GetRestNum(Json::Value& jDayList);
	static int GetActNum(Json::Value& jDayList);
	static int GetDayNum(Json::Value& jDayList);
	static double GetTotPrice(Json::Value& jDayList);
	static int GetCloseNum(Json::Value& jDayList);
	static int GetTotDist(Json::Value& jDayList);
	static int GetRest(BasePlan* basePlan, Json::Value& jDayList);
	static int GetRestDay(BasePlan* basePlan, const std::string& date, Json::Value& jViewList);
	static double GetIntensity(BasePlan* basePlan, Json::Value& jDayList);
	static int GetIntensityLabel(BasePlan* basePlan, Json::Value& jDayList);
	static int GetCrossCnt(BasePlan* basePlan, Json::Value& jDayList);
	
	static int GetTrafDay(Json::Value& jViewList);
	static int GetPlaceDurDay(Json::Value& jViewList);
	static int GetMiddleDurDay(Json::Value& jViewList);
	static int GetRcmdDurDay(BasePlan* basePlan, Json::Value& jViewList);
	static double GetTrafPer(Json::Value& jDayList);
	static double GetPlayPer(Json::Value& jDayList);
};
#endif

