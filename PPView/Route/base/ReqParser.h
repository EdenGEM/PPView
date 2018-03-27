#ifndef _REQ_PARSER_H_
#define _REQ_PARSER_H_

#include <iostream>
#include "BasePlan.h"

class ReqParser {
public:
	static int DoParse(const QueryParam&, Json::Value& req, BasePlan* basePlan);
public:
	static int DoParseSSV005(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseSSV006(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseSSV007(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseP202(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseP201(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseP104(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseP105(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int DoParseB116(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
    static int DoParseS128(const QueryParam& param, Json::Value& req, BasePlan* basePlan);

	static int ParseFilterP104(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseDetailList(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseProductIDList(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseBasicParam(const QueryParam&, BasePlan* basePlan);
	static int ParseBasicReq(Json::Value& req, BasePlan* basePlan);
	static int ParseCity(const QueryParam&, Json::Value& req, BasePlan* basePlan);
	static int ParseCityPrefer(const QueryParam &,Json::Value& req, BasePlan* basePlan);
	static int ParseProduct(const QueryParam&, Json::Value& req, BasePlan* basePlan);
	static int ParseCarRental(const QueryParam&, Json::Value& req, BasePlan* basePlan);
	static int ParseFilter(const QueryParam&, Json::Value& req, BasePlan* basePlan);
	static int ParseDaysProd(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseDaysPois(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static const LYPlace* GetHotelByDidx (BasePlan* basePlan, int didx);
	static int ParseDaysNotPlan(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseCustomTraf(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseCustomTrafFromView(Json::Value& req, Json::Value& jCustomTrafList);
	static int ParseTrafFromView(Json::Value& jView, Json::Value& jCustomTrafList, Json::Value& jLastTripTrafList);
	static TrafficItem* ParseTraffic2TrafItem (const QueryParam& param, BasePlan* basePlan, std::string trafKey, Json::Value& jTraffic);

	static int ParseCustomPois(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseCid(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
	static int ParseCustomHotel(const QueryParam& qParam, Json::Value& jHotel, BasePlan* basePlan, const std::string& hid);

	static const LYPlace* GetLYPlace(BasePlan* basePlan, const std::string& id, int pType,const QueryParam& param, ErrorInfo& errorInfo);
	static const LYPlace* GetLYPlace(const std::string& id, int pType,const QueryParam& param, ErrorInfo& errorInfo);
	static int RemoveTogoPoi(BasePlan *basePlan);//用于删除已经去过的POI
	static int CompleteHotel(const QueryParam& param, Json::Value& req, BasePlan* basePlan, int routeIdx);
	static int NewCity2OldCity(const QueryParam& param, Json::Value& req);
	static int DelClashPois(BasePlan* basePlan, const PlaceOrder placeOrder, std::tr1::unordered_set<const LYPlace*>& delPoisSet);
	static int DealClashOfSpecialPois(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, std::tr1::unordered_set<const LYPlace*>& delPoisSet);
	static bool IsBaoChe(BasePlan* basePlan, Json::Value& req);
	static bool ParseLockDay(const Json::Value& jDayList, Json::Value& lockDateList, Json::Value& jWarningList) {
		std::tr1::unordered_set<int> lockDidx;
		for (int i = 0; i < jDayList.size(); i++) {
			if (jDayList[i].isMember("locking") and jDayList[i]["locking"].isInt() and jDayList[i]["locking"].asInt()) {
				lockDidx.insert(i);
				if (jDayList[i].isMember("date") and jDayList[i]["date"].isString()) {
					lockDateList.append(jDayList[i]["date"].asString());
				}
			}
		}
		Json::Value jRemainWaring;
		for (int i = 0; i < jWarningList.size(); i++) {
			Json::Value& jWarning = jWarningList[i];
			if (jWarning.isMember("didx") and jWarning["didx"].isInt()) {
				if (lockDidx.count(jWarning["didx"].asInt())) {
					jRemainWaring.append(jWarning);
				}
			}
		}
		jWarningList = jRemainWaring;
		return true;
	}
};

#endif
