#include <iostream>
#include <math.h>
#include "BagPlan.h"

bool BagPlan::m_track = false;

int BagPlan::AdaptOrder(const SPath* sPath, std::vector<PlaceOrder>& orderList) {
	if (!sPath) return 1;

	int lastBPos = -1;
	std::string date;
	std::vector<std::pair<int, int> > restIdxList(GetBlockNum(), std::pair<int, int>(-1, -1));
	for (int i = 0; i < sPath->m_len; ++i) {
		uint8_t vPos = sPath->GetVPos(i);
		uint8_t bPos = sPath->GetBPos(vPos);
		uint8_t vType = GetVType(bPos, vPos);
		if (vType & NODE_FUNC_PLACE_RESTAURANT) {
			if (bPos >= restIdxList.size()) {
				MJ::PrintInfo::PrintErr("[%s]BagPlan::AdaptOrder, bPos: %d is out of blockNum", m_qParam.log.c_str(), bPos);
				return 1;
			}
			if (restIdxList[bPos].first < 0) {
				restIdxList[bPos].first = i;
			} else if (restIdxList[bPos].second < 0) {
				restIdxList[bPos].second = i;
			}
		}
	}
	for (int i = 0; i < sPath->m_len; ++i) {
		uint8_t vPos = sPath->GetVPos(i);
		uint8_t bPos = sPath->GetBPos(vPos);
		unsigned char nodeType = NODE_FUNC_PLACE_VIEW_SHOP;
		if (bPos != lastBPos) {
			const KeyNode* keynode = GetKey(bPos);
			date = keynode->_close_date;
			lastBPos = bPos;
		}
		const LYPlace* place = GetPosPlace(vPos);
		uint8_t vType = GetVType(bPos, vPos);
		if (vType & NODE_FUNC_PLACE_RESTAURANT) {
			if (i == restIdxList[bPos].first) {
				if (restIdxList[bPos].second >= 0) {
					nodeType = NODE_FUNC_PLACE_REST_LUNCH;
				} else {
					nodeType = vType;
				}
			} else if (i == restIdxList[bPos].second) {
				nodeType = NODE_FUNC_PLACE_REST_SUPPER;
			} else {
				// >2餐厅情况，当view规划，不应出现
				nodeType = NODE_FUNC_PLACE_VIEW_SHOP;
				MJ::PrintInfo::PrintErr("[%s]BagPlan::AdaptOrder, more than 3 rest in block: %d", m_qParam.log.c_str(), bPos);
				return 1;
			}
		}
		orderList.push_back(PlaceOrder(place, date, orderList.size(), nodeType));
	}
	return 0;
}

double BagPlan::NormScore(int hot, int dist, int time) {
	if (dist > m_maxDist) return 0.0;
	if (time > m_maxTime) return 0.0;
	double hotN = 0;
	if (m_maxHot > 0) {
		hotN = std::min(3 * (hot - (m_maxHot + 0) / 2.0) / ((m_maxHot - 0) / 2.0), 700.0);
	}
	double distN = 0;
	if (m_maxDist > 0) {
		distN = std::min(3 * (dist - (m_maxDist + 0) / 2.0) / ((m_maxDist - 0) / 2.0), 700.0);
	}
	double timeN = 0;
	if (m_maxTime > 0) {
		timeN = std::min(3 * (hot - (m_maxTime + 0) / 2.0) / ((m_maxTime - 0) / 2.0), 700.0);
	}
	double hotS = 1.0 / (1 + exp(-1 * hotN));
	double distS = 1.0 / (1 + exp(distN));
	double timeS = 1.0 / (1 + exp(timeN));
//	return (0.75 * hotS + 0.2 * distS + 0.05 * timeS);
	return (0.8 * hotS + 0.2 * distS);

//	double hotS = hot * 1.0 / m_maxHot;
//	double distS = log(m_maxDist - dist) / log(m_maxDist);
//	double timeS = log(m_maxTime - time) / log(m_maxTime);
//	fprintf(stderr, "jjj norm\t%.5f\t%.5f\t%.5f\n", hotS, distS, timeS);
//	return (0.3 * hotS + 0.6 * distS + 0.1 * timeS);
}

int BagPlan::SetMaxHot(int maxHot) {
	if (maxHot > 0) {
		m_maxHot = maxHot;
	}
	return 0;
}

int BagPlan::SetMaxDist(int maxDist) {
	if (maxDist > 0) {
		m_maxDist = maxDist;
	}
	return 0;
}

int BagPlan::SetMaxTime(int maxTime) {
	if (maxTime > 0) {
		m_maxTime = maxTime;
	}
	return 0;
}

int BagPlan::MaxHot() const {
	return m_maxHot;
}

