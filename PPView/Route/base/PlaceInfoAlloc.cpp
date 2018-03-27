#include "PlaceInfoAlloc.h"
#include <algorithm>
#include <math.h>
#include "MJLog.h"

// 填写每天的placeinfo
int PlaceInfoAlloc::DoAlloc(BasePlan* basePlan) {
	int ret = 0;

	// 1 清空node_list
	for (int i = 0; i < basePlan->GetBlockNum(); ++i) {
		basePlan->GetBlock(i)->ClearNode();
	}

	// 2 安排placeinfo 不可安排的移除
	ret = Alloc(basePlan);
	for (std::vector<const LYPlace*>::iterator it = basePlan->m_waitPlaceList.begin(); it != basePlan->m_waitPlaceList.end();) {
		const LYPlace* place = *it;
		if (_allocable_place_set.find(place->_ID) == _allocable_place_set.end()) {
			it = basePlan->m_waitPlaceList.erase(it);
			basePlan->m_planStats.SetPlaceStats(place, PLACE_STATS_THROW_ALLOC_FAIL);
		} else {
			++it;
		}
	}

	// 3 修正timeblock中的left信息
	int leftAvailDur = 0;
	std::tr1::unordered_map<std::string, int> leftChanceMap;  // 从后往前，包括当前，place还有几次机会拓展
	for (int i = basePlan->GetBlockNum() - 1; i >= 0; --i) {
		TimeBlock* block = basePlan->GetBlock(i);
		std::vector<const PlaceInfo*>& pInfoList = block->m_pInfoList;

		leftAvailDur += std::max(0, block->_avail_dur);
		block->_left_avail_dur = leftAvailDur;

		std::tr1::unordered_set<std::string> availIdSet;
		for (int i = 0; i < pInfoList.size(); ++i) {
			availIdSet.insert(pInfoList[i]->m_vPlace->_ID);
		}
		for (std::tr1::unordered_set<std::string>::iterator it = availIdSet.begin(); it != availIdSet.end(); ++it) {
			std::string id = *it;
			if (leftChanceMap.find(id) == leftChanceMap.end()) {
				leftChanceMap[id] = 1;
			} else {
				leftChanceMap[id] += 1;
			}
		}
	}

	MJ::PrintInfo::PrintLog("[%s]PlaceInfoAlloc::Alloc, stat time total_avail_dur\t%d", basePlan->m_qParam.log.c_str(), basePlan->GetBlock(0)->_avail_dur + basePlan->GetBlock(0)->_left_avail_dur);
	MJ::PrintInfo::PrintLog("[%s]PlaceInfoAlloc::Alloc, Dump TimeBlocks after fix", basePlan->m_qParam.log.c_str());
	for (int i = 0; i < basePlan->GetBlockNum(); ++i) {
		basePlan->GetBlock(i)->Dump(true);
	}
	return 0;
}

int PlaceInfoAlloc::Alloc(BasePlan* basePlan) {
	for (int i = 0; i < basePlan->m_waitPlaceList.size(); ++i) {
//		MJ::PrintInfo::PrintLog("core gdb vPlace name");
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(basePlan->m_waitPlaceList[i]);
		if (_allocable_place_set.find(vPlace->_ID) != _allocable_place_set.end()
				|| _unallocable_place_set.find(vPlace->_ID) != _unallocable_place_set.end()) continue;

		// 饭店, 购物: 用数量限制
		// 景点: 用数量/scale限制
		// 默认按景点规划
		if (basePlan->IsRestaurant(vPlace)
				&& _allocated_rest_cnt >= basePlan->m_RestNeedNum) continue;
		if (vPlace->_t & LY_PLACE_TYPE_SHOP
				&& _allocated_shop_cnt >= basePlan->m_ShopNeedNum) continue;

		// 尝试安排vPlace
		bool allocable = IsAllocable(basePlan, vPlace);
		// 景点可更灵活地安排
		if (allocable == false and (vPlace->getRawType() != LY_PLACE_TYPE_CAR_STORE)) {
			if (basePlan->m_userMustPlaceSet.count(vPlace) && !(vPlace->_t & LY_PLACE_TYPE_TOURALL)) {
				basePlan->m_lookSet.insert(vPlace);
			}
			if (basePlan->m_lookSet.find(vPlace) != basePlan->m_lookSet.end()
					&& vPlace->_intensity_list.front()->_ignore_open) {
				if(basePlan->m_UserDurMap.find(vPlace->_ID)==basePlan->m_UserDurMap.end())
				{
					basePlan->m_UserDurMap[vPlace->_ID] = vPlace->_intensity_list.front()->_dur;
				}
				MJ::PrintInfo::PrintDbg("[%s]PlaceInfoAlloc::Alloc, %s(%s) only take photos", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str());
				allocable = IsAllocable(basePlan, vPlace);
			}
			// 尝试不吃饭也要玩 对餐厅而言即容错
			if (allocable == false) {
				MJ::PrintInfo::PrintDbg("[%s]PlaceInfoAlloc::Alloc, %s(%s) skip meal", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str());
				allocable = IsAllocable(basePlan, vPlace);
			}
		}

		if (allocable) {
			_allocable_place_set.insert(vPlace->_ID);
			if (basePlan->IsRestaurant(vPlace)) {
				++_allocated_rest_cnt;
			} else if (vPlace->_t & LY_PLACE_TYPE_SHOP) {
				++_allocated_shop_cnt;
			}
		} else {
			_unallocable_place_set.insert(vPlace->_ID);
			basePlan->m_failSet.insert(vPlace);
			MJ::PrintInfo::PrintDbg("[%s]PlaceInfoAlloc::Alloc, unallocable_place: %s(%s)", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str());
		}
		const DurS durS = basePlan->GetDurS(vPlace);
		basePlan->SetDurS(vPlace, durS);
	}
	return 0;
}

