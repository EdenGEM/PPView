#include <iostream>
#include <algorithm>
#include <limits>
#include "Route/base/DataChecker.h"
#include "MJCommon.h"
#include "DFSearch.h"
#include "RouteSearch.h"
#include "BagSearch.h"
#include "MyCluster.h"

int BagSearch::DoSearch(BagPlan* bagPlan, MyThreadPool* threadPool) {
	int ret = 0;
	bool isTimeOut = false;
	MJ::MyTimer t;

	// 0 准备工作
	CalDur(bagPlan);
	GetCBP(bagPlan, threadPool);
	ret = bagPlan->DoPos();
	if (ret == 0) {
		AllocDur(bagPlan);  // 用户点过多时挤压place时长
	}
	if (ret == 0) {
		ret = GetEstPlace(bagPlan);
		ret = DataChecker::GetAvgTraf(bagPlan, bagPlan->m_futureList);
	}
	if (ret == 0) {
		ret = GetBenchMark(bagPlan);
	}
	isTimeOut = bagPlan->IsTimeOut();

	// 1 搜包
	std::vector<const SPath*> rootList;
	if (ret == 0 && !isTimeOut) {
		bagPlan->m_rootTimer.start();
		t.start();
		YANGSHU::ClusterOrganizer clusterOrganizer(bagPlan, &rootList);
		clusterOrganizer.Run();
		bagPlan->m_bagCost.m_rootSearch = t.cost();//暂时用richer的相应域代替cluster
		bagPlan->m_bagStat.m_rootNum = rootList.size();
	}
	isTimeOut = bagPlan->IsTimeOut();

	// 2 dfs顺序
	std::vector<const SPath*> dfsList;
	if (ret == 0 && !isTimeOut) {
		bagPlan->m_dfsTimer.start();
		t.start();
		ret = DFSearch::DoSearch(bagPlan, rootList, dfsList, threadPool);
		bagPlan->m_bagCost.m_dfSearch = t.cost();
		bagPlan->m_bagStat.m_dfsNum = dfsList.size();
	}
	for (int i = 0; i < rootList.size(); ++i) {
		delete rootList[i];
	}
	isTimeOut = bagPlan->IsTimeOut();

	// 3 所有branch path 做route
	if (ret == 0 && !isTimeOut) {
		bagPlan->m_routeTimer.start();
		t.start();
		ret = RouteSearch::DoSearch(bagPlan, dfsList, threadPool);
		bagPlan->m_bagCost.m_routeSearch = t.cost();
//todo show the size of bag
		fprintf(stderr, "miojiParam\t%s\tA:%.4f,C:%.4f,P1:%d,P2:%d,P3:%d,P4:%d,P5:%d,P6:%d,P7:%d\th:%d,d:%d,t:%d,c:%d,s:%.4f\thMax:%d,dMax:%d,tMax:%d\n", bagPlan->m_qParam.qid.c_str(), bagPlan->m_cbP.m_cbBagWeightA, bagPlan->m_cbP.m_cbBagThre, bagPlan->m_cbP.m_cbRootHeapLimit, bagPlan->m_cbP.m_cbRichHeapLimit, bagPlan->m_cbP.m_cbDFSHeapLimit, bagPlan->m_cbP.m_cbRootRetLimit, bagPlan->m_cbP.m_cbRichExtraDur, bagPlan->m_cbP.m_cbRichTopK, bagPlan->m_cbP.m_cbRichMissLimit, bagPlan->m_PlanList._hot, bagPlan->m_PlanList._dist, bagPlan->m_PlanList._time, bagPlan->m_PlanList._cross, bagPlan->m_PlanList._score, bagPlan->MaxHot(), bagPlan->MaxDist(), bagPlan->MaxTime());
	} else {
		fprintf(stderr, "miojiParam\t%s\tA:%.4f,C:%.4f,P1:%d,P2:%d,P3:%d,P4:%d,P5:%d,P6:%d,P7:%d\tNULL\n", bagPlan->m_qParam.qid.c_str(), bagPlan->m_cbP.m_cbBagWeightA, bagPlan->m_cbP.m_cbBagThre, bagPlan->m_cbP.m_cbRootHeapLimit, bagPlan->m_cbP.m_cbRichHeapLimit, bagPlan->m_cbP.m_cbDFSHeapLimit, bagPlan->m_cbP.m_cbRootRetLimit, bagPlan->m_cbP.m_cbRichExtraDur, bagPlan->m_cbP.m_cbRichTopK, bagPlan->m_cbP.m_cbRichMissLimit);
	}
	for (int i = 0; i < dfsList.size(); ++i) {
		delete dfsList[i];
	}
	dfsList.clear();

	if (isTimeOut) {
		MJ::PrintInfo::PrintLog("[%s]BagSearch::DoSearch, time out!!!", bagPlan->m_qParam.log.c_str());
//		bagPlan->m_error.Set(55403, "计算超时!");
		return 1;
	}
	if (bagPlan->m_PlanList.Length() <= 0) {
		MJ::PrintInfo::PrintLog("[%s]BagSearch::DoSearch, bagSearch failed!!!", bagPlan->m_qParam.log.c_str());
		return 1;
	}
	return 0;
}

