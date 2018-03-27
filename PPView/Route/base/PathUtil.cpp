#include <iostream>
#include "PathUtil.h"
#include <queue>
#include <algorithm>
#include "PathTraffic.h"

// 合并行李点和酒店点 并修改交通和时间
int PathUtil::MergeLuggAndSleep(BasePlan* basePlan) {
	PathView& path = basePlan->m_PlanList;

	if (path.Length() >= 3) {
		PlanItem* arvItem = path.GetItemIndex(0);
		PlanItem* luggageItem = path.GetItemIndex(1);
		PlanItem* sleepItem = path.GetItemIndex(2);
		if(luggageItem->_type == NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE
				&& luggageItem->_place == sleepItem->_place){
			sleepItem->_arriveTraffic = luggageItem->_arriveTraffic;
			//sleepItem->_arriveTime = luggageItem->_arriveTime;
			//sleepItem->_arriveDate = luggageItem->_arriveDate;
			path.Erase(1);
		}
	}
	if (path.Length() >= 3) {
		int len = path.Length();
		PlanItem* deptItem = path.GetItemIndex(len - 1);
		PlanItem* luggageItem = path.GetItemIndex(len - 2);
		PlanItem* sleepItem = path.GetItemIndex(len - 3);
		if (luggageItem->_type == NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE
				&& luggageItem->_place == sleepItem->_place
				&& luggageItem->_arriveTime - sleepItem->_departTime < 3*3600){
			sleepItem->_departTraffic = luggageItem->_departTraffic;
			//sleepItem->_departTime = luggageItem->_departTime;
			//sleepItem->_departDate = luggageItem->_departDate;
			path.Erase(len - 2);
		}
	}

	return 0;
}

int PathUtil::BackHotelTimeChange(PathView & path)
{
	//返回酒店的时间改为真实时间
	for (int i = 1; i < path.Length()-1; ++i) {
		PlanItem* lastItem = path.GetItemIndex(i - 1);
		PlanItem* curItem = path.GetItemIndex(i);
		if (not lastItem || not curItem ) continue;
		//防止空行程时修改酒店时间
		if (lastItem->_type != NODE_FUNC_KEY_HOTEL_SLEEP && curItem->_type == NODE_FUNC_KEY_HOTEL_SLEEP)
		{
			int arvTrafT = lastItem->_departTraffic ? lastItem->_departTraffic->_time : 0;
			time_t elstArvT = lastItem->_departTime + arvTrafT;
			curItem->_arriveTime = elstArvT;
		}
	}

	return 0;
}


int PathUtil::Norm15Min(BasePlan* basePlan) {
	PathView& path = basePlan->m_PlanList;
	for (int i = 1; i < path.Length() - 1; ++i) {
		PlanItem* curItem = path.GetItemIndex(i);
		PlanItem* lastItem = path.GetItemIndex(i - 1);
		PlanItem* nextItem = path.GetItemIndex(i + 1);
		if (!curItem || !lastItem || !nextItem) continue;
		if (curItem->_type & NODE_FUNC_PLACE) {
//			curItem->Dump();
//			if ("制定了到达离开时间") {
//				continue;
//			}
//
			if (basePlan->m_vPlaceOpenMap.find(curItem->_place->_ID) != basePlan->m_vPlaceOpenMap.end()) {
				continue;
			}

			MJ::PrintInfo::PrintLog("[%s]PathUtil::Norm15Min, change 15min  for %s(%s)",basePlan->m_qParam.log.c_str(), curItem->_place->_ID.c_str() , curItem->_place->_name.c_str());
			int arvTrafT = lastItem->_departTraffic ? lastItem->_departTraffic->_time : 0;
			int deptTrafT = nextItem->_arriveTraffic ? nextItem->_arriveTraffic->_time : 0;
			time_t elstArvT = lastItem->_departTime + arvTrafT;
			time_t ltstDeptT = nextItem->_arriveTime - deptTrafT;
			if (basePlan->m_UserDurMap.find(curItem->_place->_ID) != basePlan->m_UserDurMap.end()) {//锁定用户时长
				int dur = basePlan->m_UserDurMap[curItem->_place->_ID];
				time_t arvT = std::max(curItem->_openTime, elstArvT);
				time_t deptT = std::min(curItem->_closeTime, ltstDeptT);
				curItem->_arriveTime = arvT;
				curItem->_departTime = arvT + dur;
				continue;
			}
			int dur = curItem->GetDur();
			int mod = dur % 900;
			if (mod == 0) {
				continue;
			}
			int needAdd = 900 - mod;
			int newdur = dur + needAdd;
			if (ltstDeptT - elstArvT >= newdur) {
				if (curItem->_arriveTime - needAdd > elstArvT) {
					curItem->_arriveTime = curItem->_arriveTime - needAdd;
				} else {
					needAdd -= curItem->_arriveTime - elstArvT;
					curItem->_arriveTime = elstArvT;
					curItem->_departTime += needAdd;
				}
			} else {
				if (curItem->GetDur() - mod > 0) {
					curItem->_departTime -= mod;
				}
			}
		}
	}
	return 0;
}

