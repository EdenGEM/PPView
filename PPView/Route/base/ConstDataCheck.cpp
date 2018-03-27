#include <algorithm>
#include <cmath>
#include <string.h>
#include <limits>
#include "MJCommon.h"
#include "RouteConfig.h"
#include "PlaceGroup.h"
#include "TrafficData.h"
#include "TimeIR.h"
#include "ConstDataCheck.h"

bool ConstDataCheck::m_delFarDist = 1;
bool ConstDataCheck::m_delIntensity = 1;
bool ConstDataCheck::m_delOpenClose = 0;
bool ConstDataCheck::m_needLog = 1;
int ConstDataCheck::m_farDistLimit = 40 * 1000;

bool ConstDataCheck::CheckIntensity(const std::tr1::unordered_map<std::string, std::string>& labelMap, const std::string& rcmdIntensity, const std::string& intensityStr, std::string& errStr) {
	if (rcmdIntensity == "") {
		errStr = "RCMDINTENSITY DATA IS NULL";
		return false;
	}
	if (intensityStr == "") {
		errStr = "INTENSITY DATA IS NULL";
		return false;
	}
	if (labelMap.find(rcmdIntensity) == labelMap.end()) {
		errStr = rcmdIntensity + " NO LABEL IN MAP";
		return false;
	}
	std::vector<std::string> intensityRules;
	TimeIR::split(intensityStr, "|", intensityRules);
	if (intensityRules.size() <= 0) {
		return false;
	}
	bool findIntensity = false;
	for (int i = 0 ; i < intensityRules.size(); i ++) {
		std::vector<std::string> items;
		TimeIR::split(intensityRules[i], ":", items);
		if (items.size() != 2) {
			errStr = intensityRules[i] + " ITEMS NOT 2";
			return false;
		}
		if (labelMap.find(items[0]) == labelMap.end()) {
			errStr = items[0] + " NO LABEL IN MAP";
			return false;
		}
		if (items[0] == rcmdIntensity) {
			findIntensity = true;
		}
		std::string desc = items[1];
		TimeIR::split(desc, "_", items);
		if (items.size() != 3) {
			errStr = desc + " ITEMS NOT 3";
			return false;
		}
		int dur = 0;
		if (!ToInt(items[0], dur) || dur <= 0) {
			errStr = items[0];
			return false;
		}
		double price = 0;
		if (!ToDouble(items[1], price)) {
			errStr = items[1];
			return false;
		}
		if (items[2] != "yes" && items[2] != "no") {
			errStr = items[1];
			return false;
		}
	}
	if (!findIntensity) {
		errStr = rcmdIntensity + " NOT FOUND";
		return false;
	}
	return true;
}

bool ConstDataCheck::ToInt(const std::string& str, int& num) {
	num = 0;
	int start = 0;
	if (str[0]  == '-' && str.size() > 2) {
		start = 1;
	}
	for (int i = start; i < str.size(); i ++) {
		if (str[i] >= '0' && str[i] <= '9') {
			num = num * 10 + (str[i] - '0');
		} else {
			return false;
		}
	}
	if (start == 1) {
		num = -num;
	}
	return true;
}


bool ConstDataCheck::ToDouble(const std::string& str, double& num) {
	num = 0;
	int start = 0;
	if (str[0]  == '-' && str.size() > 2) {
		start = 1;
	}
	int hasPoint = 0;
	double pow_1 = 0.1;
	for (int i = start; i < str.size(); i ++) {
		if (hasPoint == 0 && str[i] >= '0' && str[i] <= '9') {
			num = num * 10 + (str[i] - '0');
		} else if (hasPoint == 1 && str[i] >= '0' && str[i] <= '9'){
			num = num + (str[i] - '0') * pow_1;
			pow_1 *= pow_1;
		} else if (str[i] == '.') {
			hasPoint = 1;
		} else {
			return false;
		}
	}
	if (start == 1) {
		num = -num;
	}
	return true;
}

bool ConstDataCheck::CheckOpenClose(const std::string& openCloseStr, std::string& errStr) {
	std::vector<std::string> rules;
	TimeIR::split(openCloseStr, "|", rules);
	if (rules.size() == 0) {
		errStr = "OPENCLOSE IS NULL";
		return false;
//		return true;
	}
	for (int i = 0 ; i < rules.size(); i ++) {
		if (rules[i] == "") {
			errStr = "NULL RULE ITEM";
			return false;
		}
		std::vector<std::string> items;
		GetItemsBracket("<", ">", rules[i], items);
		if (items.size() != 4) {
			errStr = rules[i] + " RULES NOT 4";
			return false;
		}
		if (!CheckDate(items[0])) {
			errStr = items[0];
			return false;
		}
		if (!CheckWeek(items[1])) {
			errStr = items[1];
			return false;
		}
		if (!CheckDay(items[2])) {
			errStr = items[2];
			return false;
		}
		if (!CheckSure(items[3])) {
			errStr = items[3];
			return false;
		}
	}
	return true;
}


