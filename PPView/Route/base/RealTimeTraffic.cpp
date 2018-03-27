#include <limits>
#include <iostream>
#include <algorithm>
#include <math.h>
#include "RealTimeTraffic.h"
#include "TrafficData.h"
#include "PathTraffic.h"

// 替换整条Path中的交通
int RealTimeTraffic::DoReplace(BasePlan* basePlan, PathView& path) {
	MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::DoReplace, ...", basePlan->m_qParam.log.c_str());

	// 获取真实交通
	int ret = GetRealTimeTraf(basePlan, path);
	if (ret != 0) return 1;

	ret = ModLuggageHotel(basePlan, path);
	if (ret != 0) return 1;

	// 按arrive和睡觉keynode划分
	int begIdx = -1;
	PathView tmpPath;
	for (int endIdx = 0; endIdx < path.Length(); ++endIdx) {
		const PlanItem* item = path.GetItemIndex(endIdx);
		if (!(item->_type & NODE_FUNC_KEY) || item->_type & NODE_FUNC_KEY_HOTEL_LUGGAGE) continue;
		if (begIdx >= 0 && endIdx >= 0) {
			tmpPath.CopyN(path, begIdx, endIdx);
			ret = Replace(basePlan, tmpPath);
			if (ret == 0) {
				path.SubN(tmpPath, begIdx, endIdx);
				MJ::PrintInfo::PrintErr("[8003STAT] qid:%s,RealTimeTraffic::DoReplace Result:Success",basePlan->m_qParam.qid.c_str());
				MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::DoReplace, [%d, %d] Replace Done", basePlan->m_qParam.log.c_str(), begIdx, endIdx);
			} else {
				MJ::PrintInfo::PrintErr("[8003STAT] qid:%s,RealTimeTraffic::DoReplace Result:fail",basePlan->m_qParam.qid.c_str());
				MJ::PrintInfo::PrintErr("[%s]RealTimeTraffic::DoReplace, [%d, %d] Replace failed", basePlan->m_qParam.log.c_str(), begIdx, endIdx);
			}
		}
		begIdx = endIdx;
	}

	return 0;
}

int RealTimeTraffic::GetRealTimeTraf(BasePlan* basePlan, PathView& path) {
	std::tr1::unordered_set<std::string> pairSet;
	const PlanItem* lastNonAttachItem = NULL;
	for (int i = 0; i < path.Length(); ++i) {
		const PlanItem* item = path.GetItemIndex(i);
		if (lastNonAttachItem && LYConstData::IsRealID(lastNonAttachItem->_place->_ID) && LYConstData::IsRealID(item->_place->_ID)) {
			const TrafficItem* trafItem = basePlan->GetTraffic(lastNonAttachItem->_place->_ID, item->_place->_ID, lastNonAttachItem->_trafDate);
			if (!trafItem || trafItem->m_stat != TRAF_STAT_REAL_TIME && basePlan->m_trafPrefer != TRAF_PREFER_TAXI) {
				// 实时不用替换
				pairSet.insert(lastNonAttachItem->_place->_ID + "_" + item->_place->_ID + "_" + lastNonAttachItem->_trafDate);
			}
		}
		if (item->_place != LYConstData::m_attachRest) {
			lastNonAttachItem = item;
		}
	}
	int ret = PathTraffic::GetTrafPair(basePlan, pairSet, true);
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("[%s]RealTimeTraffic::GetRealTimeTraf, GetRealTimeTraf Error!", basePlan->m_qParam.log.c_str());
		return 1;
	}
	return 0;
}

// 替换某一天
int RealTimeTraffic::Replace(BasePlan* basePlan, PathView& path) {
	// 时间压缩
	int ret = PathUtil::DoZipDur(basePlan, path);
	if (ret != 0) return 1;

	// 交通替换
	ret = ChangeTraf(basePlan, path);
	if (ret != 0) return 1;

	// 拉伸时长
	ret = PathUtil::TimeEnrich(basePlan, path);
	if (ret != 0) return 1;

	return 0;
}

int RealTimeTraffic::ChangeTraf(BasePlan* basePlan, PathView& path) {
	if (path.Length() <= 0) return 1;

	const PlanItem* lastItem = path.GetItemIndex(path.Length() - 1);
	if (lastItem->_place->_t & LY_PLACE_TYPE_ARRIVE) {
		return ChangeTrafRev(basePlan, path);
	} else {
		return ChangeTrafPos(basePlan, path);
	}
	return 1;
}

