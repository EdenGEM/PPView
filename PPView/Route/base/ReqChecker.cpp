#include <iostream>
#include "ReqChecker.h"
#include "ToolFunc.h"

int ReqChecker::DoCheck(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (param.type == "ssv005_rich") {
		ret = CheckSSV005(param, req, errorInfo);
	} else if (param.type == "ssv006_light" || param.type == "s202" || param.type == "s204" || param.type == "s205" || param.type == "s131"
			|| param.type == "s128"	|| param.type == "s125" || param.type == "s129") {
		//高级编辑
		ret = CheckSSV006(param, req, errorInfo);
	} else if (param.type == "s126") {
		ret = CheckS126(param, req, errorInfo);
	} else if (param.type == "p202" || param.type == "p104"
			|| param.type == "p101") {
		ret = CheckPoiList(param, req, errorInfo);
	} else if (param.type == "p105") {
		ret = CheckP105(param, req, errorInfo);
	} else if (param.type == "b116") {
		ret = CheckB116(param, req, errorInfo);
    } else if (param.type == "s203" || param.type == "ssv005_s130"
			|| param.type == "s130") {
		ret = CheckS130(param, req, errorInfo);
	} else if (param.type == "ssv007") {
		ret = CheckSSV007(param, req, errorInfo);
	} else if (param.type == "p201") {
		ret = CheckType(param, req, "cid", JSON_TYPE_STRING, "req.cid", errorInfo);
	}

	return ret;
}

int ReqChecker::CheckSSV005(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;

	if (req.isMember("list")) {
		ret = CheckType(param, req, "list", JSON_TYPE_ARRAY, "list", errorInfo);
		if (ret) return 1;
	}
	ret = CheckCityPrefer(param, req, errorInfo);
	if (ret) return 1;
	{
		for (int j = 0; j < req["list"].size(); j ++) {
			Json::Value& jReq = req["list"][j];
			ret = CheckCity(param, jReq, errorInfo);
			if (ret) return 1;

			if (jReq.isMember("notPlanDays") and !jReq["notPlanDays"].isNull()) {
				ret = CheckType(param, jReq, "notPlanDays", JSON_TYPE_ARRAY, "city-notPlanDays", errorInfo);
				if (ret) return 1;
				ret = CheckNotPlanDays(param, jReq["notPlanDays"], errorInfo);
				if (ret) return 1;
			}

			ret = CheckType(param, jReq, "ridx", JSON_TYPE_INT, "ridx", errorInfo);
			if (ret) return 1;


			if (jReq.isMember("time") and !jReq["time"].isNull()) {
				ret = CheckType(param, jReq, "time", JSON_TYPE_INT, "time", errorInfo);
				if (ret) return ret;
				int timeIntensity = jReq["time"].asInt();
				if (timeIntensity < 0 || timeIntensity > 3) {
					MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckSSV005, time in query range err, time: %d", param.log.c_str(), timeIntensity);
					errorInfo.Set(51101, "请求格式异常: time in query range err");
					return 1;
				}
			}

			if (jReq.isMember("selectPois") and !jReq["selectPois"].isNull()) {
				ret = CheckType(param, jReq, "selectPois", JSON_TYPE_ARRAY, "selectPois", errorInfo);
				if (ret) return ret;
			}

			Json::Value& jPoiList = jReq["selectPois"];
			for (int i = 0; i < jPoiList.size(); ++i) {
				Json::Value& jPoi = jPoiList[i];

				if (jPoi.isMember("mode")) {
					ret = CheckType(param, jPoi, "mode", JSON_TYPE_INT, "mode", errorInfo);
					if (ret) return 1;
				}

				ret = CheckType(param, jPoi, "id", JSON_TYPE_STRING, "id", errorInfo);
				if (ret) return 1;
			}
		}
	}
	ret = CheckType(param, req, "product", JSON_TYPE_OBJECT, "product", errorInfo);
	if (ret) return 1;

	ret = CheckProduct(param, req["product"], errorInfo);
	if (ret) return 1;
	return 0;
}

int ReqChecker::CheckCity(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, req, "city", JSON_TYPE_OBJECT, "city", errorInfo);
	if (ret) return 1;
	Json::Value& jCity = req["city"];
	ret = CheckCityInfo(param, jCity, errorInfo);
	if (ret) return 1;
	return 0;
}

