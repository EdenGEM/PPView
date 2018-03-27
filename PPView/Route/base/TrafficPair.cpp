#include <iostream>
#include "define.h"
#include "PathTraffic.h"
#include "TrafficPair.h"

int TrafficPair::GetTraffic(BasePlan* basePlan) {
	char buff[1024];
	std::tr1::unordered_set<std::string> trafKeySet;
	// 1 KeyNode间交通
	for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
		const RouteBlock* rBlock = basePlan->m_RouteBlockList[i];
		const LYPlace* arvPlace = rBlock->_arrive_place;
		const LYPlace* deptPlace = rBlock->_depart_place;
		const std::vector<std::string>& dates = rBlock->_dates;

		const LYPlace* firstHotel = NULL;
		const LYPlace* lastHotel = NULL;
		if (!rBlock->_hotel_list.empty()) {
			firstHotel = rBlock->_hotel_list[0];
			lastHotel = rBlock->_hotel_list[rBlock->_hotel_list.size() - 1];
		}
		const LYPlace* secondHotel = NULL;
		const LYPlace* lastButOneHotel = NULL;//倒数第二个
		if (rBlock->_hotel_list.size() > 1) {
			secondHotel = rBlock->_hotel_list[1];
			lastButOneHotel = rBlock->_hotel_list[rBlock->_hotel_list.size() - 2];
		}

		trafKeySet.insert(arvPlace->_ID + "_" + deptPlace->_ID + "_" + dates[0]);
		trafKeySet.insert(arvPlace->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 1]);
		if (dates.size() > 2) {
			trafKeySet.insert(arvPlace->_ID + "_" + deptPlace->_ID + "_" + dates[1]);
			trafKeySet.insert(arvPlace->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 2]);
		}

		if (firstHotel && lastHotel) {
			trafKeySet.insert(arvPlace->_ID + "_" + firstHotel->_ID + "_" + dates[0]);
			trafKeySet.insert(lastHotel->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 1]);
			if (dates.size() > 1) {
				trafKeySet.insert(arvPlace->_ID + "_" + firstHotel->_ID + "_" + dates[1]);
				trafKeySet.insert(lastHotel->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 2]);
			}
		}
		//hy add: 第一个酒店可能不住，也不放行李 且倒数第一个酒店可能不住，也不取行李
		if (secondHotel && lastButOneHotel) {
			trafKeySet.insert(arvPlace->_ID + "_" + secondHotel->_ID + "_" + dates[0]);
			trafKeySet.insert(lastButOneHotel->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 1]);
			if (dates.size() > 1) {
				trafKeySet.insert(arvPlace->_ID + "_" + secondHotel->_ID + "_" + dates[1]);
				trafKeySet.insert(lastButOneHotel->_ID + "_" + deptPlace->_ID + "_" + dates[dates.size() - 2]);
			}
		}

		for (int j = 1; j < rBlock->_hInfo_list.size(); ++j) {
			const HInfo* hInfo = rBlock->_hInfo_list[j];
			const HInfo* lastHInfo = rBlock->_hInfo_list[j - 1];
			trafKeySet.insert(lastHInfo->m_hotel->_ID + "_" + hInfo->m_hotel->_ID + "_" + hInfo->m_checkIn);
		}
	}

	int ret = PathTraffic::GetTrafPair(basePlan, trafKeySet, basePlan->m_realTrafNeed & REAL_TRAF_ADVANCE);
	if (ret != 0) return 1;

	for (std::tr1::unordered_set<std::string>::iterator it = trafKeySet.begin(); it != trafKeySet.end(); ++it) {
		std::string trafKey = *it;
		std::vector<std::string> itemList;
		ToolFunc::sepString(trafKey, "_", itemList);
		if (itemList.size() < 2) continue;
		std::string fromID = itemList[0];
		std::string toID = itemList[1];
		const TrafficItem* trafItem = NULL;
		if (itemList.size() > 2) {
			std::string date = itemList[2];
			if (fromID == "arvNullPlace" || toID == "deptNullPlace") {
				TrafficItem* traf = new TrafficItem(*TrafficData::_blank_traffic_item);
				trafItem = traf;
			}
			else
				trafItem = PathTraffic::GetTrafItem(basePlan, fromID, toID, date);
		} else {
			if (fromID == "arvNullPlace" || toID == "deptNullPlace") {
				TrafficItem* traf = new TrafficItem(*TrafficData::_blank_traffic_item);
				trafItem = traf;
			}
			else
				trafItem = PathTraffic::GetTrafItem(basePlan, fromID, toID);
		}
		if (!trafItem) return 1;
	}

	return 0;
}
