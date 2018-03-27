#include "MJCommon.h"
#include "BasePlan.h"
#include "TrafficData.h"
#include <algorithm>

TrafficItem* TrafficData::_blank_traffic_item = NULL;
TrafficItem* TrafficData::_self_traffic_item = NULL;
TrafficItem* TrafficData::_fake_traffic_item = NULL;
TrafficItem* TrafficData::_fake_hotel_traffic_item = NULL;
TrafficItem* TrafficData::m_zeroTraffic = NULL;
TrafficItem* TrafficData::m_zipTraffic = NULL ;
TrafficItem* TrafficData::m_normalTraffic = NULL;

std::tr1::unordered_map<std::string, int> TrafficData::m_whitePreferMap;

int chooseDbg = 0;

const std::string TrafficData::m_trafKeySplit("_");

int TrafficData::Init() {
	bool ret = LoadTrafficWhite(RouteConfig::data_dir, RouteConfig::traffic_white_file);
	if (!ret) {
		MJ::PrintInfo::PrintErr("TrafficData::Init, Load Traffic White Data error");
		return 1;
	}

	_blank_traffic_item = NULL;
	_blank_traffic_item = new TrafficItem();
	_blank_traffic_item->_mode = TRAF_MODE_DRIVING;
	//    _blank_traffic_item->_cost = 0;
	_blank_traffic_item->_time = 0;
	_blank_traffic_item->_dist = 0;
	_blank_traffic_item->_mid = "TrafficData::blank";

	_self_traffic_item = NULL;
	_self_traffic_item = new TrafficItem();
	_self_traffic_item->_mode = TRAF_MODE_DRIVING;
	//    _self_traffic_item->_cost = 0;
	_self_traffic_item->_time = 0;
	_self_traffic_item->_dist = 1;
	_self_traffic_item->_mid = "TrafficData::self";

	_fake_traffic_item = NULL;
	_fake_traffic_item = new TrafficItem();
	_fake_traffic_item->_mode = TRAF_MODE_DRIVING;
	//    _fake_traffic_item->_cost = 0;
	_fake_traffic_item->_time = 20 * 60;
	_fake_traffic_item->_dist = 1000;
	_fake_traffic_item->_mid = "TrafficData::fake";

	_fake_hotel_traffic_item = NULL;
	_fake_hotel_traffic_item = new TrafficItem();
	_fake_hotel_traffic_item->_mode = TRAF_MODE_DRIVING;
	//    _fake_hotel_traffic_item->_cost = 0;
	_fake_hotel_traffic_item->_time = 60 * 60;
	_fake_hotel_traffic_item->_dist = 15 * 1000;
	_fake_hotel_traffic_item->_mid = "TrafficData::fakeHotel";

	m_normalTraffic = NULL;
	m_normalTraffic = new TrafficItem();
	m_normalTraffic->_mode = TRAF_MODE_DRIVING;
	//    m_normalTraffic->_cost = 0;
	m_normalTraffic->_time = 30 * 60;
	m_normalTraffic->_dist = 10 * 1000;
	m_normalTraffic->_mid = "NormalTraffic";

	m_zeroTraffic = NULL;
	m_zeroTraffic = new TrafficItem();
	m_zeroTraffic->_mode = TRAF_MODE_DRIVING;
	//    m_zeroTraffic->_cost = 0;
	m_zeroTraffic->_time = 0;
	m_zeroTraffic->_dist = 0;
	m_zeroTraffic->_mid = "ZeroTraffic";

	m_zipTraffic = NULL;
	m_zipTraffic = new TrafficItem();
	m_zipTraffic->_mode = TRAF_MODE_DRIVING;
	//    m_zipTraffic->_cost = 0;
	m_zipTraffic->_time = 15 * 60;
	m_zipTraffic->_dist = 5 * 1000;
	m_zipTraffic->_mid = "ZipTraffic";
	return 0;
}

int TrafficData::Release() {
	if (_blank_traffic_item) {
		delete _blank_traffic_item;
		_blank_traffic_item = NULL;
	}
	if (_self_traffic_item) {
		delete _self_traffic_item;
		_self_traffic_item = NULL;
	}
	if (_fake_traffic_item) {
		delete _fake_traffic_item;
		_fake_traffic_item = NULL;
	}
	if (_fake_hotel_traffic_item) {
		delete _fake_hotel_traffic_item;
		_fake_hotel_traffic_item = NULL;
	}
	return 0;
}

// 按id获取两两间交通
int TrafficData::GetTrafID(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& placeIdSet, std::string& query, int& timeCost) {
	int trafPrefer = basePlan->m_trafPrefer;
	std::tr1::unordered_map<std::string, TrafficItem*>& trafMap = basePlan->m_TrafficMap;
	bool isRealTraf = false;

	int ret = 0;
	std::vector<std::string> idList;
	for (std::tr1::unordered_set<std::string>::const_iterator it = placeIdSet.begin(); it != placeIdSet.end(); ++it) {
		std::string id = *it;
		if (LYConstData::IsRealID(id)) {
			idList.push_back(id);
		}
	}
	if (idList.size() <= 1) return 0;

	std::tr1::unordered_map<std::string, Json::Value> jTrafMap;
	int carRent = (trafPrefer == TRAF_PREFER_DRIVING || trafPrefer == TRAF_PREFER_TAXI) ? 1 : 0;
	if (basePlan->m_useStaticTraf) {
		ret = GetTrafStatic(basePlan, idList, carRent, jTrafMap, query, timeCost);
	} else {
		ret = GetTraf8002(basePlan, idList, carRent, jTrafMap, query, timeCost);
	}
	if (ret != 0) return 1;

	for (std::tr1::unordered_map<std::string, Json::Value>::iterator it = jTrafMap.begin(); it != jTrafMap.end(); ++it) {
		// yc add
		std::string trafKey = it->first;
		std::string trafKey_pos = it->first;
		Json::Value& jValue_pos = it->second;

		std::vector<std::string> itemList;
		std::set<std::string> trafModeSet;
		for (int i = 0; i < jValue_pos.size(); ++ i) {
			std::string trafInfo = jValue_pos[i].asString();
			itemList.clear();
			ToolFunc::sepString(trafInfo, "|", itemList);
			trafModeSet.insert(itemList[0]);
		}

		itemList.clear();
		ToolFunc::sepString(trafKey_pos, m_trafKeySplit, itemList);
		if (itemList.size() < 2) {
			MJ::PrintInfo::PrintErr("[%s]TrafficData::ChooseTraf, bad traf key: %s", basePlan->m_qParam.log.c_str(), trafKey_pos.c_str());
			return 1;
		}
		std::string trafKey_neg = itemList[1] + m_trafKeySplit + itemList[0];
		Json::Value jValue = jValue_pos;
		if (jTrafMap.find(trafKey_neg) != jTrafMap.end()) {
			Json::Value& jValue_neg =  jTrafMap[trafKey_neg];
			for (int i = 0; i < jValue_neg.size(); ++ i) {
				itemList.clear();
				ToolFunc::sepString(jValue_neg[i].asString(), "|", itemList);
				if (trafModeSet.find(itemList[0]) == trafModeSet.end()) {
					jValue.append(jValue_neg[i]);
				}
			}
		}
		// yc add end
		if (trafMap.find(trafKey_pos) != trafMap.end()) continue;

		TrafficItem* hitTrafItem = new TrafficItem;
		ret = ChooseTraf(basePlan, trafKey, jValue, trafPrefer, isRealTraf, hitTrafItem);
		if (ret != 0) {
			delete hitTrafItem;
			continue;
		}
		if (trafMap.find(trafKey) == trafMap.end()) {
			trafMap[trafKey] = hitTrafItem;
			if (chooseDbg & 1) { std::cerr << "jjj traf " << trafKey << " " << hitTrafItem->_dist << " " << hitTrafItem->_mode << std::endl; }
		} else {
			delete hitTrafItem;
		}
	}
	SetTourTraf8002(basePlan, placeIdSet);
	for (auto it = trafMap.begin(); it != trafMap.end(); it ++) {
		if (it->second->_time > 3600 * 6) {
			MJ::PrintInfo::PrintErr("TrafficData::GetTrafID %s traffic time > 6h", it->first.c_str());
			it->second->_time = 3600*6;
			//return 1;
		}
	}
	return 0;
}