int ReqChecker::CheckCityInfo(const QueryParam& param, Json::Value& jCity, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, jCity, "cid", JSON_TYPE_STRING, "jCity-cid", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity, "arv_time", JSON_TYPE_STRING, "jCity-arv_time", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity, "dept_time", JSON_TYPE_STRING, "jCity-dept_time", errorInfo);
	if (ret) return 1;
	//保证离开时间大于到达时间
	int arvTime = MJ::MyTime::toTime(jCity["arv_time"].asString());
	int deptTime = MJ::MyTime::toTime(jCity["dept_time"].asString());
	if (deptTime < arvTime) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckCityInfo, request error: %s", param.log.c_str(), "arv_time > dept_time");
		errorInfo.Set(51105, "请求格式异常: arv_time > dept_time");
		return 1;
	}
	if (!jCity.isMember("arv_poi")) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckCityInfo, request error: %s", param.log.c_str(), "no member arv_poi");
		errorInfo.Set(51105, "请求格式异常: jCity no arv_poi");
		return 1;
	}
	if (!jCity["arv_poi"].isNull()) {
		ret = CheckType(param, jCity, "arv_poi", JSON_TYPE_STRING, "jCity-arv_poi", errorInfo);
		if (ret) return ret;
		ret = CheckStringFormat(param, jCity["arv_poi"].asString(), STRING_TYPE_ID, "jCity-arv_poi", errorInfo);
		if (ret) return ret;
	}
	if (!jCity.isMember("dept_poi")) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckCityInfo, request error: %s", param.log.c_str(), "city no member dept_poi");
		errorInfo.Set(51105, "请求格式异常: jCity no dept_poi");
		return 1;
	}
	if (!jCity["dept_poi"].isNull()) {
		ret = CheckType(param, jCity, "dept_poi", JSON_TYPE_STRING, "jCity-dept_poi", errorInfo);
		if (ret) return ret;
		ret = CheckStringFormat(param, jCity["dept_poi"].asString(), STRING_TYPE_ID, "jCity-dept_poi", errorInfo);
		if (ret) return 1;
	}
	if (jCity.isMember("hotel") and !jCity["hotel"].isNull()) {
		ret = CheckType(param, jCity, "hotel", JSON_TYPE_ARRAY, "jCity--hotel", errorInfo);
		if (ret) return ret;
	}
	if (jCity.isMember("arvCustom") and !jCity["arvCustom"].isNull()) {
		ret = CheckType(param, jCity, "arvCustom", JSON_TYPE_OBJECT, "jCity--arvCustom", errorInfo);
		if (ret) return ret;
		Json::Value& jArvCustom = jCity["arvCustom"];
		ret = CheckType(param, jArvCustom, "name", JSON_TYPE_STRING, "jCity-jArvCustom-name", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jArvCustom, "lname", JSON_TYPE_STRING, "jCity-jArvCustom-lname", errorInfo);
		if (ret) return 1;
	}
	if (jCity.isMember("deptCustom") and !jCity["custom"].isNull()) {
		ret = CheckType(param, jCity, "deptCustom", JSON_TYPE_OBJECT, "jCity--deptCustom", errorInfo);
		if (ret) return ret;
		Json::Value& jDeptCustom = jCity["deptCustom"];
		ret = CheckType(param, jDeptCustom, "name", JSON_TYPE_STRING, "jCity-jDeptCustom-name", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jDeptCustom, "lname", JSON_TYPE_STRING, "jCity-jDeptCustom-lname", errorInfo);
		if (ret) return 1;
	}
	return 0;
}

