#include <iostream>
#include <algorithm>
#include "BasePlan.h"
#include "PathEval.h"
#include "TrafRoute.h"
#include "PathUtil.h"

int TrafRoute::DoRoute(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path, bool needLog) {
	int ret = 0;
	m_dbgDump = needLog;
	if(m_dbgDump)
	{
		MJ::PrintInfo::PrintDbg("[%s]TrafRoute::DoRoute, orderList dump", basePlan->m_qParam.log.c_str());
		for (int i = 0; i < orderList.size(); ++i) {
			orderList[i].Dump();
		}
	}
	m_keyNum = basePlan->GetKeyNum();
	m_placeNum = orderList.size();
	m_placeIdx = 0;
	m_keyIdx = 0;
	path->Init(m_placeNum + m_keyNum);

	while(m_keyIdx< m_keyNum
			and (ret=ExpandKeynode(basePlan, orderList, path),ret==0)
			and m_keyIdx < m_keyNum
			and (ret=ExpandBlock(basePlan, orderList, path),ret==0)){
	}
	if (ret != 0 || m_placeIdx < m_placeNum || m_keyIdx < m_keyNum) {
		if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::DoRoute, Expand failed!", basePlan->m_qParam.log.c_str());
		return 1;
	}
	path->m_isValid = true;
	path->Dump(basePlan);
	return 0;
}

int TrafRoute::ExpandBlock(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path) {
	int ret=0;
	const KeyNode* lastKey = basePlan->GetKey(m_keyIdx - 1);
	if (lastKey == NULL) {
		if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::Expand, m_keyIdx is out of limit", basePlan->m_qParam.log.c_str());
		return 1;
	}
	const TimeBlock* block = basePlan->GetBlock(m_keyIdx - 1);
	if(block == NULL) return 1;

	if (block->m_pInfoList.empty()) return 0;

	while (m_placeIdx < m_placeNum) {
		std::string date = orderList[m_placeIdx]._date;
		if (lastKey && date != lastKey->_trafDate) break;

		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(orderList[m_placeIdx]._place);
		if (vPlace == NULL) {
			++m_placeIdx;
			continue;
		}

		const PlaceInfo* pInfo = block->GetPlaceInfo(vPlace->_ID);
		if (pInfo == NULL) return 1;

		const DurS* durSP = basePlan->GetDurSP(vPlace);
		if (durSP ==NULL) return 1;

		ret = ExpandNode(basePlan, vPlace, durSP, NODE_FUNC_PLACE, 0, pInfo->m_arvID, pInfo->m_deptID, pInfo->m_availOpenCloseList, path);
		//若需要具体place node 的node_func_type, 需增加place->_t - node_func_type对应关系
		if (ret != 0)  return ret;

		++m_placeIdx;
	} // while

	return 0;
}


int TrafRoute::ExpandKeynode(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, PathView* path) {
	int ret = 0;

	const KeyNode* keyNode = basePlan->GetKey(m_keyIdx);
	if (m_placeIdx < m_placeNum && MJ::MyTime::compareDayStr(orderList[m_placeIdx]._date, keyNode->_trafDate) > 0) {
		if(m_dbgDump) MJ::PrintInfo::PrintDbg("[%s]TrafRoute::ExpandKeynode, miss order: %d, %s", basePlan->m_qParam.log.c_str(), m_placeIdx, orderList[m_placeIdx]._place->_ID.c_str());
		return 1;
	}

	int type = keyNode->_type;
	ret = ExpandNode(basePlan, keyNode->_place, keyNode->GetDurS(), type, 0, keyNode->_place->_ID, keyNode->_place->_ID, keyNode->m_openCloseList, path);
	if (ret != 0) return ret;

	++m_keyIdx;
	return 0;
}