// 将行程中非keynode点时长压缩至最小
int PathUtil::DoZipDur(BasePlan* basePlan, PathView& path) {
	for (int i = 1; i < path.Length(); ++i) {
		const PlanItem* lastItem = path.GetItemIndex(i - 1);
		PlanItem* curItem = path.GetItemIndex(i);
		if (curItem->_type & NODE_FUNC_KEY) continue;
		int arvTrafTime = (curItem->_arriveTraffic) ? curItem->_arriveTraffic->_time : 0;
		time_t arvTime = lastItem->_departTime + arvTrafTime;
		if (arvTime < curItem->_openTime) {
			arvTime = curItem->_openTime;
		}
		time_t deptTime = arvTime + curItem->_durS->m_zip;
		if (deptTime > curItem->_closeTime) {
			MJ::PrintInfo::PrintErr("[%s]PathUtil::TimeEnrich DoZipDur arv = %s, zip = %d, dept = %s, open = %s, close = %s ", basePlan->m_qParam.log.c_str(),
							MJ::MyTime::toString(arvTime, basePlan->m_TimeZone).c_str(),
							curItem->_durS->m_zip,
							MJ::MyTime::toString(deptTime, basePlan->m_TimeZone).c_str(),
							MJ::MyTime::toString(curItem->_openTime, basePlan->m_TimeZone).c_str(),
							MJ::MyTime::toString(curItem->_closeTime, basePlan->m_TimeZone).c_str());
			return 1;
		}
		curItem->_arriveTime = arvTime;
		curItem->_departTime = deptTime;
	}
	return 0;
}

int PathUtil::TimeEnrich(BasePlan* basePlan, PathView& path) {
	PathView tmpPath;
	tmpPath.Copy(path);
	int start = 0;
	int stop = 0;
	int i = 0;
	int ret = DoZipDur(basePlan, tmpPath);
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("[%s]PathUtil::TimeEnrich DoZipDur error ", basePlan->m_qParam.log.c_str());
		return 1;
	}
	while (i < tmpPath.Length()) {
		const PlanItem* pItem = tmpPath.GetItemIndex(i);
		if (pItem->_type & NODE_FUNC_KEY) {
			start = stop;
			stop = i;
			if (stop - start > 1) {
				int ret = TimeStretch(basePlan, start, stop, tmpPath);
				if (ret) {
					tmpPath.Dump(basePlan);
					MJ::PrintInfo::PrintErr("[%s]PathUtil::TimeEnrich, error start %d ->  stop %d", basePlan->m_qParam.log.c_str(), start, stop);
					return ret;
				}
			}
		}
		++i;
	}
	path.Copy(tmpPath);
	return 0;
}

double StretchItem::GetStretchRate(const StretchItem* sItem) {
	return sItem->m_pItem->GetDur() * 1.0 / sItem->m_rcmdDur;
}

int StretchItem::GetHotLevel(const StretchItem* sItem) {
	if (sItem->m_pItem->_place->_t & (LY_PLACE_TYPE_VIEW | LY_PLACE_TYPE_SHOP | LY_PLACE_TYPE_RESTAURANT)) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(sItem->m_pItem->_place);
		if(vPlace) return vPlace->GetHotLevel();
	} else {
		return 0;
	}
}