int ReqChecker::CheckRoute(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	Json::Value& jCity = req;
	if (!jCity.isMember("view")) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckRoute, request error: view is not a member of city", param.log.c_str());
		errorInfo.Set(51101, "请求格式异常: view is not a member of city");
		return 1;
	}

	//特例 s126 允许view为null
	if (jCity["view"].isNull() && param.type == "s126") return 0;

	ret = CheckType(param, jCity, "view", JSON_TYPE_OBJECT, "city-view", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity["view"], "day",  JSON_TYPE_ARRAY, "city-view-day", errorInfo);
	if (ret) return 1;
	for (int i = 0 ; i < jCity["view"]["day"].size(); ++i) {
		ret = CheckType(param, jCity["view"]["day"][i], "date", JSON_TYPE_STRING, "city-view-day[]-date", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["view"]["day"][i], "stime", JSON_TYPE_STRING, "city-view-day[]-stime", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["view"]["day"][i], "etime", JSON_TYPE_STRING, "city-view-day[]-etime", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["view"]["day"][i], "view", JSON_TYPE_ARRAY, "city-view-day[]-view", errorInfo);
		if (ret) return 1;
		for (int j = 0; j < jCity["view"]["day"][i]["view"].size(); ++j) {
			ret = CheckType(param, jCity["view"]["day"][i]["view"][j], JSON_TYPE_OBJECT, "city-view-day[]-view[]", errorInfo);
			if (ret) return 1;
			Json::Value& jDayView = jCity["view"]["day"][i]["view"][j];
			ret = CheckType(param, jDayView, "id", JSON_TYPE_STRING, "city-view-day[]-view[]-id", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "dur", JSON_TYPE_INT, "city-view-day[]-view[]-dur", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "stime", JSON_TYPE_STRING, "city-view-day[]-view[]-stime", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "etime", JSON_TYPE_STRING, "city-view-day[]-view[]-etime", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "type", JSON_TYPE_INT, "city-view-day[]-view[]-type", errorInfo);
			if (ret) return 1;
		}
	}
	//expire
	ret = CheckType(param, jCity["view"], "expire",  JSON_TYPE_INT, "city-view-expire", errorInfo);
	if (ret) return 1;
	//summary
	ret = CheckType(param, jCity["view"], "summary",  JSON_TYPE_OBJECT, "city-view-summary", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity["view"]["summary"], "days",  JSON_TYPE_ARRAY, "city-view-summary-days", errorInfo);
	if (ret) return 1;
	for (int i = 0; i < jCity["view"]["summary"]["days"].size(); ++i) {
		Json::Value& jDay = jCity["view"]["summary"]["days"][i];
		ret = CheckType(param, jDay, "date",  JSON_TYPE_STRING, "city-view-summary-days[]-date", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jDay, "pois",  JSON_TYPE_ARRAY, "city-view-summary-days[]-pois", errorInfo);
		if (ret) return 1;
		for (int j = 0; j < jDay["pois"].size(); j++) {
			ret = CheckType(param, jDay["pois"][j], "id",  JSON_TYPE_STRING, "city-view-summary-days[]-pois[]-id", errorInfo);
			if (ret) return 1;
		}
	}
	return 0;
}

int ReqChecker::CheckDays(const QueryParam& param, Json::Value& jDays, ErrorInfo& errorInfo) {
	int ret = 0;

	for (int i = 0; i < jDays.size(); i ++) {
		Json::Value& jDay = jDays[i];
		ret = CheckType(param, jDay, "date",  JSON_TYPE_STRING, "day-date", errorInfo);
		if (ret) return 1;
		ret = CheckStringFormat(param, jDay["date"].asString(), STRING_TYPE_TIME_YMD, "date-format", errorInfo);
		if (ret) return 1;

		ret = CheckType(param, jDay, "pois",  JSON_TYPE_ARRAY, "day-pois", errorInfo);
		if (ret) return 1;
		Json::Value& jDayPois = jDay["pois"];
		for (int j = 0; j < jDayPois.size(); j ++) {
			Json::Value& jDayPoi = jDayPois[j];
			ret = CheckType(param, jDayPoi, "id", JSON_TYPE_STRING, "day-poi-id", errorInfo);
			if (ret) return 1;
			ret = CheckStringFormat(param, jDayPoi["id"].asString(), STRING_TYPE_ID, "day-poi-id", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayPoi, "pdur", JSON_TYPE_INT, "day-poi-pdur", errorInfo);
			if (ret) return 1;
			if (jDayPoi.isMember("pstime")) {
				ret = CheckType(param, jDayPoi, "pstime", JSON_TYPE_STRING, "day-poi-pstime", errorInfo);
				if (ret) return 1;
			}
			if (jDayPoi.isMember("custom") and !jDayPoi["custom"].isNull()) {
				ret = CheckType(param, jDayPoi, "custom", JSON_TYPE_INT, "day-poi-custom", errorInfo);
				if (ret) return ret;
				if (jDayPoi["custom"].asInt() == 1) {
					ret = CheckType(param, jDayPoi, "type", JSON_TYPE_INT, "day-poi-custom-type", errorInfo);
					if (ret) return 1;
					ret = CheckType(param, jDayPoi, "name", JSON_TYPE_STRING, "day-poi-custo-namem", errorInfo);
					if (ret) return 1;
					ret = CheckType(param, jDayPoi, "lname", JSON_TYPE_STRING, "day-poi-custom-lname", errorInfo);
					if (ret) return 1;
					ret = CheckType(param, jDayPoi, "coord", JSON_TYPE_STRING, "day-poi-custom-coord", errorInfo);
					if (ret) return 1;
					ret = CheckStringFormat(param, jDayPoi["coord"].asString(), STRING_TYPE_COORD , "day-poi-custom-coord", errorInfo);
					if (ret) return 1;
				}
			}
		}
	}
	return 0;
}

