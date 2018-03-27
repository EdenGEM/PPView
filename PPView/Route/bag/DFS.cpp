#include <iostream>
#include "Route/base/PathCross.h"
#include "Route/base/PathEval.h"
#include "MyTime.h"
#include "DFS.h"

int dfsDbg = 0;

int DFS::DoSearch(BagPlan* bagPlan, const SPath* richPath, int blockIdx, std::vector<const SPath*>& sPathList, SPathAlloc* spAlloc) {
	int ret = Init(bagPlan, richPath, blockIdx);
	if(ret != 0) return ret;
 	TimeBlock* block = bagPlan->GetBlock(blockIdx);
//	std::cerr << "jjj rich block " << blockIdx << std::endl;
	m_path[m_len++] = 0;
	m_visited[0] = true;
	Search(bagPlan, spAlloc);

	m_heap.CopyVal(sPathList);
//	std::cerr << "jjj rich block " << blockIdx << " ret: " << sPathList.size() << std::endl;
//	richPath->Dump(bagPlan);
//	for (int i = 0; i < sPathList.size(); ++i) {
//		sPathList[i]->Dump(bagPlan);
//	}
	return 0;
}

int DFS::Search(BagPlan* bagPlan, SPathAlloc* spAlloc) {
	++m_sCnt;
	if (m_sCnt > 20000) {//7个点的搜索只有5040
		//每20000次search看一下超时时间
		if (bagPlan->IsDFSTimeOut()) return 1;
		m_tCnt += 20000;
		m_sCnt = 0;
	}

	if (m_len + 1 >= m_nodeNum) {//m_len == m_nodeNum-1
		int lastIdx = m_path[m_len - 1];
		int tailIdx = m_nodeNum - 1;
		const TrafficItem* trafItem = m_traf[lastIdx * m_nodeNum + tailIdx];
		if (!trafItem) return 0;

		m_path[m_len++] = tailIdx;
		m_visited[tailIdx] = true;
		m_totDist += trafItem->_dist;
		m_nonWalkCnt += (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
		m_walkDist += (trafItem->_mode == TRAF_MODE_WALKING ? trafItem->_dist : 0);

		PushPath(bagPlan, spAlloc);

		m_totDist -= trafItem->_dist;
		m_nonWalkCnt -= (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
		m_walkDist -= (trafItem->_mode == TRAF_MODE_WALKING ? trafItem->_dist : 0);
		m_visited[tailIdx] = false;
		--m_len;
		return 0;
	}
	// 剪枝:如果在misslevel 和 clash最优的情况下仍不能放入,而score肯定会随着增加点而降分,故可以剪枝
	if (dfsDbg == 0 && !m_heap.CanPush(GetScore())) return 1;

	for (int idx = 1; idx < m_nodeNum - 1; ++idx) {
		if (m_visited[idx]) continue;
		uint8_t vType = m_vType[idx];
		if (vType & NODE_FUNC_PLACE_RESTAURANT) {
			SearchRest(bagPlan, idx, spAlloc);
		} else {
			SearchView(bagPlan, idx, spAlloc);
		}
	}
	return 0;
}

int DFS::CalClash(BagPlan* bagPlan, int& clash, int& firstL) {
	time_t offset = m_start;
	int leftRestNeed = m_blockRestNeed;
	if (dfsDbg & 2) { std::cerr << "jjj CalClash 0 " << MJ::MyTime::toString(offset, bagPlan->m_TimeZone) << std::endl; }
	for (int i = 1; i < m_len; ++i) {
		int idx = m_path[i];
		uint8_t vPos = m_vPosList[idx];
		int idxLast = m_path[i - 1];
		uint8_t vPosLast = m_vPosList[idxLast];
		const TrafficItem* trafItem = bagPlan->GetPosTraf(m_bPos, vPosLast, vPos);
		if (!trafItem) {
			MJ::PrintInfo::PrintErr("[%s]DFS::CalClash, TrafficItem lost", bagPlan->m_qParam.log.c_str());
			return 1;
		}
		time_t arv = offset + trafItem->_time;
		bool isPlanned = false;
		if (i == m_len - 1) {
			if (arv > m_stop) {
				clash = 1;
				if (dfsDbg & 4) { fprintf(stderr, "jjj arv > stop, %d/%s, %s + %d = %s/%d > %s/%d\n", i, bagPlan->GetPosPlace(vPos)->_name.c_str(), MJ::MyTime::toString(offset, bagPlan->m_TimeZone, "%R").c_str(), trafItem->_time, MJ::MyTime::toString(arv, bagPlan->m_TimeZone, "%R").c_str(), arv, MJ::MyTime::toString(m_stop, bagPlan->m_TimeZone, "%R").c_str(), m_stop); }
				return 0;
			} else {
				isPlanned = true;
				return 0;
			}
		}
		for (int j = 0; j < m_openCloseList[idx].size(); j++) {
			if (m_vType[idx] & NODE_FUNC_PLACE_RESTAURANT) {
				if (!m_openCloseList[idx][j]) continue;
				// 午/晚餐已经不需要/加过了
				// 吃饭暂时只在午餐时间和晚餐时间打开
				if (j == 0 && !(leftRestNeed & NODE_FUNC_PLACE_REST_LUNCH)) continue;
				if (j == 1 && !(leftRestNeed & NODE_FUNC_PLACE_REST_SUPPER)) continue;
			}
			time_t open = m_openCloseList[idx][j]->m_open;
			time_t close = m_openCloseList[idx][j]->m_close;
			time_t newArv = arv;
			if (newArv < open) {
				newArv = open;
			}
			if (m_zipDurList[idx] == -1) {
				MJ::PrintInfo::PrintErr("m_zipDurList < 0 ");
				return 1;
			}
			time_t newDept = newArv + m_zipDurList[idx];
			if (dfsDbg & 4) {
				fprintf(stderr, "jjj Clash, %d/%s\t"
						"%s + %d = %s vs "
						"%s + %d = %s\n",
						i, bagPlan->GetPosPlace(vPos)->_name.c_str(),
						MJ::MyTime::toString(offset, bagPlan->m_TimeZone, "%R").c_str(), trafItem->_time, MJ::MyTime::toString(arv, bagPlan->m_TimeZone, "%R").c_str(),
						MJ::MyTime::toString(newArv, bagPlan->m_TimeZone, "%R").c_str(), m_zipDurList[idx], MJ::MyTime::toString(newDept, bagPlan->m_TimeZone, "%R").c_str());
		   	}
			if (newDept > close) continue;
			if (newDept > m_stop) break;
			// 留出空白 没游玩 就去吃饭
			if (i == 1 && m_vType[idx] & NODE_FUNC_PLACE_RESTAURANT && newArv - arv > 3600) {
				firstL = 1;
			}
			offset = newDept;
			isPlanned = true;
			// 餐厅固定有长度为2的OpenClose 用于区分午晚餐
			if (m_vType[idx] & NODE_FUNC_PLACE_RESTAURANT) {
				if (j == 0) {
					leftRestNeed &= ~NODE_FUNC_PLACE_REST_LUNCH;
				} else if (j == 1) {
					leftRestNeed &= ~NODE_FUNC_PLACE_REST_SUPPER;
				}
			}
			break;
		}
		if (!isPlanned)  {
			if (dfsDbg & 2) { std::cerr << "jjj place not planned " << bagPlan->GetPosPlace(vPos)->_name << std::endl; }
			clash = 1; //放入m_heap会用到
			return 0;
		}
	//	if (!isPlanned) {
	//		const LYPlace* place = bagPlan->GetPosPlace(vPos);
	//		if (place && m_vType[idx] & NODE_FUNC_PLACE_RESTAURANT) fprintf(stderr, "hyhy find lost place %s(%s) m_openCloseChoice[idx]=%d, openClose isn't NULL?  %d in dfs calClash\n",
	//				place->_ID.c_str(), place->_name.c_str(), m_openCloseChoice[idx], m_openCloseList[idx][m_openCloseChoice[idx]] != NULL);
	//	}
	}
	return 0;
}

int DFS::PushPath(BagPlan* bagPlan, SPathAlloc* spAlloc) {
	if (dfsDbg == 0 && !m_heap.CanPush(GetScore())) return 0;

	int clash = 0;
	int firstL = 0;
	int ret = CalClash(bagPlan, clash, firstL);
	if (ret != 0) return 1;

	SPath* sPath = spAlloc->New();
	sPath->Init(bagPlan);
	for (int i = 1; i + 1 < m_len; ++i) {
		int idx = m_path[i];
		uint8_t vPos = m_vPosList[idx];
		sPath->PushV(m_bPos, vPos);
	}
	sPath->SetDist(m_totDist);
	sPath->SetClash(clash);
	sPath->SetFirstL(firstL);
	sPath->SetScore(GetScore(firstL));
	sPath->CalHashPointOrder();

	if (dfsDbg & 1) { sPath->Dump(bagPlan); }

	const SPath* dPath = m_heap.Push(sPath);
	if (dPath) {
		spAlloc->Delete(dPath);
	}
	return 0;
}

int DFS::SearchView(BagPlan* bagPlan, int idx, SPathAlloc* spAlloc) {
	int lastIdx = m_path[m_len - 1];
	const TrafficItem* trafItem = m_traf[lastIdx * m_nodeNum + idx];
	if (!trafItem) return 0;

	if (dfsDbg & 2) { fprintf(stderr, "jjj push %s\n", bagPlan->GetPosPlace(m_vPosList[idx])->_ID.c_str()); }

	m_path[m_len++] = idx;
	m_visited[idx] = true;
	m_totDist += trafItem->_dist;
	m_nonWalkCnt += (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
	m_walkDist += ((trafItem->_mode == TRAF_MODE_WALKING) ? trafItem->_dist : 0);

	Search(bagPlan, spAlloc);

	m_totDist -= trafItem->_dist;
	m_nonWalkCnt -= (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
	m_walkDist -= ((trafItem->_mode == TRAF_MODE_WALKING) ? trafItem->_dist : 0);
	m_visited[idx] = false;
	--m_len;
	if (dfsDbg & 2) { fprintf(stderr, "jjj pop %s\n", bagPlan->GetPosPlace(m_vPosList[idx])->_ID.c_str()); }
	return 0;
}

int DFS::SearchRest(BagPlan* bagPlan, int idx, SPathAlloc* spAlloc) {
	int lastIdx = m_path[m_len - 1];
	const TrafficItem* trafItem = m_traf[lastIdx * m_nodeNum + idx];
	if (!trafItem) return 0;

	if (dfsDbg & 2) { std::cerr << "jjj try push " << bagPlan->GetPosPlace(m_vPosList[idx])->_ID << std::endl; }
	uint8_t vType = m_vType[idx];
	if (!(vType & NODE_FUNC_PLACE_RESTAURANT)) {
		if (dfsDbg & 2) { std::cerr << "jjj res 0" << std::endl; }
		return 0;
	}
	if (m_leftRestNeed & NODE_FUNC_PLACE_REST_LUNCH && !(vType & NODE_FUNC_PLACE_REST_LUNCH)
			|| m_leftRestNeed == NODE_FUNC_PLACE_REST_SUPPER && !(vType & NODE_FUNC_PLACE_REST_SUPPER)) {
		if (dfsDbg & 2) { std::cerr << "jjj res 1" << std::endl; }
		return 0;
	}

	if (dfsDbg & 2) { fprintf(stderr, "jjj push %s\n", bagPlan->GetPosPlace(m_vPosList[idx])->_ID.c_str()); }

	uint8_t oriRNeed = m_leftRestNeed;
	if (m_leftRestNeed & NODE_FUNC_PLACE_REST_LUNCH) {
		m_leftRestNeed &= ~NODE_FUNC_PLACE_REST_LUNCH;
	} else if (m_leftRestNeed & NODE_FUNC_PLACE_REST_SUPPER) {
		m_leftRestNeed &= ~NODE_FUNC_PLACE_REST_SUPPER;
	} else {
		std::cerr<<"hyhy don't need rest "<<bagPlan->GetPosPlace(m_vPosList[idx])->_ID<<std::endl;
		return 0;
	}
//	const LYPlace* place = bagPlan->GetPosPlace(m_vPosList[idx]);
//	if (place && place->_t & LY_PLACE_TYPE_RESTAURANT) {
//		fprintf(stderr, "hyhy SearRest, place %s(%s), m_leftRestNeed %x, vType %x, m_openCloseChoice: %d,m_openCloseList[idx].size: %d, opcls[0]: %d, opncls[1]: %d \n",place->_ID.c_str(), place->_name.c_str(), m_leftRestNeed, vType,  m_openCloseChoice[idx], m_openCloseList[idx].size(), m_openCloseList[idx][0] != NULL, m_openCloseList[idx][1] != NULL);
//	}
	m_path[m_len++] = idx;
	m_visited[idx] = true;
	m_totDist += trafItem->_dist;
	m_nonWalkCnt += (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
	m_walkDist += (trafItem->_mode == TRAF_MODE_WALKING ? trafItem->_dist : 0);

	Search(bagPlan, spAlloc);

	m_totDist -= trafItem->_dist;
	m_nonWalkCnt -= (trafItem->_mode == TRAF_MODE_WALKING ? 0 : 1);
	m_walkDist -= (trafItem->_mode == TRAF_MODE_WALKING ? trafItem->_dist : 0);
	m_visited[idx] = false;
	--m_len;

	m_leftRestNeed = oriRNeed;
	if (dfsDbg & 2) { fprintf(stderr, "jjj pop %s\n", bagPlan->GetPosPlace(m_vPosList[idx])->_ID.c_str()); }
	return 0;
}

int DFS::Init(BagPlan* bagPlan, const SPath* richPath, int blockIdx) {
	m_nodeNum = 0;
	m_len = 0;
	m_bPos = blockIdx;
	m_totDist = 0;
	m_nonWalkCnt = 0;
	m_walkDist = 0;
	m_sCnt = 0;
	m_tCnt = 0;
	// 天内只取最优
	m_heap.Init(BagParam::m_dfsDayHeapLimit);


	std::vector<uint8_t> vPosList;
	for (int i = 0; i < richPath->m_len; ++i) {
		uint8_t vPos = richPath->GetVPos(i);
		uint8_t bPos = richPath->GetBPos(vPos);
		if (bPos == m_bPos) {
			vPosList.push_back(vPos);
		}
	}
	std::sort(vPosList.begin(), vPosList.end());
	if (vPosList.size()+2 > BagParam::m_dayNodeLimit) {
		MJ::PrintInfo::PrintErr("[%s]DFS::Init4Clash, nodeNum overflow:%d", bagPlan->m_qParam.log.c_str(), vPosList.size()+2);
		return 1;
	}
	m_vPosList[m_nodeNum++] = m_bPos;
	for (int i = 0; i < vPosList.size(); ++i) {
		m_vPosList[m_nodeNum++] = vPosList[i];
	}
	m_vPosList[m_nodeNum++] = m_bPos + 1;
	int firstRestIdx = -1;
	int secondRestIdx = -1;
	for (int i = 0; i < m_nodeNum; ++i) {
		uint8_t vPos = m_vPosList[i];
		const LYPlace* place = bagPlan->GetPosPlace(vPos);
		//m_bPos的左右边keynode的下标分别为m_bPos,m_bPos+1
		uint8_t vType = bagPlan->GetVType(m_bPos, vPos);
		m_vType[i] = vType;
		if (m_vType[i] & NODE_FUNC_PLACE_RESTAURANT) {
			if (firstRestIdx < 0) {
				firstRestIdx = i;
			} else {
				secondRestIdx = i;
			}
		}
	}
	if (firstRestIdx >= 0 && secondRestIdx >= 0) {
		uint8_t& firstRestType = m_vType[firstRestIdx];
		uint8_t& secondRestType = m_vType[secondRestIdx];
		if (firstRestType == NODE_FUNC_PLACE_REST_LUNCH) {
			secondRestType = NODE_FUNC_PLACE_REST_SUPPER;
		} else if (firstRestType == NODE_FUNC_PLACE_REST_SUPPER) {
			secondRestType = NODE_FUNC_PLACE_REST_LUNCH;
		} else if (secondRestType == NODE_FUNC_PLACE_REST_LUNCH) {
			firstRestType = NODE_FUNC_PLACE_REST_SUPPER;
		} else if (secondRestType == NODE_FUNC_PLACE_REST_SUPPER) {
			firstRestType = NODE_FUNC_PLACE_REST_LUNCH;
		}
	}
	m_blockRestNeed = NODE_FUNC_NULL;
	if (firstRestIdx >= 0) {
		m_blockRestNeed |= m_vType[firstRestIdx];
	}
	if (secondRestIdx >= 0) {
		m_blockRestNeed |= m_vType[secondRestIdx];
	}
	m_leftRestNeed = m_blockRestNeed;
//	for (int i = 0; i < m_nodeNum; ++i) {
//		fprintf(stderr, "jjj %d %d\n", i, m_vPosList[i]);
//	}

	for (int i = 0; i < m_nodeNum; ++i) {
		uint8_t vPosA = m_vPosList[i];
		for (int j = 0; j < m_nodeNum; ++j) {
			uint8_t vPosB = m_vPosList[j];
			m_traf[i * m_nodeNum + j] = bagPlan->GetPosTraf(m_bPos, vPosA, vPosB);
		}
	}

	for (int i = 0; i < m_nodeNum; ++i) {
		m_visited[i] = false;
	}
	return Init4Clash(bagPlan,blockIdx);
}

int DFS::Init4Clash(BagPlan* bagPlan, int blockIdx) {
	const TimeBlock* block = bagPlan->GetBlock(blockIdx);
	const KeyNode* fromKey = bagPlan->GetKey(blockIdx);
	const KeyNode* toKey = bagPlan->GetKey(blockIdx + 1);
	if (block == NULL || fromKey == NULL || toKey == NULL) {
		MJ::PrintInfo::PrintErr("[%s]DFS::Init4Clash, timeBlock/keynode is NULL when blockIdx = %d", bagPlan->m_qParam.log.c_str(), blockIdx);
		return 1;
	}
	m_openCloseList.resize(m_nodeNum, std::vector<const OpenClose*>());
	for (int i = 0; i < m_nodeNum; ++i) {
		uint8_t vPos = m_vPosList[i];
		const LYPlace* place = bagPlan->GetPosPlace(vPos);
		if (place == NULL) {
			MJ::PrintInfo::PrintErr("[%s]DFS::Init4Clash, place is NULL", bagPlan->m_qParam.log.c_str());
			return 1;
		}
		if (place->_t & (LY_PLACE_TYPE_VIEW | LY_PLACE_TYPE_RESTAURANT | LY_PLACE_TYPE_SHOP | LY_PLACE_TYPE_TOURALL | LY_PLACE_TYPE_VIEWTICKET) ) {
			m_zipDurList[i] = bagPlan->GetZipDur(place);
			//MJ::PrintInfo::PrintDbg("zipDur  place ID = %s, name = %s ", place->_ID.c_str(),  place->_name.c_str());
		} else {
			m_zipDurList[i] = -1;
		}
		const PlaceInfo* pInfo = block->GetPlaceInfo(place->_ID);
		if (pInfo) {
			m_openCloseList[i].assign(pInfo->m_availOpenCloseList.begin(), pInfo->m_availOpenCloseList.end());
		}
	}
	m_start = fromKey->_close;
	m_stop = toKey->_close - toKey->GetZipDur();
	return 0;
}

// 每多走1000m 加1公里
int DFS::GetScore(int firstL) {
	if (dfsDbg & 1) { fprintf(stderr, "jjj d: %d, nonWalk: %d, walkDist: %d\n", m_totDist, m_nonWalkCnt, m_walkDist); }
	return -1 * (m_totDist + std::max(m_nonWalkCnt - 1, 0) * PathEval::m_nonWalkPenalty + std::max(m_walkDist - PathEval::m_maxWalkDay, 0) + firstL * PathEval::m_firstLPenalty);

//	return -1 * (m_totDist + m_nonWalkCnt * PathEval::m_nonWalkPenalty + std::max((m_walkDist - PathEval::m_maxWalkDay) * 2 / 500 * 500, 0));
}