int BagSearch::AllocDur(BagPlan* bagPlan) {
	std::tr1::unordered_map<std::string, int> allocDurUserMap;
	std::tr1::unordered_map<std::string, int> allocDurWaitMap;
	int sumUserAllocDur = 0;
	for (auto place : bagPlan->m_userMustPlaceSet) {
		int avgAllocDur = bagPlan->GetAvgAllocDur(place);
		sumUserAllocDur += avgAllocDur;
		std::tr1::unordered_map<std::string, int>::iterator it = allocDurUserMap.find(place->_ID);
		if (it == allocDurUserMap.end()) {
			allocDurUserMap[place->_ID] = avgAllocDur;
		}
	}
	int sumWaitAllocDur = 0;
	for (int i = 0; i < bagPlan->m_waitPlaceList.size(); ++i) {
		const LYPlace* place = bagPlan->m_waitPlaceList[i];
		int avgAllocDur = bagPlan->GetAvgAllocDur(place);
		sumWaitAllocDur += avgAllocDur;
		std::tr1::unordered_map<std::string, int>::iterator it = allocDurWaitMap.find(place->_ID);
		if (it == allocDurWaitMap.end()) {
			allocDurWaitMap[place->_ID] = avgAllocDur;
		}
	}

	double scale = 1.0;
	int availAllocDur = std::min(bagPlan->m_availDur - 3600 * 6, static_cast<int>(bagPlan->m_availDur * 0.7));
	if (sumUserAllocDur > bagPlan->m_availDur) { //紧张情况
		scale = bagPlan->m_availDur * 1.0 / sumUserAllocDur;
		for (std::tr1::unordered_map<std::string, int>::iterator it = allocDurUserMap.begin(); it != allocDurUserMap.end(); ++it) {
			bagPlan->m_allocDurMap[it->first] = it->second * scale;
		}
	} else if (sumWaitAllocDur < availAllocDur) { //必须＋可选轻松
//		scale = availAllocDur * 1.0 / sumWaitAllocDur;
		scale = 1.0;
		for (std::tr1::unordered_map<std::string, int>::iterator it = allocDurWaitMap.begin(); it != allocDurWaitMap.end(); ++it) {
			bagPlan->m_allocDurMap[it->first] = std::min(static_cast<int>(it->second * scale), 3600 * 8);
		}
	}
	MJ::PrintInfo::PrintLog("[%s]BagSearch::AllocDur, scale intensity is %.2f", bagPlan->m_qParam.log.c_str(), scale);

	return 0;
}