int ReqChecker::CheckNotPlanDays(const QueryParam& param, Json::Value& jDays, ErrorInfo& errorInfo) {
	int ret = 0;
	for (int i = 0; i < jDays.size(); i ++) {
		ret = CheckType(param, jDays, JSON_TYPE_ARRAY, "notPlan-day", errorInfo);
		if (ret) return 1;
		if (jDays[i] != "") {
			ret = CheckStringFormat(param, jDays[i].asString(), STRING_TYPE_TIME_YMD, "notPlan-day-date-format", errorInfo);
		}
		if (ret) return 1;
	}
	return 0;
}

int ReqChecker::CheckSelectPois(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (!req.isMember("selectPois") || req["selectPois"].isNull()) return 0;
	ret = CheckType(param, req, "selectPois", JSON_TYPE_ARRAY, "selectPois", errorInfo);
	if (ret) return ret;
	for (int i = 0; i < req["selectPois"].size(); ++i) {
		Json::Value& jSelPoi = req["selectPois"][i];
		ret = CheckType(param, jSelPoi,"id", JSON_TYPE_STRING, "selectPois[]-id", errorInfo);
		if (ret) return 1;
		//ret = CheckType(param, jSelPoi,"priority",  JSON_TYPE_INT, "selectPois[]-priority", errorInfo);
		//if (ret) return 1;
	}
	return 0;
}

int ReqChecker::CheckCityPrefer(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (!req.isMember("cityPreferCommon") || req["cityPreferCommon"].isNull()) {
		return 0;
	}
	ret = CheckType(param, req["cityPreferCommon"], JSON_TYPE_OBJECT, "cityPreferCommon", errorInfo);
	if (ret) return ret;
	Json::Value& jCityPrefer = req["cityPreferCommon"];
	if (jCityPrefer.isMember("timeRange") and !jCityPrefer["timeRange"].isNull()) {
		ret = CheckType(param, jCityPrefer["timeRange"], JSON_TYPE_OBJECT, "city-prefer-timeRange", errorInfo);
		if (ret) return ret;
		if (jCityPrefer["timeRange"].isMember("from")) {
			ret = CheckType(param, jCityPrefer["timeRange"], "from", JSON_TYPE_STRING, "city-prefer-shopping-timerange-from", errorInfo);
			if (ret) return ret;
		}
		if (jCityPrefer["timeRange"].isMember("to")) {
			ret = CheckType(param, jCityPrefer["timeRange"], "to", JSON_TYPE_STRING, "city-prefer-shopping-timeRange-to", errorInfo);
			if (ret) return ret;
		}
	}

	return 0;
}

int ReqChecker::CheckFilter(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (!req.isMember("filter") || req["filter"].isNull()) return 0;
	ret = CheckType(param, req, "filter", JSON_TYPE_OBJECT, "filter", errorInfo);
	if (ret) return 1;
	Json::Value& jFilter = req["filter"];
	if (jFilter.isMember("sort") and !jFilter["sort"].isNull()) {
		ret = CheckType(param, jFilter, "sort", JSON_TYPE_INT, "filter-sort", errorInfo);
		if (ret) return 1;
	}
	if (jFilter.isMember("private") and !jFilter["private"].isNull()) {
		ret = CheckType(param, jFilter, "private", JSON_TYPE_INT, "filter-private", errorInfo);
		if (ret) return ret;
	}
	if (jFilter.isMember("key") and !jFilter["key"].isNull()) {
		ret = CheckType(param, jFilter, "key", JSON_TYPE_STRING, "filter-key", errorInfo);
		if (ret) return ret;
	}
	if (jFilter.isMember("mode") and !jFilter["mode"].isNull()) {
		ret = CheckType(param, jFilter, "mode", JSON_TYPE_ARRAY, "filter-key", errorInfo);
		if (ret) return ret;
	}
	if (jFilter.isMember("tagIds") and !jFilter["tagIds"].isNull()) {
		ret = CheckType(param, jFilter, "tagIds", JSON_TYPE_ARRAY, "filter-tagIds", errorInfo);
		if (ret) return ret;
		for (int i = 0; i < jFilter["tagIds"].size(); ++i) {
			ret = CheckType(param, jFilter["tagIds"][i], JSON_TYPE_STRING, "filter-tagIds[]", errorInfo);
			if (ret) return 1;
		}
	}
	return 0;
}