int PathUtil::TimeStretch(BasePlan* basePlan, int start, int stop,  PathView& path) {
	start = std::max(0, start);
	stop = std::min(stop, path.Length() - 1);
	double rate = 1.1; //std::min(rate, 1.0);
	//优先伸展Level级别高的点
	std::vector<const StretchItem*> sItemList;
	//非KeyReal点, attach点进行排序
	for (int i = start + 1 ; i < stop; i++) {
		PlanItem* pItem = path.GetItemIndex(i);
		if (pItem->_type & NODE_FUNC_KEY) continue;
		const StretchItem* sItem = new StretchItem(i, pItem, pItem->_durS->m_rcmd);
		sItemList.push_back(sItem);
	}
	if (!sItemList.size()) {
		return 0;
	}
	//Level排序
	std::sort(sItemList.begin(), sItemList.end(), StretchItemCmp());
	//入队待伸展
	std::queue<const StretchItem*> sItemQueue;
	for (int i = 0 ; i < sItemList.size(); i ++) {
		sItemQueue.push(sItemList[i]);
	}
	//rate排序
	std::sort(sItemList.begin(), sItemList.end(), StretchItemRateCmp());
	while(!sItemQueue.empty()) {
		const StretchItem* sItem = sItemQueue.front();
		sItemQueue.pop();
		int level = StretchItem::GetHotLevel(sItem);
		//判断可伸展
		bool canStretch = true;
		for (int i = 0 ; i < sItemList.size(); i ++ ) {
			if (sItemList[i] == sItem) {
				break;
			}
			//hotLevel 大于当前点且rate值更小
			if (StretchItem::GetHotLevel(sItemList[i]) >= level) {
				canStretch = false;
				break;
			}
		}
		//不可伸展回队列等待下次机会
		if (!canStretch)  {
			sItemQueue.push(sItem);
			sItem = NULL;
			continue;
		}
		//伸展逻辑
		PlanItem* pItem = path.GetItemIndex(sItem->m_index);
		int dur = std::min(static_cast<int>(pItem->GetDur() * rate), pItem->_durS->m_extend);
		dur = std::max(dur, pItem->_durS->m_zip);
		int arriveTraffic = 0;
		if (pItem->_arriveTraffic) {
			arriveTraffic = pItem->_arriveTraffic->_time;
		}
		int departTraffic = 0;
		if (pItem->_departTraffic) {
			departTraffic = pItem->_departTraffic->_time;
		}
		//获取可用前后时间
		time_t earliestT = GetEarliestDept(basePlan, start, sItem->m_index, path);
		time_t latestT = GetLatestArv(basePlan, stop, sItem->m_index, path);
		if (earliestT == -1 || latestT == -1)  {
			MJ::PrintInfo::PrintErr("[%s]PathUtil::TimeStretch, earliestT %d, latestT %d",basePlan->m_qParam.log.c_str(), earliestT, latestT);
			delete sItem;
			sItem = NULL;
			while(!sItemQueue.empty()) {
				delete sItemQueue.front();
				sItemQueue.pop();
			}
			return 1;
		}
		//处理时间冲突
		time_t arvT = earliestT + arriveTraffic;
		if (arvT < pItem->_openTime) {
			arvT = pItem->_openTime;
		}
		time_t deptT = latestT - departTraffic;
		if (deptT > pItem->_closeTime) {
			deptT = pItem->_closeTime;
		}
		if (arvT > deptT) {
			MJ::PrintInfo::PrintErr("[%s] PathUtil::TimeStretch arvT %s > deptT %s | earliestT %s + trafArv %d  > latestT %s - trafDept %d | open %s, close %s | place %s(%s),dur %d newDur %d" ,
				basePlan->m_qParam.log.c_str(), MJ::MyTime::toString(arvT, basePlan->m_TimeZone).c_str(),
												MJ::MyTime::toString(deptT, basePlan->m_TimeZone).c_str(),
												MJ::MyTime::toString(earliestT, basePlan->m_TimeZone).c_str(),
												arriveTraffic,
												MJ::MyTime::toString(latestT, basePlan->m_TimeZone).c_str(),
												departTraffic,
												MJ::MyTime::toString(pItem->_openTime, basePlan->m_TimeZone).c_str(),
												MJ::MyTime::toString(pItem->_closeTime, basePlan->m_TimeZone).c_str(),
												pItem->_place->_ID.c_str(), pItem->_place->_name.c_str(), pItem->GetDur(), dur);
			return 1;
		}
		if (arvT + dur <= deptT) {//条件满足
			pItem->_arriveTime = arvT;
			pItem->_departTime = pItem->_arriveTime + dur;
			if (pItem->GetDur() < pItem->_durS->m_extend) {//可继续拉伸
				sItemQueue.push(sItem);
				//归队后 重排rate列表
				std::sort(sItemList.begin(), sItemList.end(), StretchItemRateCmp());
			} else {
				//去除不能拉伸的点
				for (std::vector<const StretchItem*>::iterator it = sItemList.begin(); it != sItemList.end(); it ++) {
					if ((*it) == sItem) {
						sItemList.erase(it);
						break;
					}
				}
				delete sItem;
				sItem = NULL;
			}
		} else {
			dur = deptT - arvT;
			pItem->_arriveTime = arvT;
			pItem->_departTime = pItem->_arriveTime + dur;
			//去除不能拉伸的点
			for (std::vector<const StretchItem*>::iterator it = sItemList.begin(); it != sItemList.end(); it ++) {
				if ((*it) == sItem) {
					sItemList.erase(it);
					break;
				}
			}
			delete sItem;
			sItem = NULL;
		}
	}
	return 0;
}