bool ConstDataCheck::GetItemsBracket(const std::string& left, const std::string& right, const std::string& rule, std::vector<std::string>& items) {
	std::string::size_type leftIndex = 0;
	std::string::size_type rightIndex = 0;
	int pos = 0;
	leftIndex = rule.find(left, pos);
	rightIndex = rule.find(right, pos);
	while(leftIndex != std::string::npos && rightIndex != std::string::npos && leftIndex < rightIndex) {
		items.push_back(rule.substr(leftIndex + left.length(), rightIndex - leftIndex - right.length() ));
		pos = rightIndex + right.length();
		leftIndex = rule.find(left, pos);
		rightIndex = rule.find(right, pos);
	}
	return true;
}
bool ConstDataCheck::CheckDate(const std::string& dateRuleStr) {
	std::vector<std::string> dateRules;
	if (dateRuleStr == "*") {
		return true;
	}
	TimeIR::split(dateRuleStr, ",", dateRules);
	if (dateRules.size() == 0) {
		return false;
	}
	for (int i = 0 ; i < dateRules.size(); i ++) {
		std::vector<std::string> items;
		TimeIR::split(dateRules[i], "-", items);
		int date = 0;
		int stMonth = 0;
		int stDay = 0;
		int edMonth = 0;
		int edDay = 0;
		if (items.size() > 2 || items.size() <= 0) {
			return false;
		}else if (items.size() == 1) {
			if (!FindLeftNum("月",items[0],stMonth)) {
				return false;
			}
			if (stMonth > 12 || stMonth <= 0) {
				return false;
			}
			FindLeftNum("日",items[0], stDay);
			if (stDay > kMonthDay[stMonth]) {
				return false;
			}
		} else if (items.size() == 2) {
			if (!FindLeftNum("月",items[0],stMonth)) {
				return false;
			}
			if (stMonth > 12 || stMonth <= 0) {
				return false;
			}
			FindLeftNum("日",items[0], stDay);
			if (stDay > kMonthDay[stMonth]) {
				return false;
			}
			if (!FindLeftNum("月",items[1],edMonth)) {
				return false;
			}
			if (edMonth > 12 || edMonth <= 0) {
				return false;
			}
			FindLeftNum("日",items[1], edDay);
			if (edDay > kMonthDay[edMonth]) {
				return false;
			}
			if (stMonth * 100 + stDay > edMonth * 100 + edDay) {
				return false;
			}
		}
	}
	return true;
}
bool ConstDataCheck::CheckWeek(const std::string& weekRuleStr) {
	std::vector<std::string> weekRules;
	if (weekRuleStr == "*") {
		return true;
	}
	TimeIR::split(weekRuleStr, ",", weekRules);
	if (weekRules.size() == 0) {
		return false;
	}
	for (int i = 0 ; i < weekRules.size(); i ++) {
		std::vector<std::string> items;
		TimeIR::split(weekRules[i], "-", items);
		int stDay = 0;
		int edDay = 0;
		if (items.size() > 2 || items.size() <= 0) {
			return false;
		}else if (items.size() == 1) {
			if (!FindRightNum("周",items[0],stDay)) {
				return false;
			}
			if (stDay > 7 || stDay < 1 ) {
				return false;
			}
		} else if (items.size() == 2) {
			if (!FindRightNum("周",items[0],stDay)) {
				return false;
			}
			if (stDay > 7 || stDay < 1 ) {
				return false;
			}
			if (!FindRightNum("周",items[1],edDay)) {
				return false;
			}
			if (edDay > 7 || edDay < 1 ) {
				return false;
			}
			if (stDay > edDay) {
				return false;
			}
		}
	}
	return true;
}
bool ConstDataCheck::CheckDay(const std::string& dayRuleStr) {
	std::vector<std::string> dayRules;
	if (dayRuleStr == "*" || dayRuleStr == "不开门" || dayRuleStr == "全天") {
		return true;
	}
	TimeIR::split(dayRuleStr, ",", dayRules);
	if (dayRules.size() == 0) {
		return false;
	}
	for (int i = 0 ; i < dayRules.size(); i ++) {
		std::vector<std::string> items;
		TimeIR::split(dayRules[i], "-", items);
		int stHour = -1;
		int stMin = -1;
		int edHour = -1;
		int edMin = -1;
		if (items.size() > 2 || items.size() <= 1) {
			return false;
		} else if (items.size() == 2) {
			if (!FindLeftNum(":",items[0], stHour)) {
				return false;
			}
			if (stHour > 24 || stHour < 0) {
				return false;
			}
			if (!FindRightNum(":",items[0], stMin)) {
				return false;
			}
			if (stMin > 59 || stMin < 0) {
				return false;
			}
			if (!FindLeftNum(":",items[1], edHour)) {
				return false;
			}
			if (edHour > 24 || edHour < 0) {
				return false;
			}
			if (!FindRightNum(":",items[1], edMin)) {
				return false;
			}
			if (edMin > 59 || edMin < 0) {
				return false;
			}
		//	std::cerr << "SMZ " <<stHour * 60 + stMin << "   " << edHour * 60 + edMin << std::endl;
			if (stHour * 60 + stMin >= edHour * 60 + edMin) {
				return false;
			}
		}
	}
	return true;
}
bool ConstDataCheck::CheckSure(const std::string& sureStr) {
	if (sureStr != "NULL" && sureStr != "SURE" && sureStr != "NOT_SURE") {
		return false;
	}
	return true;
}

bool ConstDataCheck::FindRightNum(const std::string key, const std::string& str,int &num) {
	num = 0;
	int pos = 0;
	pos = str.find(key, pos);
	if (pos == std::string::npos) {
		return false;
	}
	int i = pos + key.size();
	while(i < str.size() && str[i] >= '0' && str[i] <= '9') {
		num *= 10;
		num += str[i] - '0';
		i ++;
	}
	return true;
}

bool ConstDataCheck::FindLeftNum(const std::string key, const std::string& str,int &num) {
	num = 0;
	int pos = 0;
	pos = str.find(key, pos);
	if (pos == std::string::npos) {
		return false;
	}
	int i = pos - 1;
	int pow10 = 1;
	while (i >= 0 && str[i] >= '0' && str[i] <= '9') {
		num += (str[i] - '0') * pow10;
		pow10 *= 10;
		i --;
	}
	return true;
}