//8003 选择玩乐相关交通
int TrafficData::SetTourTraf8003(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet) {
	for (auto trafKey: pairSet) {
		SelectTourTraffic(basePlan, trafKey);
	}
	return 0;
}

//8002选择 玩乐交通
int TrafficData::SetTourTraf8002(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& placeIdSet) {
	for (auto startId: placeIdSet) {
		for (auto endId: placeIdSet) {
			if (startId == endId) continue;
			const LYPlace* startPlace = basePlan->GetLYPlace(startId);
			const LYPlace* endPlace = basePlan->GetLYPlace(endId);
			if (!startPlace or !endPlace) continue;
			if (startPlace->_t & LY_PLACE_TYPE_TOURALL or endPlace->_t & LY_PLACE_TYPE_TOURALL) {
				std::string trafKey = startId + m_trafKeySplit + endId;
				SelectTourTraffic(basePlan, trafKey);
			}
		}
	}
	return 0;
}

//目前只要存在接送点，就选择最近的接送点
//(接送点不一定 比直达更近) 是否要选择接送交通 ！！ ???
//并且将原请求的直达交通修改为接送
int TrafficData::SelectTourTraffic(BasePlan* basePlan, std::string start2endTrafKey) {
	std::vector<std::string> itemList;
	ToolFunc::sepString(start2endTrafKey, m_trafKeySplit, itemList);
	if (itemList.size() < 2) {
		return 0;
	}
	std::string startId = itemList[0];
	std::string endId = itemList[1];
	std::string time = "";
	if (itemList.size() > 2) time = itemList[2];
	std::tr1::unordered_map<std::string, TrafficItem*>& trafficMap = basePlan->m_TrafficMap;

	const LYPlace* startPlace = basePlan->GetLYPlace(startId);
	const LYPlace* endPlace = basePlan->GetLYPlace(endId);
	if (!startPlace or !endPlace) return 0;
	if (startPlace->_t & LY_PLACE_TYPE_TOURALL	or endPlace->_t & LY_PLACE_TYPE_TOURALL) {
		std::vector<const LYPlace*> transList;
		basePlan->SelectTransPois(startPlace, endPlace, transList);
		if (transList.size()) {
			std::string start2left = startPlace->_ID + m_trafKeySplit + transList.front()->_ID;
			std::string gather2end = transList.back()->_ID + m_trafKeySplit + endPlace->_ID;
			std::string left2gather = "";
			if (time != "") {
				start2left += m_trafKeySplit + time;
				gather2end += m_trafKeySplit + time;
			}
			if (transList.size() == 2) {
				left2gather = transList.front()->_ID + m_trafKeySplit + transList.back()->_ID;
				if (time != "") left2gather += m_trafKeySplit + time;
				if (!trafficMap.count(left2gather)) {
					MJ::PrintInfo::PrintErr("[%s]TrafficData::SelectTourTrans, m_TrafficMap left 2 gather no traf key: %s", basePlan->m_qParam.log.c_str(), left2gather.c_str());
					return 0;
				}
			}
			if (!trafficMap.count(start2left)) {
				MJ::PrintInfo::PrintErr("[%s]TrafficData::SelectTourTrans, m_TrafficMap start 2 left no traf key: %s", basePlan->m_qParam.log.c_str(), start2left.c_str());
				return 0;
			}
			if (!trafficMap.count(gather2end)) {
				MJ::PrintInfo::PrintErr("[%s]TrafficData::SelectTourTrans, m_TrafficMap gather 2 end no traf key: %s", basePlan->m_qParam.log.c_str(), gather2end.c_str());
				return 0;
			}
			if (trafficMap.count(start2endTrafKey)) {
				delete trafficMap[start2endTrafKey];
				trafficMap.erase(start2endTrafKey); //先删除原来的交通 修改为多段组合
			}
			AddTrafficItem(basePlan, start2endTrafKey, start2left);
			if (left2gather != "") AddTrafficItem(basePlan, start2endTrafKey, left2gather);
			AddTrafficItem(basePlan, start2endTrafKey, gather2end);
		}
	}

	return 0;
}
//将addTrafkey对应的trafficItem 添加到 trafKey的list中
//添加多段交通
int TrafficData::AddTrafficItem (BasePlan* basePlan, const std::string trafKey, const std::string addTrafKey) {
	std::tr1::unordered_map<std::string, TrafficItem*>& trafficMap = basePlan->m_TrafficMap;
	if(!trafficMap.count(addTrafKey)) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::AddTrafficItem, m_trafficMap no traf key: %s", basePlan->m_qParam.log.c_str(), addTrafKey.c_str());
		return 0;
	}
	TrafficItem* addItem = trafficMap[addTrafKey];
	if(!trafficMap.count(trafKey)) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::AddTrafficItem, m_trafficMap no traf key: %s, and add new traffic item", basePlan->m_qParam.log.c_str(), trafKey.c_str());
		TrafficItem* traffic = new TrafficItem;
		std::vector<std::string> itemList;
		ToolFunc::sepString(trafKey, m_trafKeySplit, itemList);
		if (itemList.size() < 2) {
			delete traffic;
			traffic = NULL;
			return 0;
		}
		traffic->m_startP = itemList[0];
		traffic->m_stopP = itemList[1];
		trafficMap[trafKey] = traffic;
	}

	if (chooseDbg & 2) std::cerr << "trafKey mid: " << trafficMap[trafKey]->_mid << " time: " << trafficMap[trafKey]->_time << " dist: " << trafficMap[trafKey]->_dist << std::endl
		<< "add mid: " << addItem->_mid << " time: " << addItem->_time << " dist: " << addItem->_dist << std::endl;
	trafficMap[trafKey]->m_trafficItemList.push_back(addItem);
	trafficMap[trafKey]->_time += addItem->_time;
	trafficMap[trafKey]->_dist += addItem->_dist;
	trafficMap[trafKey]->m_realDist += addItem->m_realDist;
	return 0;
}
//shyy end
// 按pair获取交通
// needReal参数控制是否请求实时
int TrafficData::GetTrafPair(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::string& query, int& timeCost) {
	int trafPrefer = basePlan->m_trafPrefer;
	std::tr1::unordered_map<std::string, TrafficItem*>& trafMap = basePlan->m_TrafficMap;
	bool isRealTraf = basePlan->m_useRealTraf;

	if (pairSet.empty()) return 0;

	if (basePlan->m_customTrafMap.size()) {
		for (std::tr1::unordered_set<std::string>::const_iterator it = pairSet.begin(); it != pairSet.end(); ++it) {
			std::string::size_type pos  = it->rfind('_');
			std::string key = it->substr(0, pos);
			const TrafficItem* customTrafItem = basePlan->GetCustomTraffic(key);
			if (customTrafItem) {
				TrafficItem* trafItem = new TrafficItem();
				trafItem->Copy(customTrafItem);
				if (trafMap.find(key) == trafMap.end()) {
					trafMap[key] = trafItem;
				} else {
					delete trafItem;
				}
			}
		}
		if (pairSet.size() == trafMap.size()) return 0;
	}
	std::tr1::unordered_map<std::string, Json::Value> jTrafMap;
	int carRent = (trafPrefer == TRAF_PREFER_TAXI) ? 1 : 0;
	int ret = 0;
	if (basePlan->m_useStaticTraf) {
		ret = GetTrafStaticReal(basePlan, pairSet, needReal, jTrafMap, query, timeCost, carRent);
	} else {
		ret = GetTraf8003(basePlan, pairSet, needReal, jTrafMap, query, timeCost, carRent);
	}
	if (ret != 0) return 1;

	for (std::tr1::unordered_map<std::string, Json::Value>::iterator it = jTrafMap.begin(); it != jTrafMap.end(); ++it) {
		std::string jKey = it->first;
		std::string trafKey = jKey;
		std::vector<std::string> itemList;
		ToolFunc::sepString(jKey, m_trafKeySplit, itemList);
		if (itemList.size() == 2 || itemList.size() > 2 && itemList[2].empty()) {
			trafKey = itemList[0] + m_trafKeySplit + itemList[1];
		}
		std::string trafKeyNoDate = trafKey;
		ToolFunc::sepString(trafKey, m_trafKeySplit, itemList);
		if (itemList.size() > 2) {
			trafKeyNoDate = itemList[0] + m_trafKeySplit + itemList[1];
		}
		Json::Value& jValue = it->second;
		if (trafMap.find(trafKey) != trafMap.end() || (!basePlan->m_customTrafMap.empty() && basePlan->GetCustomTraffic(trafKeyNoDate))) continue;

		TrafficItem* hitTrafItem = new TrafficItem;
		ret = ChooseTraf(basePlan, trafKey, jValue, trafPrefer, isRealTraf, hitTrafItem);
		if (ret != 0) {
			delete hitTrafItem;
			continue;
		}
		if (hitTrafItem->m_stat == TRAF_STAT_REAL_TIME) {
			if (trafMap.find(trafKey) == trafMap.end()) {
				trafMap[trafKey] = hitTrafItem;
			} else {
				delete hitTrafItem;
			}
		} else {
			if (trafMap.find(trafKeyNoDate) == trafMap.end()) {
				trafMap[trafKeyNoDate] = hitTrafItem;
			} else {
				delete hitTrafItem;
			}
		}
	}
	SetTourTraf8003(basePlan, pairSet);
	for (auto it = trafMap.begin(); it != trafMap.end(); it ++) {
		if (it->second->_time > 3600 * 6) {
			MJ::PrintInfo::PrintErr("TrafficData::GetTrafID %s traffic time > 6h", it->first.c_str());
			it->second->_time = 3600*6;
			//return 1;
		}
	}
	return 0;
}