int BagPlan::MaxDist() const {
	return m_maxDist;
}

int BagPlan::MaxTime() const {
	return m_maxTime;
}

int BagPlan::DoPos() {
	m_posPList.clear();
	for (int i = 0; i < GetKeyNum(); ++i) {
		const KeyNode* keynode = GetKey(i);
		if (keynode->_place) {
			m_posPList.push_back(keynode->_place);
			m_isPosNonMiss.push_back(false);
		}
	}
	m_kNum = m_posPList.size();
	for (int i = 0; i < m_waitPlaceList.size(); ++i) {
		const LYPlace* place = m_waitPlaceList[i];
		if (place) {
			m_posPList.push_back(place);
			if (m_userMustPlaceSet.count(place)) {
				m_isPosNonMiss.push_back(true);
			} else {
				m_isPosNonMiss.push_back(false);
			}
		}
	}
	if (m_posPList.size() > SPath::kPathLimit) {
		MJ::PrintInfo::PrintErr("[%s]BagPlan::DoPos, too more pos place %d vs %d!!", m_qParam.log.c_str(), m_posPList.size(), SPath::kPathLimit);
		m_posPList.erase(m_posPList.begin() + SPath::kPathLimit, m_posPList.end());
		m_isPosNonMiss.erase(m_isPosNonMiss.begin() + SPath::kPathLimit, m_isPosNonMiss.end());
	}
	m_nodeNum = m_posPList.size();
/*jjj
	for (int i = 0; i < m_posPList.size(); ++i) {
		std::cerr << "jjj " << i << " " << m_posPList[i]->_name << std::endl;
	}
	for (int i = 0; i < m_waitPlaceList.size(); ++i) {
		const LYPlace* place = m_waitPlaceList[i];
		std::cerr << "jjj " << i << " " << place->_name << std::endl;
	}
jjj*/

	m_blockNum = GetBlockNum();
	if (m_blockNum > SPath::kBlockLimit) {
		MJ::PrintInfo::PrintErr("[%s]BagPlan::DoPos, too more block %d vs %d!!", m_qParam.log.c_str(), m_blockNum, SPath::kBlockLimit);
		return 1;
	}

	m_vTypeList.clear();
	m_vTypeList.resize(m_blockNum * m_nodeNum, NODE_FUNC_NULL);
	for (int blockIdx = 0; blockIdx < m_blockNum; ++blockIdx) {
		const TimeBlock* block = GetBlock(blockIdx);
		for (int nodeIdx = 0; nodeIdx < m_nodeNum; ++nodeIdx) {
			const LYPlace* place = m_posPList[nodeIdx];
			if (block->Has(place->_ID)) {
				unsigned int nodeFunc = block->GetFunc(place->_ID);
				uint8_t nodeType = NODE_FUNC_PLACE_VIEW_SHOP;
				if (nodeFunc & NODE_FUNC_PLACE_VIEW_SHOP) {
					nodeType = NODE_FUNC_PLACE_VIEW_SHOP;
				} else if (nodeFunc == NODE_FUNC_PLACE_REST_LUNCH) {
					nodeType = NODE_FUNC_PLACE_REST_LUNCH;
				} else if (nodeFunc == NODE_FUNC_PLACE_REST_SUPPER) {
					nodeType = NODE_FUNC_PLACE_REST_SUPPER;
				} else if (nodeFunc & NODE_FUNC_PLACE_REST_LUNCH && nodeFunc & NODE_FUNC_PLACE_REST_SUPPER) {
					nodeType = NODE_FUNC_PLACE_RESTAURANT;
				}
				m_vTypeList[blockIdx * m_nodeNum + nodeIdx] = nodeType;
			}
		}
	}

	m_posMap.clear();
	for (int i = m_kNum; i < m_nodeNum; ++i) {
		const LYPlace* place = m_posPList[i];
		m_posMap[place->_ID] = i;
	}

	std::tr1::unordered_map<std::string, int> pIdxMap;
	for (int i = 0; i < m_nodeNum; ++i) {
		const LYPlace* place = m_posPList[i];
		if (place->_t & LY_PLACE_TYPE_TOURALL) {
			const Tour* tour = dynamic_cast<const Tour*>(place);
			for (std::tr1::unordered_set<std::string>::const_iterator it = tour->m_refPoi.begin(); it != tour->m_refPoi.end(); ++it) {
				const std::string id = *it;
				if (pIdxMap.find(id) == pIdxMap.end()) {
					pIdxMap[id] = pIdxMap.size();
				}
			}
		}
		if (pIdxMap.find(place->_ID) == pIdxMap.end()) {
			pIdxMap[place->_ID] = pIdxMap.size();
		}
	}
	if (pIdxMap.size() > 160) {
		MJ::PrintInfo::PrintErr("[%s]BagPlan::DoPos, too more wait place: %d > 160", m_qParam.log.c_str(), pIdxMap.size());
	}

	m_bidMap.clear();
	m_bidPList.clear();
	m_bidPList.resize(m_nodeNum);
	m_richBid.reset();
	m_userBid.reset();
	for (int i = 0; i < m_nodeNum; ++i) {
		const LYPlace* place = m_posPList[i];
		bit160 bid;
		if (place->_t & LY_PLACE_TYPE_TOURALL) {
			const Tour* tour = dynamic_cast<const Tour*>(place);
			for (auto it = tour->m_refPoi.begin(); it != tour->m_refPoi.end(); ++it) {
				std::string id = (*it);
				std::tr1::unordered_map<std::string, int>::iterator _it = pIdxMap.find(id);
				if (_it != pIdxMap.end() && _it->second < 160) {
					bid.set(_it->second);
				}
			}
		}
		std::tr1::unordered_map<std::string, int>::iterator it = pIdxMap.find(place->_ID);
		if (it != pIdxMap.end() && it->second < 160) {
			bid.set(it->second);
		}

		m_bidMap[place->_ID] = bid;
		m_bidPList[i] = bid;
		if (m_userMustPlaceSet.count(place)) {
			m_userBid |= bid;
			const View* viewS = dynamic_cast<const View*>(place);
			m_richBid |= bid;
		}
	}
	return 0;
}