// 检查req中成员memName类型正确性，报错errInfo
int ReqChecker::CheckType(const QueryParam& param, Json::Value& req, const std::string& memName, int jType, const std::string& errStr, ErrorInfo& errorInfo) {
	if (!req.isMember(memName)) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckType, request error: %s", param.log.c_str(), errStr.c_str());
		errorInfo.Set(51101, "请求格式异常: " + errStr);
		return 1;
	}

	int ret = CheckType(param, req[memName], jType, errStr, errorInfo);
	if (ret != 0) return 1;

	return 0;
}

int ReqChecker::CheckStringFormat(const QueryParam& param, const std::string& str, int sType, const std::string& errStr, ErrorInfo& errorInfo) {
	int ret = 0;
	if (sType == STRING_TYPE_TIME_YMD) {
		ret = ToolFunc::FormatChecker::CheckDate(str);
	} else if (sType == STRING_TYPE_TIME_YMD_HM) {
		ret = ToolFunc::FormatChecker::CheckTime(str);
	} else if (sType == STRING_TYPE_TIME_HM) {
		ret = ToolFunc::FormatChecker::CheckHM(str);
	} else if (sType == STRING_TYPE_COORD) {
		ret = ToolFunc::FormatChecker::CheckCoord(str);
	} else if (sType == STRING_TYPE_ID) {
		ret = CheckId(str);
	}
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckStringFormat, request error: %s (%s)", param.log.c_str(), errStr.c_str(), str.c_str());
		errorInfo.Set(51101, "请求格式异常: " + errStr);
	}
	return ret;
}

// 检查jValue类型正确性，报错errInfo
int ReqChecker::CheckType(const QueryParam& param, Json::Value& jValue, int jType, const std::string& errStr, ErrorInfo& errorInfo) {
	int ret = 0;
	if (jType == JSON_TYPE_OBJECT) {
		if (!jValue.isObject()) {
			ret = 1;
		}
	} else if (jType == JSON_TYPE_ARRAY) {
		if (!jValue.isArray()) {
			ret = 1;
		}
	} else if (jType == JSON_TYPE_INT) {
		if (!jValue.isInt()) {
			ret = 1;
		}
	} else if (jType == JSON_TYPE_STRING) {
		if (!jValue.isString()) {
			ret = 1;
		}
	} else if (jType == JSON_TYPE_DOUBLE) {
		if (!jValue.isDouble()) {
			ret = 1;
		}
	} else if (jType == JSON_TYPE_NULL) {
		if (!jValue.isNull()) {
			ret = 1;
		}
	}
	if (ret == 1) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckType, request error: %s", param.log.c_str(), errStr.c_str());
		errorInfo.Set(51101, "请求格式异常: " + errStr);
	}
	return ret;
}

int ReqChecker::CheckS130(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckCity(param, req, errorInfo);
	if (ret) return 1;

	ret = CheckType(param, req, "days", JSON_TYPE_ARRAY, "days", errorInfo);
	if (ret) return 1;

	ret = CheckDays(param, req["days"], errorInfo);
	if (ret) return 1;

	ret = CheckType(param, req, "option", JSON_TYPE_OBJECT, "option", errorInfo);
	if (ret) return 1;

	ret = CheckType(param, req["option"], "play_order", JSON_TYPE_INT, "req-option-mode-playOrder", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, req["option"], "play_dur", JSON_TYPE_INT, "req-option-mode-playDur", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, req["option"], "play_time_range", JSON_TYPE_INT, "req-option-mode-playTimeRange", errorInfo);
	if (ret) return 1;

	return 0;
}
int ReqChecker::CheckSSV006(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckCity(param, req, errorInfo);
	if (ret != 0) return 1;

	ret = CheckType(param, req, "ridx", JSON_TYPE_INT, "ridx", errorInfo);
	if (ret != 0) return 1;

	ret = CheckCityPrefer(param, req, errorInfo);
	if (ret) return 1;

	ret = CheckCostomTraffic(param, req, errorInfo);
	if (ret) return 1;

	if ( param.type == "s205"
		||	param.type == "s128") {
		CheckTrafficPass(param, req, errorInfo);
	}else{
		ret = CheckType(param, req, "days", JSON_TYPE_ARRAY, "days", errorInfo);
		if (ret) return 1;
		ret = CheckDays(param, req["days"], errorInfo);
		if (ret) return 1;
	}

	return 0;
}