// 某个key有多种交通 选择最优
int TrafficData::ChooseTraf(BasePlan* basePlan, const std::string& trafKey, Json::Value& jValue, int trafPrefer, bool isRealTraf, TrafficItem* hitTrafItem) {
	int ret = 0;
	std::vector<std::string> itemList;
	ToolFunc::sepString(trafKey, m_trafKeySplit, itemList);
	if (itemList.size() < 2) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::ChooseTraf, bad traf key: %s", basePlan->m_qParam.log.c_str(), trafKey.c_str());
		return 1;
	}
	std::string startP = itemList[0];
	std::string stopP = itemList[1];
	std::string date = "";
	if (itemList.size() == 3) date = itemList[2];
	if (date != "" and basePlan->m_date2trafPrefer.count(date)) {
		trafPrefer = basePlan->m_date2trafPrefer[date];
	} else if (date == "") {
		trafPrefer = basePlan->m_trafPrefer;
	} else {
		trafPrefer = TRAF_PREFER_AI;
	}

	std::tr1::unordered_map<std::string, const TrafficItem*> midMap;
	Json::Value& jItemList = jValue;
	for (int i = 0; i < jItemList.size(); ++i) {
		Json::Value& jItem = jItemList[i];
		std::string trafInfo = jItem.asString();
		std::vector<std::string> attrList;
		ToolFunc::sepString(trafInfo, "|", attrList);
		std::string mid = attrList[1];
		if (midMap.find(mid) != midMap.end()) continue;
		int trafStat = isRealTraf ? TRAF_STAT_REAL_TIME : TRAF_STAT_STATIC;
		TrafficItem* trafItem = new TrafficItem;
		trafItem->_mode = atoi(attrList[0].c_str());
		trafItem->_mid = mid;
		trafItem->m_realDist = atoi(attrList[2].c_str());
		const LYPlace* placeA = basePlan->GetLYPlace(startP);
		const LYPlace* placeB = basePlan->GetLYPlace(stopP);
		if (!placeA || !placeB) {
			delete trafItem;
			continue;
		}
		trafItem->_dist = LYConstData::CaluateSphereDist(placeA, placeB);
		//		trafItem->_dist = trafItem->m_realDist;
		trafItem->_time = (atoi(attrList[3].c_str())/60) * 60;
		trafItem->m_startP = startP;
		trafItem->m_stopP = stopP;
		trafItem->m_stat = trafStat;
		if (attrList.size() >= 6) {
			trafItem->m_order = atoi(attrList[5].c_str());
		}
		if (midMap.find(trafItem->_mid) == midMap.end()) {
			midMap[trafItem->_mid] = trafItem;
		} else {
			delete trafItem;
		}
	}

	std::string selectMid = "";
	std::string trafKeyNoDate = startP + m_trafKeySplit + stopP;
	if (chooseDbg & 1) { std::cerr << "jjj traf key " << trafKeyNoDate << std::endl; }
	if (m_whitePreferMap.find(trafKeyNoDate) != m_whitePreferMap.end() && (trafPrefer & (trafPrefer - 1))) {
		int whitePrefer = m_whitePreferMap[trafKeyNoDate];
		ChooseTraf(midMap, whitePrefer, selectMid);
	} else {
		ChooseTraf(midMap, trafPrefer, selectMid);
	}

	std::tr1::unordered_map<std::string, const TrafficItem*>::iterator ii = midMap.find(selectMid);
	if (ii != midMap.end()) {
		hitTrafItem->Copy(ii->second);
		ret = 0;
	} else {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::ChooseTraf, miss mid: %s", basePlan->m_qParam.log.c_str(), selectMid.c_str());
		ret = 1;
	}
	for (ii = midMap.begin(); ii != midMap.end(); ++ii) {
		delete ii->second;
	}
	midMap.clear();
	return ret;
}
// 选择一种合适的交通方式
int TrafficData::ChooseTraf(std::tr1::unordered_map<std::string, const TrafficItem*>& midMap, int trafPrefer, std::string& selectMid) {
	selectMid.clear();
	if (midMap.empty()) {
		MJ::PrintInfo::PrintErr("TrafficData::ChooseTraf, midMap is empty!!");
		return 1;
	}

	const TrafficItem* bestWalk = NULL;
	const TrafficItem* bestBus = NULL;
	const TrafficItem* bestTaxi = NULL;
	for (std::tr1::unordered_map<std::string, const TrafficItem*>::iterator it = midMap.begin(); it != midMap.end(); ++it) {
		std::string mid = it->first;
		const TrafficItem* trafItem = it->second;
		if (chooseDbg & 1) { MJ::PrintInfo::PrintDbg("mode: %d, dist:\t%d, time:\t%d, order: %d, mid: %s", trafItem->_mode, trafItem->_dist, trafItem->_time, trafItem->m_order, trafItem->_mid.c_str()); }
		if (trafItem->_mode == TRAF_MODE_TAXI) {
			if (!bestTaxi || TrafDistCmp()(trafItem, bestTaxi)) {
				bestTaxi = trafItem;
			}
		} else if (trafItem->_mode == TRAF_MODE_WALKING) {
			if (!bestWalk || TrafDistCmp()(trafItem, bestWalk)) {
				//大于三小时一定不选步行
				if (trafItem->_time < 3600*3)
					bestWalk = trafItem;
			}
		} else if (trafItem->_mode == TRAF_MODE_BUS) {
			if (!bestBus || TrafDistCmp()(trafItem, bestBus)) {
				bestBus = trafItem;
			}
		}
	}

	const TrafficItem* bestTrafItem = NULL;
	if (trafPrefer == TRAF_PREFER_WALKING) {
		// 选择步行＋公交＋打车：<1km步行，>1km优先公交，无公交方案, >2km打车,<2km仍然步行
		if (bestWalk && bestWalk->m_realDist <= kWalkDistLimit) {
			bestTrafItem = bestWalk;
		} else {
			if (bestBus) {
				bestTrafItem = bestBus;
			} else if (bestWalk && bestWalk->m_realDist <= kWalkBusTaxiDistUpperLimit) {
				bestTrafItem = bestWalk;
			} else if (bestTaxi) {
				bestTrafItem = bestTaxi;
			} else if (bestWalk) {
				bestTrafItem = bestWalk;
			}
		}
	} else if (trafPrefer == TRAF_PREFER_AI) {
		// 选择步行＋公交＋打车：<1km步行，>1km优先公交，无公交方案给打车
		if (bestWalk && bestWalk->m_realDist <= kWalkBusTaxiDistLimit) {
			bestTrafItem = bestWalk;
		} else {
			if (bestBus) {
				bestTrafItem = bestBus;
			} else if (bestTaxi) {
				bestTrafItem = bestTaxi;
			} else if (bestWalk) {
				bestTrafItem = bestWalk;
			}
		}
	} else if (trafPrefer == TRAF_PREFER_TAXI) {
		// 选择包车, <1km仍然步行
		if (bestWalk && bestWalk->m_realDist <= kWalkTaxiDistLimit) {
			bestTrafItem = bestWalk;
		} else {
			if (bestTaxi && bestTaxi->m_realDist >= kTaxiUseLimit) {
				bestTrafItem = bestTaxi;
			} else {
				if (bestWalk) {
					bestTrafItem = bestWalk;
				}
			}
		}
		if (!bestTrafItem && bestTaxi) {
			bestTrafItem = bestTaxi;
		}
		if(!bestTrafItem) {
			MJ::PrintInfo::PrintErr("no taxi data to choose please check ");
		}
	}
	if (bestTrafItem) {
		selectMid = bestTrafItem->_mid;
		if (chooseDbg & 1) { MJ::PrintInfo::PrintDbg("choose mode: %d, dist:\t%d, time:\t%d, order: %d, mid: %s", bestTrafItem->_mode, bestTrafItem->_dist, bestTrafItem->_time, bestTrafItem->m_order, bestTrafItem->_mid.c_str()); }
		return 0;
	} else {
		if (chooseDbg & 1) { MJ::PrintInfo::PrintDbg("choose null"); }
		return 1;
	}
	return 0;
}