int BagSearch::GetBenchMark(BagPlan* bagPlan) {
	int maxHot = 0;
	int maxDist = 0;
	int maxTime = 0;
	for (int i = 0; i < bagPlan->m_futureList.size(); ++i) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(bagPlan->m_futureList[i]);
		if (!vPlace) continue;
		int hot = bagPlan->GetHot(vPlace);
		int dist = 0;
		if (bagPlan->m_maxTrafDistMap.find(vPlace->_ID) != bagPlan->m_maxTrafDistMap.end()) {
			dist = bagPlan->m_maxTrafDistMap[vPlace->_ID];
		} else {
			dist = bagPlan->GetAvgTrafDist(vPlace->_ID);
		}
		int time = 0;
		if (bagPlan->m_maxTrafTimeMap.find(vPlace->_ID) != bagPlan->m_maxTrafTimeMap.end()) {
			time = bagPlan->m_maxTrafTimeMap[vPlace->_ID];
		} else {
			time = bagPlan->GetAvgTrafTime(vPlace->_ID);
		}
		maxHot += hot;
		maxDist += dist;
		maxTime += time;
		MJ::PrintInfo::PrintDbg("[%s]BagSearch::GetBenchMark, [v%d], %s(%s), %d, %d, miss: %d, traf: %d, %s", bagPlan->m_qParam.log.c_str(), i, vPlace->_ID.c_str(), vPlace->_name.c_str(), hot, bagPlan->GetAllocDur(vPlace), bagPlan->GetMissLevel(vPlace), dist, ToolFunc::NormSeconds(time).c_str());
	}
	for (int i = 0; i + 1 < bagPlan->GetKeyNum(); ++i) {
		const KeyNode* curKey = bagPlan->GetKey(i);
		const KeyNode* nextKey = bagPlan->GetKey(i + 1);
		int dist = 0;
		int time = 0;
		if (curKey->_place == nextKey->_place) {
			if (nextKey->_continuous == KEY_NODE_CONTI_NO) {
				if (bagPlan->m_maxTrafDistMap.find(curKey->_place->_ID) != bagPlan->m_maxTrafDistMap.end()) {
					dist = bagPlan->m_maxTrafDistMap[curKey->_place->_ID];
				} else {
					dist = bagPlan->GetAvgTrafDist(curKey->_place->_ID);
				}
				if (bagPlan->m_maxTrafTimeMap.find(curKey->_place->_ID) != bagPlan->m_maxTrafTimeMap.end()) {
					time = bagPlan->m_maxTrafTimeMap[curKey->_place->_ID];
				} else {
					time = bagPlan->GetAvgTrafTime(curKey->_place->_ID);
				}
			}
		} else {
			const TrafficItem* trafItem = bagPlan->GetTraffic(curKey->_place->_ID, nextKey->_place->_ID, curKey->_close_date);
			if (trafItem) {
				dist = trafItem->_dist;
				time = trafItem->_time;
			}
		}
		MJ::PrintInfo::PrintDbg("[%s]BagSearch::GetBenchMark, [k%d], %s(%s): %d, %s", bagPlan->m_qParam.log.c_str(), i + 1, nextKey->_place->_ID.c_str(), nextKey->_place->_name.c_str(), dist, ToolFunc::NormSeconds(time).c_str());
		maxDist += dist;
		maxTime += time;
	}
	maxDist *= 1.2;
	maxTime *= 1.2;
	bagPlan->SetMaxHot(maxHot);
	bagPlan->SetMaxDist(maxDist);
	bagPlan->SetMaxTime(maxTime);
	if (maxHot < 0 || maxDist < 0 || maxTime < 0) {
		MJ::PrintInfo::PrintErr("[%s]BagSearch::GetBenchMark, bcmHot or bcmDist or bcmTime is 0!!!", bagPlan->m_qParam.log.c_str());
		bagPlan->m_error.Set(55401, "计算benchMark为0");
		return 1;
	}
	MJ::PrintInfo::PrintLog("[%s]BagSearch::GetBenchMark, bcmHot: %d, bcmDist: %d, bcmTime: %s", bagPlan->m_qParam.log.c_str(), maxHot, maxDist, ToolFunc::NormSeconds(maxTime).c_str());
	return 0;
}

// 获取预估数量
int BagSearch::GetEstPlace(BagPlan* bagPlan) {
	bagPlan->m_estPlaceNum = 0;
	int leftAvailDur = bagPlan->m_availDur + std::min(bagPlan->m_availDur / 2, bagPlan->m_cbP.m_cbRichExtraDur);
	for (int i = 0; i < bagPlan->m_waitPlaceList.size(); ++i) {
		const LYPlace* place = bagPlan->m_waitPlaceList[i];
		if (!bagPlan->m_userMustPlaceSet.count(place) and leftAvailDur < 0) continue; //保证加入所有的必去点
		if (bagPlan->m_futureSet.find(place->_ID) != bagPlan->m_futureSet.end()) continue;
		bagPlan->m_futureSet.insert(place->_ID);
		bagPlan->m_futureList.push_back(place);
//		std::cerr << "jjj future " << place->_ID << " " << place->_name << std::endl;
		if (bagPlan->m_userMustPlaceSet.count(place) or leftAvailDur >= bagPlan->m_cbP.m_cbRichExtraDur) {
			++bagPlan->m_estPlaceNum;
		}
		if (place->_t & LY_PLACE_TYPE_VIEW || place->_t & LY_PLACE_TYPE_SHOP) {
			leftAvailDur -= bagPlan->GetAllocDur(place);
		}
	}
	return 0;
}