int ReqChecker::CheckP105(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, req, "list", JSON_TYPE_ARRAY, "list", errorInfo);
	if (ret) return 1;
	for (int i = 0; i < req["list"].size(); ++i) {
		ret = CheckType(param, req["list"][i], "mode", JSON_TYPE_INT, "list-mode", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, req["list"][i], "id", JSON_TYPE_STRING, "list-id", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, req["list"][i], "date", JSON_TYPE_OBJECT, "list-date", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, req["list"][i]["date"], "from", JSON_TYPE_STRING, "list-date-from", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, req["list"][i]["date"], "to", JSON_TYPE_STRING, "list-date-to", errorInfo);
		if (ret) return 1;
		std::string from_Date = req["list"][i]["date"]["from"].asString();
		std::string to_Date = req["list"][i]["date"]["to"].asString();
		if (from_Date.size() != 8 || to_Date.size() != 8 || to_Date < from_Date) {
			MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckType, request date error", param.log.c_str());
			errorInfo.Set(51101, "请求格式异常: date from-to err");
			return 1;
		}
	}
	return 0;
}

int ReqChecker::CheckB116(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, req, "wanle", JSON_TYPE_ARRAY, "wanle", errorInfo);
	if (ret) {
		return 1;
	}
	for (int i = 0; i < req["wanle"].size(); ++i) {
		ret = CheckType(param, req["wanle"][i], "product_id", JSON_TYPE_STRING, "wanle-product_id", errorInfo);
		if (ret) return 1;
		if (req["wanle"][i].isMember("date")) {
			ret = CheckType(param, req["wanle"][i], "date", JSON_TYPE_STRING, "wanle-date", errorInfo);
			if (ret) return 1;
		}
	}
	return 0;
}

int ReqChecker::CheckPoiList(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (param.type == "p101" || param.type == "p202") {
		ret = CheckType(param, req, "mode", JSON_TYPE_INT, "ssv004 mode", errorInfo);
		if (ret) return 1;
	}
	ret = CheckType(param, req, "page", JSON_TYPE_INT, "page", errorInfo);
	if (ret) return 1;
	if (req.isMember("page_cnt")) {
		ret = CheckType(param, req, "page_cnt", JSON_TYPE_INT, "pagecnt", errorInfo);
		if (ret) return 1;
	}
	ret = CheckType(param, req, "cids", JSON_TYPE_ARRAY, "cids", errorInfo);
	if (ret) return 1;
	for (int i = 0; i < req["cids"].size(); i ++) {
		ret = CheckType(param, req["cids"][i], JSON_TYPE_STRING, "cids i", errorInfo);
		if (ret) return 1;
	}
	ret = CheckFilter(param, req, errorInfo);
	if (ret) return 1;
	return 0;
}

int ReqChecker::CheckCostomTraffic(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	if (!req.isMember("customTraffic") or req["customTraffic"].isNull()) {
		return 0;
	}
	ret = CheckType(param, req,"customTraffic", JSON_TYPE_OBJECT, "customTraffic", errorInfo);
	if (ret) return ret;

	Json::Value::Members jCustomList = req["customTraffic"].getMemberNames();
	for (Json::Value::Members::iterator it = jCustomList.begin(); it != jCustomList.end(); it ++) {
		std::string key = *it;
		ret = CheckType(param, req["customTraffic"], key, JSON_TYPE_OBJECT, "customTraffic-item", errorInfo);
		if (ret) return 1;
		Json::Value& jCTraf = req["customTraffic"][key];
		ret = CheckType(param, jCTraf, "dur", JSON_TYPE_INT, "customTraffic-dur", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCTraf, "type", JSON_TYPE_INT, "customTraffic-type", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCTraf, "dist", JSON_TYPE_INT, "customTraffic-dist", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCTraf, "no", JSON_TYPE_STRING, "customTraffic-no", errorInfo);
		if (ret) return 1;
	}

	return 0;
}

