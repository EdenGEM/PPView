#ifndef __PATH_TRAFFIC_H__
#define __PATH_TRAFFIC_H__

#include <iostream>
#include "BasePlan.h"

class PathTraffic {
public:
	static int GetTrafID(BasePlan* basePlan, std::tr1::unordered_set<std::string>& place_id_set);
	static int GetTrafPair(BasePlan* basePlan, std::tr1::unordered_set<std::string>& pairSet, bool needReal = false);
	static int GetTrafficSummary(BasePlan* basePlan, std::vector<TrafficItem*>& trafficList);
	static const TrafficItem* GetTrafItem(BasePlan* basePlan, const std::string& from_id, const std::string& to_id, const std::string& date);
	static const TrafficItem* GetTrafItem(BasePlan* basePlan, const std::string& from_id, const std::string& to_id);
	static int ZeroTrafDur(BasePlan* basePlan);
	//smz
	static int FilterIdPairSetForMultiAndCoreHotel(std::tr1::unordered_set<std::string>& idPairSet);
	static int FilterIdSetForMultiAndCoreHotel(std::tr1::unordered_set<std::string>& idSet);

};

class DetailCmp {
public:
	static bool CmpWalk(const TrafficDetail* pa, const TrafficDetail* pb) {
		const TrafficItem* a = pa->m_trafficItem;
		const TrafficItem* b = pb->m_trafficItem;
		bool res = false;
		if (a->m_stat > b->m_stat) {
			res = true;
		} else if (a->m_stat < b->m_stat) {
			res = false;
		} else {
			if (a->m_order <= 0 && b->m_order > 0 ) {
				res = false;
			} else if (a->m_order > 0 && b->m_order <= 0 ) {
				res = true;
			} else if (a->m_order > 0 && b->m_order > 0 ) {
				res = a->m_order < b->m_order;
			} else {
				if (a->_dist < b->_dist) {
					res = true;
				} else if (a->_dist > b->_dist) {
					res = false;
				}
			}
		}
		return res;
	}
	static bool CmpBus(const TrafficDetail* pa, const TrafficDetail* pb) {
		const TrafficItem* a = pa->m_trafficItem;
		const TrafficItem* b = pb->m_trafficItem;
		int res = 0;
		if (a->m_stat > b->m_stat) {
			res = true;
		} else if (a->m_stat < b->m_stat) {
			res = false;
		} else {
			if (a->m_order <= 0 && b->m_order > 0 ) {
				res = false;
			} else if (a->m_order > 0 && b->m_order <= 0 ) {
				res = true;
			} else if (a->m_order > 0 && b->m_order > 0 ) {
				res = a->m_order < b->m_order;
			} else {
				if (a->_time < b->_time) {
					res = true;
				} else if (a->_time > b->_time) {
					res = false;
				}
			}
		}
		return res;
	}
	static bool CmpTaxi(const TrafficDetail* pa, const TrafficDetail* pb) {
		const TrafficItem* a = pa->m_trafficItem;
		const TrafficItem* b = pb->m_trafficItem;
		int res = 0;
		if (a->m_stat > b->m_stat) {
			res = true;
		} else if (a->m_stat < b->m_stat) {
			res = false;
		} else {
			if (a->m_order <= 0 && b->m_order > 0 ) {
				res = false;
			} else if (a->m_order > 0 && b->m_order <= 0 ) {
				res = true;
			} else if (a->m_order > 0 && b->m_order > 0 ) {
				res = a->m_order < b->m_order;
			} else {
				if (a->_dist < b->_dist) {
					res = true;
				} else if (a->_dist > b->_dist) {
					res = false;
				}
			}
		}
		return res;
	}
};

#endif