int BagSearch::GetCBP(BagPlan* bagPlan, MyThreadPool* threadPool) {
	std::string flag = "Default";
	int level = bagPlan->AvailDurLevel();
	std::tr1::unordered_map<std::string, std::vector<std::pair<int, CityBParam> > >::iterator it = BagParam::m_cbPMap.find(bagPlan->m_City->_ID);
	if (bagPlan->m_useCBP && it != BagParam::m_cbPMap.end() && !it->second.empty()) {

		std::vector<std::pair<int, CityBParam> >& cbParamList = it->second;
		std::vector<std::pair<int, CityBParam> >::iterator matchIt = cbParamList.end();
		for (std::vector<std::pair<int, CityBParam> >::iterator ii = cbParamList.begin(); ii != cbParamList.end(); ++ii) {
			if (level <= ii->first) {
				matchIt = ii;
				break;
			}
		}
		if (matchIt != cbParamList.end()) {
			bagPlan->m_cbP = matchIt->second;
			flag = "Hit";
		} else {
			bagPlan->m_cbP = BagParam::m_minCBParam;
			flag = "Min";
		}
	}
	// 由于预分配内存限制 不能超过初始大小
	if (bagPlan->m_cbP.m_cbRootHeapLimit > BagParam::m_rootHeapLimit) {
		bagPlan->m_cbP.m_cbRootHeapLimit = BagParam::m_rootHeapLimit;
	}
	if (bagPlan->m_userMustPlaceSet.size() * 2 >= bagPlan->m_waitPlaceList.size()) {
		bagPlan->m_cbP.m_cbRootHeapLimit = BagParam::m_rootHeapLimit;
		MJ::PrintInfo::PrintLog("[%s]BagSearch::GetCBP, Change rootHeapLimit for userMustplace is too more: %d", bagPlan->m_qParam.log.c_str(), bagPlan->m_userMustPlaceSet.size());
	}
	if (bagPlan->m_cbP.m_cbRichHeapLimit > BagParam::m_richHeapLimit) {
		bagPlan->m_cbP.m_cbRichHeapLimit = BagParam::m_richHeapLimit;
	}
	if (bagPlan->m_cbP.m_cbDFSHeapLimit > BagParam::m_dfsHeapLimit) {
		bagPlan->m_cbP.m_cbDFSHeapLimit = BagParam::m_dfsHeapLimit;
	}
	threadPool->SetTopK(bagPlan->m_cbP.m_cbRichHeapLimit, bagPlan->m_cbP.m_cbDFSHeapLimit);
	MJ::PrintInfo::PrintLog("[%s]BagSearch::GetCBP, %s!! %s_%d, P1:%d,P2:%d,P3:%d,P4:%d,P5:%d,P6:%d,P7:%d", bagPlan->m_qParam.log.c_str(), flag.c_str(), bagPlan->m_City->_ID.c_str(), level, bagPlan->m_cbP.m_cbRootHeapLimit, bagPlan->m_cbP.m_cbRichHeapLimit, bagPlan->m_cbP.m_cbDFSHeapLimit, bagPlan->m_cbP.m_cbRootRetLimit, bagPlan->m_cbP.m_cbRichExtraDur, bagPlan->m_cbP.m_cbRichTopK, bagPlan->m_cbP.m_cbRichMissLimit);
	return 0;
}

int BagSearch::CalDur(BagPlan* bagPlan) {
	for (int i = 0; i < bagPlan->m_waitPlaceList.size(); ++i) {
		const LYPlace* place = bagPlan->m_waitPlaceList[i];
		const DurS* durSP = bagPlan->GetDurSP(place);
		if (!durSP) {
			const DurS durS = bagPlan->GetDurS(place);
			bagPlan->SetDurS(place,durS);
		}
	}
	return 0;
}