// 判断vPlace是否能安排到timeblock内
bool PlaceInfoAlloc::IsAllocable(BasePlan* basePlan, const VarPlace* vPlace) {
	bool ret = false;

	for (int i = 0; i < basePlan->GetBlockNum(); ++i) {
		TimeBlock* block = basePlan->GetBlock(i);
		if (block->_avail_dur < 0) continue;
		if (not (vPlace->getRawType() & LY_PLACE_TYPE_CAR_STORE) and block->IsNotPlay()) continue;
		// 用户不在当前天游玩
		std::string date = block->_trafDate;
		if (!basePlan->IsUserDateAvail(vPlace, date)) continue;

		const KeyNode* fromKey = basePlan->GetKey(i);
		const KeyNode* toKey = basePlan->GetKey(i + 1);

		// vPlace -> placeinfo (受date和user指定影响)
		PlaceInfo* pInfo = NULL;
		basePlan->VarPlace2PlaceInfo(vPlace, date, pInfo);
		if (!pInfo) {
			MJ::PrintInfo::PrintErr("[%s]PlaceInfoAlloc::IsAllocable, vPlaceInfo is NULL in date: %s, vid %s", basePlan->m_qParam.log.c_str(), date.c_str(), vPlace->_ID.c_str());
			continue;
		}
		pInfo->Dump();
		if (pInfo->m_openCloseList.empty()) {
			MJ::PrintInfo::PrintErr("[%s]PlaceInfoAlloc::IsAllocable, vPlaceInfo's openCloseList size 0, vid %s", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str());
			continue;
		}

		// 非容错状况需要判断
		if (basePlan->m_faultTolerant) {
			pInfo->m_availOpenCloseList.assign(pInfo->m_openCloseList.begin(), pInfo->m_openCloseList.end());
		} else {
			time_t start = fromKey->_close;
			time_t stop = toKey->_close - toKey->GetZipDur();

			int arvTrafDur = 0;
			const TrafficItem* arvTraf = basePlan->GetTraffic(fromKey->_place->_ID, pInfo->m_arvID, date);
			if (arvTraf) {
				arvTrafDur = arvTraf->_time;
			} else {
				arvTrafDur = basePlan->GetAvgTrafTime(vPlace->_ID);
			}

			int deptTrafDur = 0;
			const TrafficItem* deptTraf = basePlan->GetTraffic(pInfo->m_deptID, toKey->_place->_ID, date);
			if (deptTraf) {
				deptTrafDur = deptTraf->_time;
			} else {
				deptTrafDur = basePlan->GetAvgTrafTime(vPlace->_ID);
			}

			for (int j = 0; j < pInfo->m_openCloseList.size(); ++j) {
				const OpenClose* openClose = pInfo->m_openCloseList[j];
				time_t okArv = start + arvTrafDur;
				if (okArv < openClose->m_open) {
					okArv = openClose->m_open;
				}
				time_t okDept = stop - deptTrafDur;
				if (okDept > openClose->m_close) {
					okDept = openClose->m_close;
				}
				int nodeType = (pInfo->m_type & NODE_FUNC_PLACE_RESTAURANT) ? NODE_FUNC_PLACE_RESTAURANT : NODE_FUNC_PLACE_VIEW_SHOP;
				int zipDur = basePlan->GetZipDur(vPlace);
				MJ::PrintInfo::PrintDbg("[%s]PlaceInfoAlloc::IsAllocable, %s(%s), %s + %d = %s(vs %s)", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str(), MJ::MyTime::toString(okArv, basePlan->m_TimeZone).c_str(), zipDur, MJ::MyTime::toString(okArv + zipDur, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(okDept, basePlan->m_TimeZone).c_str());
				if (okArv + zipDur > okDept && !(vPlace->_t & LY_PLACE_TYPE_RESTAURANT && basePlan->m_lookSet.find(vPlace) != basePlan->m_lookSet.end())) continue;
				pInfo->m_availOpenCloseList.push_back(openClose);
			}
			//重设pInfo->m_avialOpenCloseList 使openClose成对
			if (vPlace->_t & LY_PLACE_TYPE_RESTAURANT) {
				int lunchNum = 0;
				int supperNum = 0;
				for (int k = 0 ; k < pInfo->m_availOpenCloseList.size(); ++k) {
					const OpenClose* openClose = pInfo->m_availOpenCloseList[k];
					if (openClose->m_meals == RESTAURANT_TYPE_LUNCH) {
						lunchNum++;
					} else if (openClose->m_meals == RESTAURANT_TYPE_SUPPER) {
						supperNum++;
					}
				}
				if (lunchNum == 1 && !supperNum) {
					pInfo->m_availOpenCloseList.push_back(NULL);
				} else if (supperNum == 1 && !lunchNum) {
					pInfo->m_availOpenCloseList.insert(pInfo->m_availOpenCloseList.begin(), NULL);
				}

			}
		}
		if (pInfo->m_availOpenCloseList.empty()) {
			continue;
		}
		MJ::PrintInfo::PrintDbg("[%s]PlaceInfoAlloc::IsAllocable, %s(%s) alloc successed in block[%d]!!", basePlan->m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str(), i);
		pInfo->DumpAvail();
		block->m_pInfoList.push_back(pInfo);
		block->m_pInfoMap[vPlace->_ID] = pInfo;
		block->_nodeFuncMap[vPlace->_ID] |= pInfo->m_type;
		ret = true;  // 成功allocable
	}
	return ret;
}