//8009获取交通摘要 真实交通补全
int TrafficData::GetTrafficSummary(BasePlan* basePlan, std::vector<TrafficItem*>& trafficList, std::string& query, int& timeCost) {
	std::tr1::unordered_set<std::string> midSet;  //交通_mid 打8009
	std::tr1::unordered_map<std::string, TrafficItem*> midMap;  //包含tour的交通,只将start-tour,tour-end放入midSet
	for (int i = 0; i < trafficList.size(); i ++) {
		if (trafficList[i]->m_trafficItemList.size() > 0) {
			std::vector<const TrafficItem* > trafItem = trafficList[i]->m_trafficItemList;
			for (int j = 0; j < trafItem.size(); ++j) {
				if(midSet.find(trafItem[j]->_mid) == midSet.end()) {
					midSet.insert(trafItem[j]->_mid);
				}
				midMap.insert(make_pair(trafItem[j]->_mid, trafficList[i]));
			}
		}
		else {
			if(midSet.find(trafficList[i]->_mid) == midSet.end()) {
				midSet.insert(trafficList[i]->_mid);
			}
		}
	}
	std::tr1::unordered_map<std::string, Json::Value> trafficSummaryList;
	std::tr1::unordered_map<std::string, Json::Value>::iterator it;
	std::tr1::unordered_map<std::string, TrafficItem*>::iterator ii;
	int ret = GetTrafficFromServer8009(basePlan, midSet, trafficSummaryList, query, timeCost);
	if (ret) return 1;
	for (ii = midMap.begin(); ii != midMap.end(); ii++) {
		it = trafficSummaryList.find(ii->first);
		if (it != trafficSummaryList.end()){
			Json::Value& jItem = it->second;
			TrafficItem* trafficItem = ii->second;
			if (jItem.isMember("warnings") && jItem["warnings"].isArray()) 
				trafficItem->m_warnings = jItem["warnings"];
			if (jItem.isMember("info") && jItem["info"].isArray()) {
				for (int jj = 0; jj < jItem["info"].size(); jj++) {
					Json::Value Info = jItem["info"][jj];
					Json::Value tInfo;
					tInfo["type"] = Info["mode"];
					tInfo["dur"] = Info["time"];
					tInfo["icon"] = Info["icon"];
					tInfo["bg_color"] = Info["bg_color"];
					tInfo["font_color"] = "";
					tInfo["dist"] = Info["dist"];
					tInfo["desc"] = Info["no"];
					trafficItem->m_transfers.append(tInfo);
				}
			}
		}
	}
	return 0;
}

