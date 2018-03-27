#ifndef __PLANTHROUGHTPOIS_H__
#define __PLANTHROUGHTPOIS_H__

#include <iostream>
#include <vector>
#include "base/ReqChecker.h"
#include "MJCommon.h"

class PoisInfo {
public:
	int pdur;
	int trafficIdx;		//对应交通的下标
	int custom;		//是否为用户自定义
	std::string id;
	Json::Value product;
	Json::Value addInfo;
	Json::Value rawInfo;
	std::string m_play;
	PoisInfo(std::string id = "", int dur = 0, Json::Value product = Json::Value(), Json::Value addInfo = Json::Value(), Json::Value traffic = Json::Value()):id(id), pdur(dur), product(product), addInfo(addInfo) {
	}
};

class PlanDayInfo {
private:
	std::vector<PoisInfo> m_pois;
	std::vector<Json::Value> m_trafficBetweenPoi;
public:
	int LoadPoisData(Json::Value& jDayInfo, const QueryParam& param, const std::string &date, const Json::Value & reqProduct);
	int LoadPoisTraffic(Json::Value& jTraffic);
	int GetDayViewInfo(const QueryParam& param, Json::Value& jDayView, std::string stime) const;		//stime 临时
private:
	Json::Value GetTrafficByIdx(int idx) const;
	int MakeTraffic(Json::Value& traffic) const;
};

class PlanThroughtPois {
private:
	std::tr1::unordered_map<std::string, PlanDayInfo> m_planDays;		//记录date - days[i]
	Json::Value m_trafficOfCity;
	Json::Value m_cityInfo;
	Json::Value m_customTraffic;	//记录用户自定义pois间交通
	Json::Value m_product;
public:
	int PlanPath(const QueryParam& param, Json::Value& req, Json::Value& resp, ErrorInfo& error);
	static int HandleTrafficPass(Json::Value& jTrafficPass, const QueryParam& param, const std::string& deptTime, int trafficDur, Json::Value jtourParam = Json::Value(), Json::Value jProduct = Json::Value());
	static int MakeOutputTrafficPass(Json::Value& req, const QueryParam& param, Json::Value& resp);
private:
	int LoadReq(const Json::Value& req, const QueryParam& param);
	int LoadCityInfo(const QueryParam& param, const Json::Value& req);
	int LoadReqPois(const QueryParam& param);	//获取请求中的pois信息
	int MakeOutputS128(const Json::Value & req, const QueryParam& param, Json::Value& resp);
	static int GetTrafficDur(Json::Value& jTraffic, Json::Value& jProduct);
};


#endif