int TrafRoute::ExpandNode(BasePlan* basePlan, const LYPlace* place, const DurS* durS, int nodeType, int cost, const std::string& arvID, const std::string& deptID, const std::vector<const OpenClose*> &openCloseList, PathView* path) {
	if (openCloseList.empty()) {
		if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::ExpandNode, openCloseList is empty for %s(%s)", basePlan->m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str());
		return 1;
	}
	if (!durS) {
		if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::ExpandNode, durS is NULL for %s(%s)", basePlan->m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str());
		return 1;
	}
	if(m_keyIdx == 0)
	{
		const KeyNode* keynode = basePlan->GetKey(m_keyIdx);
		path->Append(keynode->_place, keynode->_place->_ID, keynode->_place->_ID, NULL, NULL, keynode->_open, keynode->_close, keynode->GetDurS(), keynode->_trafDate, keynode->_open, keynode->_close, keynode->_cost, nodeType, basePlan->m_TimeZone);
		m_lastItem = path->GetItemLast();
		return 0;
	}

	const KeyNode* fromKey = basePlan->GetKey(m_keyIdx - 1);
	const KeyNode* toKey = basePlan->GetKey(m_keyIdx);

	// 0 最晚离开
	time_t latestDept = 0;
	if (nodeType & NODE_FUNC_PLACE) {
		const TrafficItem* deptTraf = basePlan->GetTraffic(deptID, toKey->_place->_ID, fromKey->_trafDate);
		if (!deptTraf) {
			if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::ExpandNode, no traffic: %s_%s_%s", basePlan->m_qParam.log.c_str(), deptID.c_str(), toKey->_place->_ID.c_str(), fromKey->_trafDate.c_str());
			return 1;
		}
		latestDept = toKey->_close - toKey->GetZipDur() - deptTraf->_time;
	} else {
		latestDept = toKey->_close;
	}

	// 1 到达交通
	const TrafficItem* arvTraf = basePlan->GetTraffic(m_lastItem->_deptID, arvID, fromKey->_trafDate);
	if (!arvTraf) {
		if(m_dbgDump) MJ::PrintInfo::PrintErr("[%s]TrafRoute::ExpandNode, no traffic: %s_%s_%s", basePlan->m_qParam.log.c_str(), m_lastItem->_deptID.c_str(), arvID.c_str(), fromKey->_trafDate.c_str());
		return 1;
	}

	// 2 遍历当天时间点
	for (int i = 0; i < openCloseList.size(); ++i) {
		const OpenClose* openClose = openCloseList[i];
		if (!openClose) continue;
		time_t newLatestDept = std::min(latestDept, openClose->m_close);
		time_t lastDept = m_lastItem->_departTime;
		time_t newArv = lastDept + arvTraf->_time;
		if (newArv < openClose->m_open) {
			newArv = openClose->m_open;
		}
		if (newArv > openClose->m_latestArv) {
			if(m_dbgDump) MJ::PrintInfo::PrintDbg("[%s]TrafRoute::ExpandNode, node: %s(%s), %s > %s", basePlan->m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str(), MJ::MyTime::toString(newArv, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(openClose->m_latestArv, basePlan->m_TimeZone).c_str());
			if (nodeType & NODE_FUNC_KEY) {
				newArv = openClose->m_latestArv;
			} else continue;
		}
		time_t newDept = newArv + durS->m_zip;
		if(m_dbgDump) MJ::PrintInfo::PrintDbg("[%s]TrafRoute::ExpandNode, node: %s(%s), %s + %d = (%s vs %s) --> (%s vs %s)", basePlan->m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str(),
				MJ::MyTime::toString(lastDept, basePlan->m_TimeZone).c_str(),
				arvTraf->_time,
				MJ::MyTime::toString(lastDept + arvTraf->_time, basePlan->m_TimeZone).c_str(),
				MJ::MyTime::toString(newArv, basePlan->m_TimeZone).c_str(),
				MJ::MyTime::toString(newDept, basePlan->m_TimeZone).c_str(),
				MJ::MyTime::toString(newLatestDept, basePlan->m_TimeZone).c_str());
		if (newDept > newLatestDept) {
			if (nodeType & NODE_FUNC_KEY) {
				newDept = newLatestDept;
			} else continue;
		}
		if (!(nodeType & NODE_FUNC_PLACE)) {
			newDept = newArv + durS->m_rcmd;
			if (newDept > newLatestDept) {
				newDept = newLatestDept;
			}
		}

		m_lastItem->_departTime = lastDept;
		m_lastItem->_departTraffic = arvTraf;
		path->Append(place, arvID, deptID, arvTraf, NULL, newArv, newDept, durS, basePlan->GetKey(m_keyIdx - 1)->_trafDate, openClose->m_open, openClose->m_close, 0, nodeType, basePlan->m_TimeZone);
		m_lastItem = path->GetItemLast();
		return 0;
	}
	return 1;
}