int TrafficData::GetTraf8002(BasePlan* basePlan, const std::vector<std::string>& idList, int carRent, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost) {
	int ret = 0;
	ret = MakeQuery8002(basePlan, carRent, idList, query);
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8002, MakeQuery8002 Error", basePlan->m_qParam.log.c_str());
		return 1;
	}

	Json::Value jRet;
	MJ::MyTimer t;
	std::string urlPath("");
	std::string type("8002");
	ret = GetHttpData(basePlan->m_qParam, RouteConfig::traffic_server_addr, RouteConfig::traffic_server_port, RouteConfig::traffic_server_timeout, MJ::Get_Http_11 , urlPath, type, query, jRet);
	basePlan->m_cost.m_traffic8002 = std::max(t.cost(), basePlan->m_cost.m_traffic8002);
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8002, Socket Error", basePlan->m_qParam.log.c_str());
		return ret;
	}

	Json::Value& jInfo = jRet["data"]["info"];
	Json::Value::Members memList = jInfo.getMemberNames();
	for (Json::Value::Members::iterator it = memList.begin(); it != memList.end(); ++it) {
		std::string trafKey = *it;
		std::vector<std::string> itemList;
		ToolFunc::sepString(trafKey, m_trafKeySplit, itemList);
		if (itemList.size() != 2) {
			MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8002, trafKey split error: %s", basePlan->m_qParam.log.c_str(), trafKey.c_str());
			continue;
		}
		Json::Value& jValue = jInfo[trafKey];
		jTrafMap[trafKey] = jValue;
	}
	return 0;
}

int TrafficData::GetTraf8003(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost, int carRent) {
	int ret = 0;
	ret = MakeQuery8003(basePlan, needReal, pairSet, query, carRent);
	Json::Value jRet;
	if (ret == 0) {
		MJ::MyTimer t;
		std::string urlPath("");
		std::string type("8003");
		ret = GetHttpData(basePlan->m_qParam, RouteConfig::traffic_server_addr, RouteConfig::traffic_server_port, RouteConfig::traffic_server_timeout, MJ::Get_Http_11 , urlPath, type, query, jRet);
		basePlan->m_cost.m_traffic8003 = std::max(t.cost(), basePlan->m_cost.m_traffic8003);
		if (ret != 0) {
			MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8003, Socket Error", basePlan->m_qParam.log.c_str());
			return 1;
		}
		Json::Reader jr(Json::Features::strictMode());
		Json::Value jQuery;
		if (jr.parse(query,jQuery)) {
			jQuery["qid"] = basePlan->m_qParam.qid;
			Json::Value stat;
			stat["query"] = jQuery;
			stat["resp"] = jRet;
			Json::FastWriter fastWriter;
			MJ::PrintInfo::PrintErr("[8003STAT] %s",fastWriter.write(stat).c_str());
		}
		//return 1;
	}
	else
	{
		MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8003, MakeQuery8003 Error", basePlan->m_qParam.log.c_str());
	}

	Json::Value& jInfo = jRet["data"]["info"];
	Json::Value::Members memList = jInfo.getMemberNames();
	for (Json::Value::Members::iterator it = memList.begin(); it != memList.end(); ++it) {
		std::string trafKey = *it;
		std::vector<std::string> itemList;
		ToolFunc::sepString(trafKey, m_trafKeySplit, itemList);
		if (itemList.size() != 3) {
			MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTraf8003, trafKey split error: %s", basePlan->m_qParam.log.c_str(), trafKey.c_str());
			continue;
		}
		Json::Value& jValue = jInfo[trafKey];
		jTrafMap[trafKey] = jValue;
	}
	return 0;
}

int TrafficData::GetTrafficFromServer8009(BasePlan* basePlan, std::tr1::unordered_set<std::string>& midSet, std::tr1::unordered_map<std::string, Json::Value>& trafficSummaryList, std::string& query, int& timeCost) {
	int ret = 0;
	query = MakeQuery8009(basePlan, midSet);
	Json::Value jRet;
	int tCost = 0;
	MJ::MyTimer t;
	std::string urlPath("");
	std::string type("8009");
	ret = GetHttpData(basePlan->m_qParam, RouteConfig::traffic_server_addr, RouteConfig::traffic_server_port, RouteConfig::traffic_server_timeout, MJ::Get_Http_11 , urlPath, type, query, jRet);
	basePlan->m_cost.m_traffic8009 = std::max(t.cost(), basePlan->m_cost.m_traffic8009);
	timeCost += tCost;
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("[%s]TrafficData::GetTrafficFromServer8009, Error", basePlan->m_qParam.log.c_str());
		return ret;
	}
	for (int i = 0; i < jRet["data"]["list"].size(); i ++) {
		Json::Value& trafficValue = jRet["data"]["list"][i];
		trafficSummaryList[trafficValue["mid"].asString()] = trafficValue;
	}
	return 0;

}
bool TrafficData::IsReal(const std::string& mid) {
	if (mid == _blank_traffic_item->_mid
			|| mid == _self_traffic_item->_mid
			|| mid == _fake_traffic_item->_mid
			|| mid == _fake_hotel_traffic_item->_mid) {
		return false;
	} else {
		return true;
	}
}

