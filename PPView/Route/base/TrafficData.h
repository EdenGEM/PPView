#ifndef _TRAFFIC_DATA_H
#define _TRAFFIC_DATA_H

#include <iostream>
#include <algorithm>
#include <limits>
#include <vector>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "define.h"
#include "MJCommon.h"
#include "RouteConfig.h"
#include "LYConstData.h"
#include "PathView.h"
#include "Utils.h"
#include "LogDump.h"
#include "http/SocketClientEpoll.h"

const int kWalkDistLimit = 2000;
const int kWalkBusDistLimit = 1000;
const int kWalkTaxiDistLimit = 1000;
//const int kWalkTaxiDistLimit = 800;
const int kTaxiUseLimit = 800;
const int kTaxiForcedUseWalkLimit = 500;
const int kWalkBusTaxiDistLimit = 1000;
const int kWalkBusTaxiDistUpperLimit = 2000;

class BasePlan;

class TrafficData {
public:
    static int Init();
    static int Release();
	//8002
	static int GetTrafID(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& placeIdSet, std::string& query, int& timeCost);
	// 8003
	static int GetTrafPair(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::string& query, int& timeCost);
	// 8009 补充摘要
	static int GetTrafficSummary(BasePlan* basePlan, std::vector<TrafficItem*>& trafficList, std::string& query, int& timeCost);

	static bool IsReal(const std::string& mid);
	static int GetHttpData(const QueryParam& qParam, const std::string& addr, const int& port, const int& timeout, MJ::MJHttpMethod mjMethod, const std::string& urlPath, const std::string& type, const std::string& query, Json::Value& resp);
private:
	static bool LoadTrafficWhite(const std::string& data_path, std::vector<std::string>& traffic_white_file_list);
	static int LoadTrafficMode();
//8002接口交通
	static int GetTraf8002(BasePlan* basePlan, const std::vector<std::string>& idList, int carRent, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost);
//8002自计算交通
	static int GetTrafStatic(BasePlan* basePlan, const std::vector<std::string>& idList, int carRent, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost);
//8003接口交通
	static int GetTraf8003(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost, int carRent);
//8003自计算交通
	static int GetTrafStaticReal(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost, int carRent);
	static int GetTrafficFromServer8009(BasePlan* basePlan, std::tr1::unordered_set<std::string>& midSet, std::tr1::unordered_map<std::string, Json::Value>& trafficSummaryList, std::string& query, int& timeCost);
//点对制作8002交通
	static Json::Value GetTratItemStatic(BasePlan* basePlan, const std::string& idA, const std::string& idB);
//静态获取placeA - placeB 交通 （8003）样式
	static Json::Value GetTratItemRealStatic(BasePlan* basePlan, const std::string& idA, const std::string& idB);

	static int ChooseTraf(BasePlan* basePlan, const std::string& trafKey, Json::Value& jValue, int trafPrefer, bool useRealTraf, TrafficItem* hitTrafItem);
	static int ChooseTraf(std::tr1::unordered_map<std::string, const TrafficItem*>& midMap, int trafPrefer, std::string& selectMid);

	static int MakeQuery8002(BasePlan* basePlan, int carRent, const std::vector<std::string>& idList, std::string& query);
	static int MakeQuery8003(BasePlan* basePlan, int needReal,const std::tr1::unordered_set<std::string>& pairSet, std::string& query, int carRent);
	static std::string MakeQuery8009(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& midSet);
	static int LYPlaceType2TrafPoiType(int type);
public:
	static TrafficItem* _blank_traffic_item;
	static TrafficItem* _self_traffic_item;  // 从A到A
	static TrafficItem* _fake_traffic_item;
	static TrafficItem* _fake_hotel_traffic_item;
	static TrafficItem* m_zeroTraffic;
	static TrafficItem* m_zipTraffic;
	static TrafficItem* m_normalTraffic;
	static std::tr1::unordered_map<std::string, int> m_whitePreferMap;
	static const std::string m_trafKeySplit;
private:
	//shyy
	static int SetTourTraf8002(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& placeIdSet);
	static int SetTourTraf8003(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet);
	static int SelectTourTraffic(BasePlan* basePlan, std::string start2endTrafKey);
	static int InsertjDataList(BasePlan* basePlan, const std::string& start, const std::string& dest, const std::string& time, Json::Value& jData, std::tr1::unordered_set<std::string>& jDataList);
	static int SetCustomPois(BasePlan* basePlan, const LYPlace* tourPlace, Json::Value& jCustomPoisList, std::tr1::unordered_set<std::string>& customId);
	static int SetTrafficPair(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, Json::Value& jData, Json::Value& jCustomPoisList);
	static int AddTrafficItem (BasePlan* basePlan, const std::string trafKey, const std::string addTrafKey);
	//shyy end
};

struct TrafDistCmp {
	bool operator()(const TrafficItem* pa, const TrafficItem* pb) {
		if (pa->m_stat > pb->m_stat) {
			return true;
		} else if (pa->m_stat < pb->m_stat) {
			return false;
		} else {
			if (pa->m_order != pb->m_order) {
				if (pa->m_order > 0 && pb->m_order > 0) {
					return (pa->m_order < pb->m_order);
				} else {
					return (pa->m_order > pb->m_order);
				}
			} else {
				if (pa->_dist < pb->_dist) {
					return true;
				} else if (pa->_dist > pb->_dist) {
					return false;
				} else {
					return (pa->_time < pb->_time);
				}
			}
		}
		return true;
	}
};

struct TrafTimeCmp {
	bool operator()(const TrafficItem* pa, const TrafficItem* pb) {
		if (pa->m_stat > pb->m_stat) {
			return true;
		} else if (pa->m_stat < pb->m_stat) {
			return false;
		} else {
			if (pa->m_order != pb->m_order) {
				if (pa->m_order > 0 && pb->m_order > 0) {
					return (pa->m_order < pb->m_order);
				} else {
					return (pa->m_order > pb->m_order);
				}
			} else {
				if (pa->_time < pb->_time) {
					return true;
				} else if (pa->_time > pb->_time) {
					return false;
				} else {
					return (pa->_dist < pb->_dist);
				}
			}
		}
		return true;
	}
};

#endif