int ReqChecker::CheckProductInfo(const QueryParam& param, Json::Value& jProduct, ErrorInfo& errorInfo) {
	if (jProduct.isNull() || !jProduct.isMember("hotel")) return 0;
	//hotel
	Json::Value::Members hotel_key = jProduct["hotel"].getMemberNames();
	for (auto it = hotel_key.begin(); it != hotel_key.end(); it ++) {
		Json::Value& jHotel = jProduct["hotel"][*it];
		if (!jHotel.isMember("checkin") || !jHotel.isMember("checkout")) {
			MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckCityInfo, request error: %s", param.log.c_str(), "hotel-checkin or hotel-checkout");
			errorInfo.Set(51101, "请求格式异常: hotel-checkin or hotel-checkout");
			return 1;
		}
		int ret = CheckType(param, jHotel, "id", JSON_TYPE_STRING, "hotel-id", errorInfo);
		if (ret) return 1;
		ret = CheckStringFormat(param, jHotel["id"].asString(), STRING_TYPE_ID, "custom-hotel-id", errorInfo);
		if (ret) return 1;
		if (jHotel.isMember("coord") and !jHotel["coord"].isNull()) {
			ret = CheckType(param, jHotel, "coord", JSON_TYPE_STRING, "hotelcoord", errorInfo);
			if (ret) return 1;
			ret = CheckStringFormat(param, jHotel["coord"].asString(), STRING_TYPE_COORD, "custom-hotel-coord", errorInfo);
			if (ret) return 1;
		}
	}
	return 0;
}
int ReqChecker::CheckProduct(const QueryParam& param, Json::Value& jProduct, ErrorInfo errorInfo) {
	int ret = 0;
	//ret = CheckType(param, jProduct, "hotel", JSON_TYPE_OBJECT, "product-hotel", errorInfo);
	if (ret) return 1;
	ret = CheckProductInfo(param, jProduct, errorInfo);
	if (ret) return 1;
	return 0;
}
int ReqChecker::CheckS126(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, req, "list", JSON_TYPE_ARRAY, "list", errorInfo);
	if (ret) return 1;
	Json::Value& jProduct = req["product"];
	ret = CheckProduct(param, jProduct, errorInfo);
	if (ret) return 1;
	for (int  i = 0; i < req["list"].size(); i++) {
		Json::Value jReq;
		jReq["city"] = req["list"][i];
		ret = CheckCity(param, jReq, errorInfo);
		if (ret) return 1;
		ret = CheckCityPrefer(param, req, errorInfo);
		if (ret) return 1;
		if (jReq.isMember("ridx")) {
			ret = CheckType(param, req["list"][i], "ridx", JSON_TYPE_INT, "list-ridx", errorInfo);
			if (ret) return 1;
		}
		if (!req["list"][i].isMember("originView") or req["list"][i]["originView"].isNull()) {
			return 0;
		} else {
			ret = CheckType(param, req["list"][i], "originView", JSON_TYPE_OBJECT, "originView", errorInfo);
			if (ret) return ret;
			Json::Value jView;
			jView["view"] = req["list"][i]["originView"];
			ret = CheckRoute(param, jView, errorInfo);
			if (ret) return 1;
			//ret = CheckTrafficPass(param, jReq["city"], errorInfo);
			//if (ret) return 1;
		}
	}
	return 0;
}

int ReqChecker::CheckId(const std::string& str) {
	if (str.size() == 0) {
		return 1;
	}
	if (str.find("_") != std::string::npos) {
		return 1;
	}
	return 0;
}


