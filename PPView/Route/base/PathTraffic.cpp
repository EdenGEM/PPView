#include <iostream>
#include "MJCommon.h"
#include "TrafficData.h"
#include "PathTraffic.h"

// 计算所有node间的traffic
int PathTraffic::GetTrafID(BasePlan* basePlan, std::tr1::unordered_set<std::string>& place_id_set) {
	std::string query = "";
	int timeCost = 0;

	int ret = FilterIdSetForMultiAndCoreHotel(place_id_set);

	ret = TrafficData::GetTrafID(basePlan, place_id_set, query, timeCost);
//	basePlan->m_cost.m_traffic8002 = timeCost;
	if (ret) {
		basePlan->m_error.Set(52101, std::string("8002接口异常"));
		MJ::PrintInfo::PrintErr("[%s]PathTraffic::GetTrafID, 8002接口异常: %s", basePlan->m_qParam.log.c_str(), query.c_str());
		return 1;
	}
	ZeroTrafDur(basePlan);

	return 0;
}

int PathTraffic::GetTrafficSummary(BasePlan* basePlan, std::vector<TrafficItem*>& trafficList) {
	int timeCost = 0;
	std::string query = "";
	int ret  = 0;
	if (!basePlan->m_useStaticTraf)
		ret = TrafficData::GetTrafficSummary(basePlan, trafficList, query, timeCost);
	if (ret) {
		basePlan->m_error.Set(52101, std::string("8009接口异常"));
		MJ::PrintInfo::PrintErr("[%s]PathTraffic::GetTrafficSummary, 8009接口异常: %s", basePlan->m_qParam.log.c_str(), query.c_str());
		return 1;
	}
	return 0;
}

int PathTraffic::GetTrafPair(BasePlan* basePlan, std::tr1::unordered_set<std::string>& pairSet, bool needReal) {
	int timeCost = 0;
	std::string query = "";

	int ret = FilterIdPairSetForMultiAndCoreHotel(pairSet);

	ret = TrafficData::GetTrafPair(basePlan, pairSet, needReal, query, timeCost);
//	basePlan->m_cost.m_traffic8003 += timeCost;
	if (ret != 0) {
		basePlan->m_error.Set(52101, std::string("8003接口异常"));
		MJ::PrintInfo::PrintErr("[%s]PathTraffic::GetTrafPair, 8003接口异常: %s", basePlan->m_qParam.log.c_str(), query.c_str());
		return 1;
	}
	ZeroTrafDur(basePlan);
	return 0;
}

const TrafficItem* PathTraffic::GetTrafItem(BasePlan* basePlan, const std::string& from_id, const std::string& to_id, const std::string& date) {
	char buff[100];
	const TrafficItem* traffic = basePlan->GetTraffic(from_id, to_id, date);
	if (traffic == NULL) {
		MJ::PrintInfo::PrintErr("[%s]PathTraffic::GetTrafItem, 交通缺失 %s_%s_%s", basePlan->m_qParam.log.c_str(), from_id.c_str(), to_id.c_str(), date.c_str());
		snprintf(buff, sizeof(buff), "交通缺失 %s_%s_%s", from_id.c_str(), to_id.c_str(), date.c_str());
		basePlan->m_error.Set(55103, std::string(buff));
		return NULL;
	}
	return traffic;
}

const TrafficItem* PathTraffic::GetTrafItem(BasePlan* basePlan, const std::string& from_id, const std::string& to_id) {
	char buff[100];
	const TrafficItem* traffic = basePlan->GetTraffic(from_id, to_id);
	if (traffic == NULL) {
		MJ::PrintInfo::PrintErr("[%s]PathTraffic::GetTrafItem, 交通缺失 %s_%s", basePlan->m_qParam.log.c_str(), from_id.c_str(), to_id.c_str());
		snprintf(buff, sizeof(buff), "交通缺失 %s->%s", from_id.c_str(), to_id.c_str());
		basePlan->m_error.Set(55103, std::string(buff));
		return NULL;
	}
	return traffic;
}

// arvPoi和deptPoi为空，且不需要酒店时，将和酒店有关的交通的时长置为0
int PathTraffic::ZeroTrafDur(BasePlan* basePlan) {
	if (basePlan->m_RouteBlockList.empty() || basePlan->m_KeyNode.empty() || basePlan->m_RouteBlockList[0]->_need_hotel)	return 1;
	std::string coreHotelId = "";
	const KeyNode* arvNode = basePlan->m_KeyNode[0];
	const KeyNode* deptNode = basePlan->m_KeyNode[basePlan->GetKeyNum() - 1];
	if (arvNode->_type == NODE_FUNC_KEY_ARRIVE && arvNode->_place && arvNode->_place->_t & LY_PLACE_TYPE_HOTEL) coreHotelId = arvNode->_place->_ID;
	if (deptNode->_type == NODE_FUNC_KEY_DEPART && deptNode->_place && deptNode->_place->_t & LY_PLACE_TYPE_HOTEL) coreHotelId = deptNode->_place->_ID;
	std::tr1::unordered_map<std::string, TrafficItem*>& trafMap = basePlan->m_TrafficMap;
	for (std::tr1::unordered_map<std::string, TrafficItem*>::iterator it = trafMap.begin(); it != trafMap.end(); ++it) {
		if (it->second->m_startP == coreHotelId || it->second->m_stopP == coreHotelId) {
			it->second->_time = 0;
			it->second->_dist = 0;
		}
	}
	return 0;
}

int PathTraffic::FilterIdSetForMultiAndCoreHotel(std::tr1::unordered_set<std::string>& idSet) {
	std::tr1::unordered_set<std::string> filterIDSet;
	for (auto it = idSet.begin() ; it != idSet.end(); it ++) {
		std::string newId = BasePlan::GetCutId(*it);
		filterIDSet.insert(newId);
	}
	idSet = filterIDSet;
	return 0;
}

int PathTraffic::FilterIdPairSetForMultiAndCoreHotel(std::tr1::unordered_set<std::string>& idPairSet) {
	std::tr1::unordered_set<std::string> filterIDPairSet;
	for (auto it = idPairSet.begin() ; it != idPairSet.end(); it ++) {
		std::vector<std::string> items;
		std::string trafKey = (*it);
		ToolFunc::sepString(trafKey, "_", items);
		if (items.size() != 3 && items.size() != 2) {
			continue;
		}
		std::string startId = BasePlan::GetCutId(items[0]);
		std::string endId = BasePlan::GetCutId(items[1]);
		if (items.size() == 3) {
			filterIDPairSet.insert(startId + "_" + endId + "_" + items[2]);
		} else {
			filterIDPairSet.insert(startId + "_" + endId);
		}
	}
	idPairSet = filterIDPairSet;
	return 0;
}
