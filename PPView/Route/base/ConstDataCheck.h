#ifndef _CONST_DATA_CHECK_
#define _CONST_DATA_CHECK_
#include <algorithm>
#include <cmath>
#include <string.h>
#include <limits>
#include "MJCommon.h"
#include "RouteConfig.h"
#include "PlaceGroup.h"
#include "TrafficData.h"
#include "TimeIR.h"

const int kMonthDay[13] = {0,31,29,31,30,31,30,31,31,30,31,30,31};

class ConstDataCheck {
public:
	static bool CheckOpenClose(const std::string& openCloseStr, std::string& errStr);//检验开关门
	static bool CheckDate(const std::string& dateRuleStr);//检验开关门的日期规则
	static bool CheckWeek(const std::string& weekRuleStr);//检验 开关门的周规则
	static bool CheckDay(const std::string& dayRuleStr);//检验天中的时间规则
	static bool CheckSure(const std::string& sureStr);//检验数据的真实字段是否正常
	static bool CheckIntensity(const std::tr1::unordered_map<std::string, std::string>& labelMap,const std::string& rcmdIntensity, const std::string& intensityStr, std::string& errStr);//检验游玩强度是否异常
public:
	// <4月1日-9月30日><周5-周7><10:00-17:00><SURE>
	// 1-1:2700_0.0_yes|2-1:5400_10.0_no|3-42:10800_14.5_no
	static bool GetItemsBracket(const std::string& left, const std::string& right, const std::string& rule, std::vector<std::string>& items);
	static bool FindRightNum(const std::string key, const std::string& str,int &num);
	static bool FindLeftNum(const std::string key, const std::string& str,int &num);
	static bool ToInt(const std::string& str, int& num);
	static bool ToDouble(const std::string& str, double& num);
public:
	static bool m_delFarDist;
	static bool m_delIntensity;
	static bool m_delOpenClose;
	static bool m_needLog;
	static int m_farDistLimit;
};
#endif