int TrafficData::MakeQuery8002(BasePlan* basePlan, int carRent, const std::vector<std::string>& idList, std::string& query) {
	Json::Value jPoiList;
	jPoiList.resize(0);
	//add by shyy
	std::tr1::unordered_set<std::string> PoiSet;
	for (int i = 0; i < idList.size(); ++i) {
		PoiSet.insert(idList[i]);
	}
	for (int i = 0; i < idList.size(); ++i) {
		const LYPlace* place = basePlan->GetLYPlace(idList[i]);
		if(place == NULL) {
			continue;
		}
		if (place->_t & LY_PLACE_TYPE_TOURALL) {
			const Tour* tour = dynamic_cast<const Tour*>(place);
			if (tour == NULL) {
				continue;
			}
			basePlan->AddTourJieSongPoi(tour, PoiSet);
		}
	}
	for (std::tr1::unordered_set<std::string>::iterator it = PoiSet.begin(); it != PoiSet.end(); it ++) {
		jPoiList.append(*it);
	}
	if (jPoiList.size() == 0) {
		MJ::PrintInfo::PrintErr("TrafficData::MakeQuerry, 8002 无点");
		return 1;
	}
	//shyy end
	Json::Value jCustomPoisList;
	jCustomPoisList.resize(0);
	std::tr1::unordered_set<std::string> customId;
	for (int i = 0; i < idList.size(); ++i) {
		const LYPlace* place = basePlan->GetLYPlace(idList[i]);
		if (place && (place->m_custom || place->_t & LY_PLACE_TYPE_TOURALL) && customId.find(place->_ID) == customId.end()) {
			Json::Value jCustomPoiInfo;
			jCustomPoiInfo["id"] = place->_ID;
			jCustomPoiInfo["name"] = place->_name;
			jCustomPoiInfo["map_info"] = place->_poi;
			jCustomPoiInfo["custom"] = place->m_custom;
			jCustomPoiInfo["mode"] = LYPlaceType2TrafPoiType(place->_t);
			jCustomPoiInfo["city_id"] = basePlan->m_City->_ID;
			jCustomPoisList.append(jCustomPoiInfo);
			customId.insert(place->_ID);
		}
	}
	Json::Value req;
	req["pois"] = jPoiList;
	req["custom_pois"] = jCustomPoisList;
	req["car_rental"] = carRent;

	Json::FastWriter fastWriter;
	char buff[1024 * 1024] = {};
	snprintf(buff, sizeof(buff), "%s", fastWriter.write(req).c_str());
	query.assign(buff);
	return 0;
}
//add by shyy
int TrafficData::SetCustomPois(BasePlan* basePlan, const LYPlace* pPlace, Json::Value& jCustomPoisList, std::tr1::unordered_set<std::string>& customId) {
	Json::Value jCustomPoiInfo;
	jCustomPoiInfo["id"] = pPlace->_ID;
	jCustomPoiInfo["name"] = pPlace->_name;
	jCustomPoiInfo["map_info"] = pPlace->_poi;
	jCustomPoiInfo["custom"] = pPlace->m_custom;
	jCustomPoiInfo["mode"] = LYPlaceType2TrafPoiType(pPlace->_t);
	jCustomPoiInfo["city_id"] = basePlan->m_City->_ID;
	jCustomPoisList.append(jCustomPoiInfo);
	customId.insert(pPlace->_ID);
	return 0;
}
int TrafficData::InsertjDataList(BasePlan* basePlan, const std::string& start, const std::string& dest, const std::string& time, Json::Value& jData, std::tr1::unordered_set<std::string>& jDataList) {
	if (start == dest) {
		TrafficItem* trafItem = new TrafficItem();
		trafItem->Copy(_self_traffic_item);
		std::string trafKey = start + m_trafKeySplit + dest;
		basePlan->m_TrafficMap[trafKey] = trafItem;
		return 0;
	}
	Json::Value jItem;
	jItem["start_id"] = start;
	jItem["dest_id"] = dest;
	jItem["time"] = time;
	std::string jItemString = jItem["start_id"].asString() + "_" + jItem["dest_id"].asString() + "_" + jItem["time"].asString();
	if (jDataList.find(jItemString) == jDataList.end()) {
		jData.append(jItem);
		jDataList.insert(jItemString);
	}
	return 0;
}
int TrafficData::SetTrafficPair(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, Json::Value& jData, Json::Value& jCustomPoisList) {
	std::tr1::unordered_set<std::string> customIds;
	std::tr1::unordered_set<std::string> jDataList;
	for (std::tr1::unordered_set<std::string>::const_iterator ii = pairSet.begin(); ii != pairSet.end(); ++ii) {
		const std::string& pairKey = *ii;
		std::vector<std::string> itemList;
		ToolFunc::sepString(pairKey, m_trafKeySplit, itemList);
		if (itemList[0] == itemList[1]) continue;
		std::string time = "";
		if (itemList.size() > 2) {
			time = itemList[2];
		}
		const LYPlace* place_0 = basePlan->GetLYPlace(itemList[0]);
		const LYPlace* place_1 = basePlan->GetLYPlace(itemList[1]);

		if (place_0 == NULL || place_1 == NULL) continue;
		if (place_0 == basePlan->m_arvNullPlace || place_1 == basePlan->m_deptNullPlace) continue;

		//判断place_0,place_1是否为自定义点
		if (place_0 && (place_0->m_custom||place_0->_t & LY_PLACE_TYPE_TOURALL) && !customIds.count(place_0->_ID)) {
			SetCustomPois(basePlan, place_0, jCustomPoisList, customIds);
		}
		if (place_1 && (place_1->m_custom||place_1->_t & LY_PLACE_TYPE_TOURALL) && !customIds.count(place_1->_ID)) {
			SetCustomPois(basePlan, place_1, jCustomPoisList, customIds);
		}
		//对于玩乐是否要加 poi 直接到 tour ??? ！！！
		InsertjDataList(basePlan, itemList[0], itemList[1], time, jData, jDataList);

		//对于玩乐 选择接送点 请求交通
		if ((place_0->_t & LY_PLACE_TYPE_TOURALL) or (place_1->_t & LY_PLACE_TYPE_TOURALL)) {
			std::vector<const LYPlace*> poiList;
			basePlan->SelectTransPois(place_0, place_1, poiList);
			if (poiList.size()) {
				if (poiList.front() and poiList.front()->m_custom) {
					SetCustomPois(basePlan, poiList.front(), jCustomPoisList, customIds);
				}
				InsertjDataList(basePlan, itemList[0], poiList.front()->_ID, time, jData, jDataList);
				InsertjDataList(basePlan, poiList.back()->_ID, itemList[1], time, jData, jDataList);
				if (poiList.size() == 2) {
					if (poiList.back() and poiList.back()->m_custom) {
						SetCustomPois(basePlan, poiList.back(), jCustomPoisList, customIds);
					}
					InsertjDataList(basePlan, poiList.front()->_ID, poiList.back()->_ID, time, jData, jDataList);
				}
			}
		}
	}
	if (jDataList.size() ==0) {
		MJ::PrintInfo::PrintErr("TrafficData::SetTrafficPair, jDataList size: 0");
		return 1;
	}
	return 0;
}

//shyy end
int TrafficData::MakeQuery8003(BasePlan* basePlan, int needReal,const std::tr1::unordered_set<std::string>& pairSet, std::string& query, int carRent) {
	Json::Value jData;
	jData.resize(0);
	Json::Value jCustomPoisList;
	jCustomPoisList.resize(0);
	int ret = SetTrafficPair(basePlan, pairSet, jData, jCustomPoisList);
	if (ret) {
		return ret;
	}

	Json::Value req;
	req["data"] = jData;
	req["needReal"] = needReal;
	req["custom_pois"] = jCustomPoisList;
	req["car_rental"] = 1;
	Json::FastWriter fastWriter;
	char buff[1024 * 1024] = {};
	snprintf(buff, sizeof(buff), "%s", fastWriter.write(req).c_str());
	query.assign(buff);
	return 0;
}