time_t PathUtil::GetEarliestDept(BasePlan* basePlan, int start, int index, PathView& path) {
	time_t earliestDeptT = -1;
	for (int i = start; i < index; i ++) {
		PlanItem* pItem = path.GetItemIndex(i);
		if (pItem->_type & NODE_FUNC_KEY) {
			earliestDeptT = pItem->_departTime;
			continue;
		}
		if (i == start) {
			earliestDeptT = pItem->_departTime;
			continue;
		}
		int trafT = 0;
		if (pItem->_arriveTraffic) {
			trafT = pItem->_arriveTraffic->_time;
		}
		time_t arriveTime = earliestDeptT + trafT;
		if (arriveTime < pItem->_openTime) {
			arriveTime = pItem->_openTime;
		}
		int dur = pItem->GetDur();
		if (arriveTime + pItem->GetDur() > pItem->_closeTime) {
			return -1;
		}
		pItem->_arriveTime = arriveTime;
		pItem->_departTime = pItem->_arriveTime + dur;
		earliestDeptT = pItem->_departTime;
	}
	return earliestDeptT;
}

time_t PathUtil::GetLatestArv(BasePlan* basePlan, int end, int index, PathView& path) {
	time_t latestArvT = -1;
	for (int i = end; i > index; i--) {
		PlanItem* pItem = path.GetItemIndex(i);
		if (pItem->_type & NODE_FUNC_KEY) {
			latestArvT = pItem->_arriveTime;
			continue;
		}
		if (i == end) {
			latestArvT = pItem->_arriveTime;
			continue;
		}
		int trafT = 0;
		if (pItem->_departTraffic) {
			trafT = pItem->_departTraffic->_time;
		}
		time_t departTime = latestArvT - trafT;
		if (departTime > pItem->_closeTime) {
			departTime = pItem->_closeTime;
		}
		int dur = pItem->GetDur();
		if (pItem->_openTime > departTime - pItem->GetDur()) {
			return -1;
		}
		pItem->_departTime = departTime;
		pItem->_arriveTime = pItem->_departTime - dur;
		latestArvT = pItem->_arriveTime;
	}
	return latestArvT;
}

int PathUtil::CalTraf(BasePlan* basePlan, PathView& path) {
	int trafSum = 0;
	for (int i = 1; i < path.Length(); i++) {
		const PlanItem* pItem = path.GetItemIndex(i);
		int trafT = 0;
		if (pItem->_arriveTraffic) {
			trafT = pItem->_arriveTraffic->_time;
		}
		trafSum += trafT;
	}
	return trafSum;
}

int PathUtil::SetDur2PlanStats(BasePlan* basePlan, PathView& path) {
	for (int i = 1; i < path.Length(); i++) {
		const PlanItem* pItem = path.GetItemIndex(i);
		if (pItem->_place->_t & LY_PLACE_TYPE_VAR_PLACE && pItem->_place->_ID != "attach") {
			basePlan->m_planStats.SetPlaceDur(pItem->_place, pItem->GetDur());
		}
	}
	return 0;
}

int PathUtil::CalIdel(BasePlan* basePlan, PathView& path) {
	int blockTime = 0;
	for (int i = 1; i < path.Length(); i++) {
		PlanItem* pItem = path.GetItemIndex(i);
		PlanItem* lItem = path.GetItemIndex(i - 1);
		int trafT = 0;
		if (lItem->_departTraffic) {
			trafT = lItem->_departTraffic->_time;
		}
		if (pItem->_arriveTraffic) {
			trafT = pItem->_arriveTraffic->_time;
		}
		if (lItem->_departTime + trafT <= pItem->_arriveTime) {
			blockTime += pItem->_arriveTime - lItem->_departTime - trafT;
		} else {
			MJ::PrintInfo::PrintErr("[%s]PathUtil::CalIdel, last %s , cur %s Error! ",basePlan->m_qParam.log.c_str(), lItem->_place->_name.c_str(), pItem->_place->_name.c_str());
			MJ::PrintInfo::PrintErr("[%s]PathUtil::CalIdel, dept %s ->trafic %d ->arv %s Error! ",basePlan->m_qParam.log.c_str(), MJ::MyTime::toString(lItem->_departTime, basePlan->m_TimeZone).c_str(), trafT,  MJ::MyTime::toString(pItem->_arriveTime, basePlan->m_TimeZone).c_str());
			path.Dump(basePlan);
			return -1;
		}
	}
	return blockTime;
}