int RealTimeTraffic::ChangeTrafPos(BasePlan* basePlan, PathView& path) {
//	std::cerr << "jjj change dump before " << path.Length() << std::endl;
//	path.Dump(true);
	for (int i = 1; i < path.Length(); ++i) {
		PlanItem* lastItem = path.GetItemIndex(i - 1);
		PlanItem* curItem = path.GetItemIndex(i);
		const TrafficItem* trafItem = curItem->_arriveTraffic;
		const TrafficItem* newTrafItem = trafItem;
		if (trafItem) {
			newTrafItem = basePlan->GetTraffic(trafItem->m_startP, trafItem->m_stopP, lastItem->_trafDate);
			if (!newTrafItem) {
				newTrafItem = trafItem;
			} else {
				MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::ChangeTrafPos (%s -> %s) date %s GetNewTraffic success ori: time %d, dist %d new: time %d, dist %d", basePlan->m_qParam.log.c_str(),
									trafItem->m_startP.c_str(), trafItem->m_stopP.c_str(), curItem->_trafDate.c_str(), trafItem->_time, trafItem->m_realDist, newTrafItem->_time, newTrafItem->m_realDist);
			}
		}

		int arvTrafTime = newTrafItem ? newTrafItem->_time : 0;

		time_t arvTime = lastItem->_departTime + arvTrafTime;
		if (arvTime < curItem->_openTime) {
			arvTime = curItem->_openTime;
		}
		time_t deptTime = arvTime + curItem->GetDur();
		if (curItem->_type & NODE_FUNC_KEY && !(curItem->_type & NODE_FUNC_KEY_HOTEL_LUGGAGE)) {
			deptTime = arvTime + curItem->_durS->m_min;
			if (deptTime < curItem->_closeTime) {
				deptTime = curItem->_closeTime;
			}
		}
		// dur已是最小 保证不了替换失败
		if (deptTime > curItem->_closeTime) return 1;
		MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::ChangeTrafRev success!", basePlan->m_qParam.log.c_str());

		lastItem->_departTraffic = newTrafItem;
		curItem->_arriveTraffic = newTrafItem;
		curItem->_arriveTime = arvTime;
		curItem->_departTime = deptTime;
	}
//	std::cerr << "jjj change dump after " << std::endl;
//	path.Dump(true);
	return 0;
}

int RealTimeTraffic::ChangeTrafRev(BasePlan* basePlan, PathView& path) {
//	std::cerr << "jjj change dump before " << path.Length() << std::endl;
//	path.Dump(true);
	for (int i = path.Length() - 2; i >= 0; --i) {
		PlanItem* nextItem = path.GetItemIndex(i + 1);
		PlanItem* curItem = path.GetItemIndex(i);
		const TrafficItem* trafItem = curItem->_departTraffic;
		const TrafficItem* newTrafItem = trafItem;
		if (trafItem) {
			newTrafItem = basePlan->GetTraffic(trafItem->m_startP, trafItem->m_stopP, curItem->_trafDate);
			if (!newTrafItem) {
				newTrafItem = trafItem;
			} else {
				MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::ChangeTrafRev (%s -> %s) date %s GetNewTraffic success ori: time %d, dist %d new: time %d, dist %d", basePlan->m_qParam.log.c_str(),
									trafItem->m_startP.c_str(), trafItem->m_stopP.c_str(), curItem->_trafDate.c_str(), trafItem->_time, trafItem->m_realDist, newTrafItem->_time, newTrafItem->m_realDist);
			}
		}

		int arvTrafTime = newTrafItem ? newTrafItem->_time : 0;
		time_t deptTime = nextItem->_arriveTime - arvTrafTime;
		if (deptTime > curItem->_closeTime) {
			deptTime = curItem->_closeTime;
		}
		time_t arvTime = deptTime - curItem->GetDur();
		if (curItem->_type & NODE_FUNC_KEY && !(curItem->_type & NODE_FUNC_KEY_HOTEL_LUGGAGE)) {
			arvTime = deptTime - curItem->_durS->m_min;
			if (arvTime > curItem->_arriveTime) {
				arvTime = curItem->_arriveTime;
			}
			if (arvTime < curItem->_arriveTime) return 1;
		}
		if (arvTime < curItem->_openTime) return 1;
		MJ::PrintInfo::PrintLog("[%s]RealTimeTraffic::ChangeTrafRev success!", basePlan->m_qParam.log.c_str());
		nextItem->_arriveTraffic = newTrafItem;
		curItem->_departTraffic = newTrafItem;
		curItem->_arriveTime = arvTime;
		curItem->_departTime = deptTime;
	}
//	std::cerr << "jjj change dump after " << std::endl;
//	path.Dump(true);
	return 0;
}

// 修改luggage hotel的时长和属性
int RealTimeTraffic::ModLuggageHotel(BasePlan* basePlan, PathView& path) {
	std::vector<PlanItem*> realKeyList;
	for (int i = 0; i < path.Length(); ++i) {
		PlanItem* curItem = path.GetItemIndex(i);
		if (curItem->_type & NODE_FUNC_KEY) {
			realKeyList.push_back(curItem);
		}
	}

	for (int i = 0; i < realKeyList.size(); ++i) {
		PlanItem* curItem = realKeyList[i];
//		if (curItem->_place->_t & LY_PLACE_TYPE_HOTEL && curItem->GetDur() < basePlan->m_MinSleepTimeCost) {
		if (curItem->_type & NODE_FUNC_KEY_HOTEL_LUGGAGE) {
			if (i - 1 >= 0) {
				PlanItem* lastItem = realKeyList[i - 1];
				curItem->_openTime = lastItem->_openTime;
			}
			if (i + 1 < realKeyList.size()) {
				PlanItem* nextItem = realKeyList[i + 1];
				curItem->_closeTime = nextItem->_closeTime;
			}
//			curItem->_type = NODE_FUNC_KEY_HOTEL_LUGGAGE;
//			std::cerr << "jjj mod luggage" << std::endl;
//			curItem->Dump();
		}
	}
	return 0;
}