std::string TrafficData::MakeQuery8009(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& midSet) {
	Json::Value req;
	Json::Value jCustomPoisList;
	jCustomPoisList.resize(0);
	Json::Value mids;
	mids.resize(0);
	std::tr1::unordered_set<std::string> customId;
	for (std::tr1::unordered_set<std::string>::const_iterator ii = midSet.begin(); ii != midSet.end(); ++ii) {
		std::vector<std::string> itemList;
		ToolFunc::sepString((*ii), m_trafKeySplit, itemList);
		if (itemList.size() < 2) {
			continue;
		}
		const LYPlace* place = basePlan->GetLYPlace(itemList[0]);
		if (place && (place->_t & LY_PLACE_TYPE_TOURALL || place->m_custom) && customId.find(place->_ID) == customId.end()) {
			Json::Value jCustomPoiInfo;
			jCustomPoiInfo["id"] = place->_ID;
			jCustomPoiInfo["name"] = place->_name;
			jCustomPoiInfo["map_info"] = place->_poi;
			jCustomPoiInfo["custom"] = place->m_custom;
			jCustomPoiInfo["mode"] = LYPlaceType2TrafPoiType(place->_t);
			jCustomPoiInfo["city_id"] = basePlan->m_City->_ID;
			jCustomPoisList.append(jCustomPoiInfo);
			customId.insert(place->_ID);
		}
		place = basePlan->GetLYPlace(itemList[1]);
		if (place && (place->_t & LY_PLACE_TYPE_TOURALL || place->m_custom) && customId.find(place->_ID) == customId.end()) {
			Json::Value jCustomPoiInfo;
			jCustomPoiInfo["id"] = place->_ID;
			jCustomPoiInfo["name"] = place->_name;
			jCustomPoiInfo["map_info"] = place->_poi;
			jCustomPoiInfo["custom"] = place->m_custom;
			jCustomPoiInfo["mode"] = LYPlaceType2TrafPoiType(place->_t);
			jCustomPoiInfo["city_id"] = basePlan->m_City->_ID;
			jCustomPoisList.append(jCustomPoiInfo);
			customId.insert(place->_ID);
		}
		mids.append(*ii);
	}
	req["mids"] = mids;
	req["custom_pois"] = jCustomPoisList;
	Json::FastWriter fastWriter;
	char buff[1024 * 1024] = {};
	snprintf(buff, sizeof(buff), "%s", fastWriter.write(req).c_str());
	std::string query = std::string(buff);
	return query;
}
//交通白名单 特定key必选 特定偏好交通
bool TrafficData::LoadTrafficWhite(const std::string& data_path, std::vector<std::string>& traffic_white_file_list) {
	if (traffic_white_file_list.size() < 0) {
		MJ::PrintInfo::PrintErr("TrafficData::LoadTrafficWhite, file list size: 0");
		return false;
	}

	MJ::PrintInfo::PrintLog("TrafficData::LoadTrafficWhite, loading ...");
	for (int i = 0; i < traffic_white_file_list.size(); ++i) {
		std::string file_name = data_path + "/" + traffic_white_file_list[i];
		std::ifstream fin(file_name.c_str());
		if (!fin) {
			MJ::PrintInfo::PrintErr("TrafficData::LoadTrafficWhite, load file error: %s", file_name.c_str());
			return false;
		}

		std::string line;
		while (std::getline(fin, line)) {
			if (line.size() > 1 && line.substr(0, 1) == "#" )
				continue;
			std::vector<std::string> tmp_list;
			ToolFunc::sepString(line, ":", tmp_list);
			if (tmp_list.size() != 2) continue;
			std::string trafKey = tmp_list[0];
			int trafMode = atoi(tmp_list[1].c_str());
			int trafPrefer = TRAF_PREFER_AI;
			if (trafMode == TRAF_MODE_TAXI) {
				trafPrefer = TRAF_PREFER_TAXI;
			} else if (trafMode == TRAF_MODE_WALKING) {
				trafPrefer = TRAF_PREFER_WALKING;
			} else if (trafMode == TRAF_MODE_BUS) {
				trafPrefer = TRAF_PREFER_BUS;
			}
			tmp_list.clear();
			ToolFunc::sepString(trafKey, "|", tmp_list);
			if (tmp_list.size() != 2) continue;

			trafKey = tmp_list.front() + m_trafKeySplit + tmp_list.back();
			m_whitePreferMap[trafKey] = trafPrefer;
			trafKey = tmp_list.back() + m_trafKeySplit + tmp_list.front();
			m_whitePreferMap[trafKey] = trafPrefer;
		}
		fin.close();
	}
	MJ::PrintInfo::PrintLog("TrafficData::LoadTrafficWhite, load num: %d", m_whitePreferMap.size());
	return true;
}

int TrafficData::LYPlaceType2TrafPoiType(int type) {
	switch(type) {
		case LY_PLACE_TYPE_CITY: return 1;
		case LY_PLACE_TYPE_VIEW: return 2;
		case LY_PLACE_TYPE_HOTEL: return 3;
		case LY_PLACE_TYPE_RESTAURANT: return 4;
		case LY_PLACE_TYPE_AIRPORT: return 5;
		case LY_PLACE_TYPE_STATION: return 6;
		case LY_PLACE_TYPE_CAR_STORE: return 7;
		case LY_PLACE_TYPE_BUS_STATION: return 8;
		case LY_PLACE_TYPE_SHOP: return 9;
		case LY_PLACE_TYPE_SAIL_STATION: return 10;
		default : return 0;
	}
}

//打接口
int TrafficData::GetHttpData(const QueryParam& qParam, const std::string& addr, const int& port, const int& timeout, MJ::MJHttpMethod mjMethod , const std::string& urlPath, const std::string& type, const std::string& query, Json::Value& resp) {

	MJ::PrintInfo::PrintLog("[%s]TrafficData::GetHttpData	%s", qParam.log.c_str(), query.c_str());
	MJ::MJSocketSession mjSocketSession(addr, port, timeout, mjMethod, urlPath, type, qParam.qid, query, "", qParam.refer_id, qParam.cur_id, qParam.ptid, qParam.lang, qParam.ccy,qParam.csuid,qParam.uid);
	MJ::SocketClientEpoll::doHttpRequest(&mjSocketSession);
	if (mjSocketSession._response_len <= 0) {
		MJ::PrintInfo::PrintLog("[%s]Json::Parser resp length = 0", qParam.log.c_str());
		return 1;
	}
	Json::Reader jr(Json::Features::strictMode());
	MJ::PrintInfo::PrintLog("[%s]TrafficData::GetHttpData	%s", qParam.log.c_str(), mjSocketSession._response.c_str());
	if ( !jr.parse(mjSocketSession._response, resp) ) {
		MJ::PrintInfo::PrintLog("[%s]Json::Parser Failed %s", qParam.log.c_str(), mjSocketSession._response.c_str());
		return 1;
	}
	return 0;
}