int ReqChecker::CheckTrafficPass(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	Json::Value& jCity = req["city"];
	if (!jCity.isMember("traffic_pass")) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckTrafficPass, request error: traffic_pass is not a member of city", param.log.c_str());
		errorInfo.Set(51101, "请求格式异常: traffic_pass is not a member of city");
		return 1;
	}

	ret = CheckType(param, jCity, "traffic_pass", JSON_TYPE_OBJECT, "city-traffic_pass", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity["traffic_pass"], "day",  JSON_TYPE_ARRAY, "city-traffic_pass-day", errorInfo);
	if (ret) return 1;
	for (int i = 0 ; i < jCity["traffic_pass"]["day"].size(); ++i) {
		ret = CheckType(param, jCity["traffic_pass"]["day"][i], "date", JSON_TYPE_STRING, "city-traffficPass-day[]-date", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["traffic_pass"]["day"][i], "stime", JSON_TYPE_STRING, "city-view-day[]-stime", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["traffic_pass"]["day"][i], "etime", JSON_TYPE_STRING, "city-traffic_pass-day[]-etime", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jCity["traffic_pass"]["day"][i], "traffic_pass", JSON_TYPE_ARRAY, "city-traffic_pass-day[]-traffic_pass", errorInfo);
		if (ret) return 1;
		for (int j = 0; j < jCity["traffic_pass"]["day"][i]["view"].size(); ++j) {
			ret = CheckType(param, jCity["traffic_pass"]["day"][i]["view"][j], JSON_TYPE_OBJECT, "city-traffic_pass-day[]-view[]", errorInfo);
			if (ret) return 1;
			Json::Value& jDayView = jCity["traffic_pass"]["day"][i]["view"][j];
			ret = CheckType(param, jDayView, "id", JSON_TYPE_STRING, "city-traffic_pass-day[]-view[]-id", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "dur", JSON_TYPE_INT, "city-traffic_pass-day[]-view[]-dur", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "stime", JSON_TYPE_STRING, "city-traffic_pass-day[]-view[]-stime", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "etime", JSON_TYPE_STRING, "city-traffic_pass-day[]-view[]-etime", errorInfo);
			if (ret) return 1;
			ret = CheckType(param, jDayView, "type", JSON_TYPE_INT, "city-traffic_pass-day[]-view[]-type", errorInfo);
			if (ret) return 1;
		}
	}
	ret = CheckType(param, jCity["traffic_pass"], "summary",  JSON_TYPE_OBJECT, "city-traffic_pass-summary", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, jCity["traffic_pass"]["summary"], "days",  JSON_TYPE_ARRAY, "city-traffic_pass-summary-days", errorInfo);
	if (ret) return 1;
	for (int i = 0; i < jCity["traffic_pass"]["summary"]["days"].size(); ++i) {
		Json::Value& jDay = jCity["traffic_pass"]["summary"]["days"][i];
		ret = CheckType(param, jDay, "date",  JSON_TYPE_STRING, "city-traffic_pass-summary-days[]-date", errorInfo);
		if (ret) return 1;
		ret = CheckType(param, jDay, "pois",  JSON_TYPE_ARRAY, "city-traffic_pass-summary-days[]-pois", errorInfo);
		if (ret) return 1;
		for (int j = 0; j < jDay["pois"].size(); j++) {
			ret = CheckType(param, jDay["pois"][j], "id",  JSON_TYPE_STRING, "city-traffic_pass-summary-days[]-pois[]-id", errorInfo);
			if (ret) return 1;
		}
	}
	return 0;
}

int ReqChecker::CheckSSV007(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckCity(param, req, errorInfo);
	if (ret) return 1;
	ret = CheckTour(param, req, errorInfo);
	if (ret) return 1;
	return 0;
}
int ReqChecker::CheckTour(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo) {
	int ret = 0;
	ret = CheckType(param, req["city"], "tour", JSON_TYPE_OBJECT, "city_tourSet error", errorInfo);
	if (ret) return 1;
	ret = CheckType(param, req["city"]["tour"], "product_id", JSON_TYPE_STRING, "city_tour_product_id error", errorInfo);
	if (ret) return 1;
	std::string productId = req["city"]["tour"]["product_id"].asString();
	if (!req["product"].isMember("tour") || !req["product"]["tour"].isMember(productId)) {
		MJ::PrintInfo::PrintErr("[%s]ReqChecker::CheckCityInfo, request error: %s", param.log.c_str(), "product no tourId");
		errorInfo.Set(51105, "请求格式异常: product 不含tourId");
		return 1;
	}
	Json::Value& jTour = req["product"]["tour"][productId];
	for (int i = 0; i < jTour["route"].size(); i ++) {
		ret = CheckCityInfo(param, jTour["route"][i], errorInfo);
		if (ret) return 1;
		ret = CheckRoute(param, jTour["route"][i], errorInfo);
		if (ret) return 1;
	}
	return 0;
}