const LYPlace* BagPlan::GetPosPlace(uint8_t vPos) {
	if (vPos >= m_nodeNum) return NULL;
	return m_posPList[vPos];
}

bool BagPlan::IsPosNonMiss(uint8_t vPos) {
	if (vPos >= m_nodeNum) return NULL;
	return m_isPosNonMiss[vPos];
}

int BagPlan::GetPosMissLevel(uint8_t vPos) {
	if (vPos >= m_nodeNum) return NULL;
	const LYPlace* place = m_posPList[vPos];
	if (!place) return 0;
	return GetMissLevel(place);
}

bit160 BagPlan::GetPosBid(uint8_t vPos) {
	if (vPos >= m_nodeNum) return 0;
	return m_bidPList[vPos];
}

const TrafficItem* BagPlan::GetPosTraf(uint8_t bPos, uint8_t iPos, uint8_t jPos) {
	if (bPos >= m_blockNum || iPos >= m_nodeNum || jPos >= m_nodeNum) return NULL;
	std::string fromID = m_posPList[iPos]->_ID;
	std::string toID = m_posPList[jPos]->_ID;
	const TimeBlock* block = GetBlock(bPos);
	if (!fromID.empty() && !toID.empty() && block) {
		return GetTraffic(fromID, toID, block->_trafDate);
	}
	return NULL;
}

uint8_t BagPlan::GetPos(const std::string& id) {
	std::tr1::unordered_map<std::string, int>::iterator it = m_posMap.find(id);
	if (it == m_posMap.end()) return 0;
	return m_posMap[id];
}

int BagPlan::PosNum() const {
	return m_nodeNum;
}

uint8_t BagPlan::GetVType(uint8_t bPos, uint8_t vPos) {
	if (bPos >= m_blockNum || vPos >= m_nodeNum) {
		MJ::PrintInfo::PrintErr("[%s]BagPlan::GetVFunc, %d vs %d, %d vs %d!!", m_qParam.log.c_str(), bPos, m_blockNum, vPos, m_nodeNum);
		return NODE_FUNC_NULL;
	}
	return m_vTypeList[bPos * m_nodeNum + vPos];
}

bool BagPlan::IsRootTimeOut() {
	if (m_rootTimer.cost() / 1000 > BagParam::m_rootTimeOut || IsTimeOut()) {
		m_bagStat.m_rootTimeOut  = true;
		return true;
	}
	return false;
}
bool BagPlan::IsRichTimeOut() {
	if (m_richTimer.cost() / 1000 > BagParam::m_richTimeOut || IsTimeOut()) {
		m_bagStat.m_richTimeOut  = true;
		return true;
	}
	return false;
}
bool BagPlan::IsDFSTimeOut() {
	if (m_dfsTimer.cost() / 1000 > BagParam::m_dfsTimeOut || IsTimeOut()) {
		m_bagStat.m_dfsTimeOut  = true;
		return true;
	}
	return false;
}
bool BagPlan::IsRouteTimeOut() {
	if (m_routeTimer.cost() / 1000 > BagParam::m_routeTimeOut || IsTimeOut()) {
		m_bagStat.m_routeTimeOut  = true;
		return true;
	}
	return false;
}