//-------------------以下为内部实现 8002 8003 功能--------------------
//静态自计算交通 8002样式
int TrafficData::GetTrafStatic(BasePlan* basePlan, const std::vector<std::string>& idList, int carRent, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost) {
	MJ::MyTimer t1;
	std::tr1::unordered_set<std::string> PoiSet;
	for (int i = 0; i < idList.size(); ++i) {
		PoiSet.insert(idList[i]);
	}
	for (int i = 0; i < idList.size(); ++i) {
		const LYPlace* place = basePlan->GetLYPlace(idList[i]);
		if (place == NULL) continue;
		if (place->_t & LY_PLACE_TYPE_TOURALL) {
			const Tour* tour = dynamic_cast<const Tour*>(place);
			if (tour == NULL) {
				continue;
			}
			basePlan->AddTourJieSongPoi(tour, PoiSet);
		}
	}
	std::tr1::unordered_set<std::string>::iterator it_i;
	std::tr1::unordered_set<std::string>::iterator it_j;
	for (it_i = PoiSet.begin(); it_i != PoiSet.end(); it_i ++) {
		for (it_j = PoiSet.begin(); it_j != PoiSet.end(); it_j ++) {
			jTrafMap[*it_i + "_" + *it_j] = GetTratItemStatic(basePlan, *it_i, *it_j);
		}
	}
	timeCost = t1.cost();
	return 0;
}

//静态自计算交通 8003样式
int TrafficData::GetTrafStaticReal(BasePlan* basePlan, const std::tr1::unordered_set<std::string>& pairSet, bool needReal, std::tr1::unordered_map<std::string, Json::Value>& jTrafMap, std::string& query, int& timeCost, int carRent) {
	MJ::MyTimer t1;
	std::tr1::unordered_set<std::string> newPairSet;
	for (std::tr1::unordered_set<std::string>::const_iterator ii = pairSet.begin(); ii != pairSet.end(); ++ii) {
		const std::string& pairKey = *ii;
		std::vector<std::string> itemList;
		ToolFunc::sepString(pairKey, m_trafKeySplit, itemList);
		if (itemList[0] == itemList[1]) continue;
		std::string time;
		if (itemList.size() > 2) {
			time = itemList[2];
		} else {
			time = "";
		}
		if(newPairSet.find(itemList[0] + m_trafKeySplit + itemList[1]) == newPairSet.end()) {
			newPairSet.insert(itemList[0] + m_trafKeySplit + itemList[1]);
		}
		const LYPlace* place_0 = basePlan->GetLYPlace(itemList[0]);
		const LYPlace* place_1 = basePlan->GetLYPlace(itemList[1]);

		if (place_0 == NULL || place_1 == NULL) continue;
		//对于玩乐 选择接送点 请求交通
		if ((place_0->_t & LY_PLACE_TYPE_TOURALL) or (place_1->_t & LY_PLACE_TYPE_TOURALL)) {
			std::vector<const LYPlace*> poiList;
			basePlan->SelectTransPois(place_0, place_1, poiList);
			if (poiList.size()) {
				std::string trafKey = place_0->_ID + m_trafKeySplit + poiList.front()->_ID;
				newPairSet.insert(trafKey);
				trafKey = poiList.back()->_ID + m_trafKeySplit + place_1->_ID;
				newPairSet.insert(trafKey);
				if (poiList.size() == 2) {
					trafKey = poiList.front()->_ID + m_trafKeySplit + poiList.back()->_ID;
					newPairSet.insert(trafKey);
				}
			}
		}
	}
	for (std::tr1::unordered_set<std::string>::const_iterator it = newPairSet.begin(); it != newPairSet.end(); it ++) {
		std::string key = (*it);
		std::vector<std::string> itemList;
		ToolFunc::sepString(key, "_", itemList);
		if (itemList.size() >= 2 && itemList[0] != "" && itemList[1] != "") {
			jTrafMap[key] = GetTratItemRealStatic(basePlan, itemList[0], itemList[1]);
		}
	}
	timeCost = t1.cost();
	return 0;
}



//静态获取placeA - placeB 交通 （8003）样式 key长度不同 
Json::Value TrafficData::GetTratItemRealStatic(BasePlan* basePlan, const std::string& idA, const std::string& idB) {
	Json::Value jTrafInfo;
	jTrafInfo = GetTratItemStatic(basePlan, idA, idB);
	for (int i = 0; i < jTrafInfo.size();i ++) {
		jTrafInfo[i] = jTrafInfo[i].asString() + "||-1";
	}
	return jTrafInfo;
}

//静态获取placeA - placeB 交通 （8002）样式
Json::Value TrafficData::GetTratItemStatic(BasePlan* basePlan, const std::string& idA, const std::string& idB) {
	Json::Value jTrafInfo;
	jTrafInfo.resize(0);
	const LYPlace* placeA = basePlan->GetLYPlace(idA);
	const LYPlace* placeB = basePlan->GetLYPlace(idB);
	if (placeA == NULL || placeB == NULL) {
		return jTrafInfo;
	}
	double dist = 0;
	dist = LYConstData::CaluateSphereDist(placeA, placeB);
	double driveSpeed = 0;
	//	if ((placeA->_t & LY_PLACE_TYPE_ARRIVE) || (placeB->_t & LY_PLACE_TYPE_ARRIVE)) {
	if (dist > 40000) {
		driveSpeed = 22;
	} else {
		driveSpeed = 11;
	}
	double busSpeed = 0;
	//	if ((placeA->_t & LY_PLACE_TYPE_ARRIVE) || (placeB->_t & LY_PLACE_TYPE_ARRIVE)) {
	if (dist > 40000) {
		busSpeed = 13.2;
	} else {
		busSpeed = 5.5;
	}
	double walkSpeed = 1.3;
	char buff[1000] = {0};
	snprintf(buff,sizeof(buff), "0|%s_%s_0_2_makeDrive|%.0lf|%.0lf", placeA->_ID.c_str(), placeB->_ID.c_str(),dist, dist / driveSpeed);
	std::string driveItem = std::string(buff);
	jTrafInfo.append(driveItem);
	snprintf(buff,sizeof(buff), "2|%s_%s_2_2_makeBus|%.0lf|%.0lf", placeA->_ID.c_str(), placeB->_ID.c_str(),dist, dist / busSpeed);
	std::string busItem = std::string(buff);
	jTrafInfo.append(busItem);
	snprintf(buff,sizeof(buff), "1|%s_%s_1_2_makeWalk|%.0lf|%.0lf", placeA->_ID.c_str(), placeB->_ID.c_str(),dist, dist / walkSpeed);
	std::string walkItem = std::string(buff);
	jTrafInfo.append(walkItem);
	return jTrafInfo;
}