int PathUtil::GetScale(BasePlan* basePlan, std::tr1::unordered_map<std::string, int>& allocDurMap) {
	std::tr1::unordered_map<std::string, int> allocDurUserMap;
	std::tr1::unordered_map<std::string, int> allocDurWaitMap;
	int sumUserAllocDur = 0;
	for (auto place: basePlan->m_userMustPlaceSet) {
		int avgAllocDur = basePlan->GetAvgAllocDur(place);
		sumUserAllocDur += avgAllocDur;
		std::tr1::unordered_map<std::string, int>::iterator it = allocDurUserMap.find(place->_ID);
		if (it == allocDurUserMap.end()) {
			allocDurUserMap[place->_ID] = avgAllocDur;
		}
	}
	int sumWaitAllocDur = 0;
	for (int i = 0; i < basePlan->m_waitPlaceList.size(); ++i) {
		const LYPlace* place = basePlan->m_waitPlaceList[i];
		int avgAllocDur = basePlan->GetAvgAllocDur(place);
		sumWaitAllocDur += avgAllocDur;
		std::tr1::unordered_map<std::string, int>::iterator it = allocDurWaitMap.find(place->_ID);
		if (it == allocDurWaitMap.end()) {
			allocDurWaitMap[place->_ID] = avgAllocDur;
		}
	}

	double scale = 1.0;
	int availAllocDur = std::min(basePlan->m_availDur - 3600 * 6, static_cast<int>(basePlan->m_availDur * 0.7));
	if (sumUserAllocDur > basePlan->m_availDur) { //紧张情况
		scale = basePlan->m_availDur * 1.0 / sumUserAllocDur;
//		return scale;
		for (std::tr1::unordered_map<std::string, int>::iterator it = allocDurUserMap.begin(); it != allocDurUserMap.end(); ++it) {
			allocDurMap[it->first] = it->second * scale;
		}
	} else if (sumWaitAllocDur < availAllocDur) { //必须＋可选轻松
		scale = availAllocDur * 1.0 / sumWaitAllocDur;
//		return scale;
		for (std::tr1::unordered_map<std::string, int>::iterator it = allocDurWaitMap.begin(); it != allocDurWaitMap.end(); ++it) {
			allocDurMap[it->first] = std::min(static_cast<int>(it->second * scale), 3600 * 8);
		}
	}
	MJ::PrintInfo::PrintLog("[%s]BagSearch::AllocDur, scale intensity is %.2f", basePlan->m_qParam.log.c_str(), scale);

	return 0;
}

int PathUtil::PerfectDelSet(BasePlan* basePlan, PathView& path) {
	std::tr1::unordered_set<const LYPlace*> inPlanSet;
	for (int i = 0; i < path.Length(); ++i) {
		const PlanItem* item = path.GetItemIndex(i);
		inPlanSet.insert(item->_place);
	}
	int userMustMiss = 0;
	int userOptMiss = 0;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (inPlanSet.find(place) == inPlanSet.end()) {
			basePlan->m_userDelSet.insert(place);
			if (!basePlan->m_planStats.HasPlaceStats(place)) {
				basePlan->m_planStats.SetPlaceStats(place, PLACE_STATS_THROW_GREEDY);
			}
			MJ::PrintInfo::PrintDbg("[%s]PathUtil::PrefectDelSet, miss userMustPlace %s(%s)", basePlan->m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str());
			userMustMiss ++;
		}
	}
	MJ::PrintInfo::PrintDbg("[%s]PathUtil::PrefectDelSet, miss User %d miss rmalbe %d", basePlan->m_qParam.log.c_str(), userMustMiss, userOptMiss);
	return 0;
}

int PathUtil::CutPathBySegment(BasePlan* basePlan, PathView& path, std::vector<PathView*>& pathList) {
	bool cutPath = true;
	int pathIdx = -1;
	for (int i = 0 ; i < path.Length(); i ++) {
		if (cutPath) {
			cutPath = false;
			PathView* newPath = new PathView();
			pathList.push_back(newPath);
			pathIdx ++;
		}
		const PlanItem* pItem = path.GetItemIndex(i);
		if (pItem->_place == LYConstData::m_segmentPlace) {
			cutPath = true;
			continue;
		}
		pathList[pathIdx]->Append(pItem);
	}
	for (int i = 0 ; i < pathList.size(); i ++) {
		pathList[i]->Dump(basePlan, true);
	}
	return 0;
}
