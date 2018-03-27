#include <iostream>
#include "MJCommon.h"
#include "BagPlan.h"
#include "SPath.h"

uint8_t SPath::GetVPos(int i) const {
	if (i >= m_len) {
		MJ::PrintInfo::PrintErr("SPath::GetVPos, %d vs %d", i, m_len);
		return 0;
	}
	return m_vPos[i];
}

uint8_t SPath::GetBPos(uint8_t vPos) const {
	if (vPos >= m_nodeNum) {
		MJ::PrintInfo::PrintErr("SPath::GetBPos, %d vs %d", vPos, m_nodeNum);
		return 0;
	}
	return m_bPos[vPos];
}

int SPath::GetDur(uint8_t bPos) const {
	if (bPos >= m_blockNum) {
		MJ::PrintInfo::PrintErr("SPath::GetDur, %d vs %d", bPos, m_blockNum);
		return 0;
	}
	return m_durList[bPos];
}

uint8_t SPath::GetRNeed(uint8_t bPos) const {
	if (bPos >= m_blockNum) {
		MJ::PrintInfo::PrintErr("SPath::GetRNeed, %d vs %d", bPos, m_blockNum);
		return 0;
	}
	return m_rNeedList[bPos];
}

int8_t SPath::GetRNum(uint8_t bPos) const {
	if (bPos >= m_blockNum) {
		MJ::PrintInfo::PrintErr("SPath::GetRNeed, %d vs %d", bPos, m_blockNum);
		return 0;
	}
	return m_rNumList[bPos];
}

uint8_t SPath::GetVNum(uint8_t bPos) const {
	if (bPos >= m_blockNum) {
		MJ::PrintInfo::PrintErr("SPath::GetVNum, %d vs %d", bPos, m_blockNum);
		return 0;
	}
	return m_vNumList[bPos];
}

int SPath::ForwardNext() const {
	++m_next;
	return 0;
}

int SPath::AddMissLevel(int missLevel) const {
	m_missLevel += missLevel;
	return 0;
}

int SPath::SetMissLevel(int missLevel) {
	m_missLevel = missLevel;
	return 0;
}

int SPath::SetScore(double score) {
	m_score = score;
	return 0;
}

int SPath::SetDist(int dist) {
	m_dist = dist;
	return 0;
}

int SPath::SetHot(int hot) {
	m_hot = hot;
	return 0;
}

int SPath::SetCross(int cross) {
	m_cross = cross;
	return 0;
}

int SPath::SetClash(int clash) {
	m_clash = clash;
	return 0;
}

int SPath::SetFirstL(int firstL) {
	m_firstL = firstL;
	return 0;
}
int SPath::DumpDetial(BagPlan* bagPlan, bool log) const {
	std::vector<std::vector<uint8_t> > posList(m_blockNum, std::vector<uint8_t>());
	int totDur = 0;
	int usedDur = 0;
	for(int i = 0; i < bagPlan->GetBlockNum(); ++i) {
		totDur += bagPlan->GetBlock(i)->_avail_dur;
	}
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = m_vPos[i];
		uint8_t bPos = m_bPos[vPos];
		if (bPos >= 0 && bPos < m_blockNum) {
			posList[bPos].push_back(vPos);
		}
	}
	MJ::PrintInfo::PrintLog("SPath::Dump, len: %d, miss: %d, hot: %d, score: %.2f, dist: %d, cross: %d, clash: %d hash: %d, hashFrom: %d, hashSet:%d", m_len, m_missLevel, m_hot, m_score, m_dist, m_cross, m_clash, m_hash, m_hashFrom, m_hashPointSet);
	for (uint8_t bPos = 0; bPos < posList.size(); ++bPos) {
		int blockDur = bagPlan->GetBlock(bPos)->_avail_dur;
		int blockLeftDur = GetDur(bPos);//当前block中仍可用的时间,即剩下的时间
		double usedRatio = (blockDur - static_cast<double>(blockLeftDur))/(blockDur+0.000001);//防止blockDur为0
		usedDur += blockDur - blockLeftDur;
		MJ::PrintInfo::PrintLog("SPath::Dump, Block:%d\tDur:%.2f\tLeftDur:%.2f\tRatio:%.2f%%,notPlay:%d",bPos,blockDur/3600.0,blockLeftDur/3600.0,usedRatio*100,bagPlan->GetBlock(bPos)->IsNotPlay());
	}
	MJ::PrintInfo::PrintLog("SPath::Dump, total Ratio:%.2f%%",static_cast<double>(usedDur)/totDur*100);
	for (uint8_t bPos = 0; bPos < posList.size(); ++bPos) {
		const KeyNode* keynode = bagPlan->GetKey(bPos);
		MJ::PrintInfo::PrintLog("SPath::Dump, %s(%s)", keynode->_place->_ID.c_str(), keynode->_place->_name.c_str());
		std::string sepLine;
		for (uint8_t ii = 0; ii < posList.size(); ++ii) {
			if (ii == bPos){
				sepLine += "O";
			} else {
				sepLine += "-";
			}
		}
		MJ::PrintInfo::PrintLog("SPath::Dump, %s",sepLine.c_str());
		std::vector<uint8_t>& vPosList = posList[bPos];
		for (int i = 0; i < vPosList.size(); ++i) {
			uint8_t vPos = vPosList[i];
			std::string openInfo;
			for (uint8_t ii = 0; ii < posList.size(); ++ii) {
				if (bagPlan->GetVType(ii, vPos) == NODE_FUNC_NULL) {
					openInfo += "-";
				}else {
					openInfo += "O";
				}
			}
			const LYPlace* place = bagPlan->GetPosPlace(vPos);
			MJ::PrintInfo::PrintLog("SPath::Dump, %s\t%s(%s)",openInfo.c_str(), place->_ID.c_str(), place->_name.c_str());
		}
		if (bPos + 1 == posList.size()) {
			keynode = bagPlan->GetKey(bPos + 1);
			MJ::PrintInfo::PrintLog("SPath::Dump, %s(%s)", keynode->_place->_ID.c_str(), keynode->_place->_name.c_str());
		}
	}
	return 0;
}


int SPath::Dump(BagPlan* bagPlan, bool log) const {
//	for (int i = 0; i < m_len; ++i) {
//		fprintf(stderr, "%d %d\n", m_vPos[i], v_bPos[m_vPos[i]]);
//	}
	std::vector<std::vector<uint8_t> > posList(m_blockNum, std::vector<uint8_t>());
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = m_vPos[i];
		uint8_t bPos = m_bPos[vPos];
		if (bPos >= 0 && bPos < m_blockNum) {
			posList[bPos].push_back(vPos);
		}
	}
	MJ::PrintInfo::PrintLog("SPath::Dump, len: %d, miss: %d, hot: %d, score: %.2f, dist: %d, cross: %d, clash: %d, firstL: %d, hash: %d, hashFrom: %d", m_len, m_missLevel, m_hot, m_score, m_dist, m_cross, m_clash, m_firstL, m_hash, m_hashFrom);
	std::vector<std::string> nameList;
	for (uint8_t bPos = 0; bPos < posList.size(); ++bPos) {
		const KeyNode* keynode = bagPlan->GetKey(bPos);
		MJ::PrintInfo::PrintLog("SPath::Dump, %s(%s)", keynode->_place->_ID.c_str(), keynode->_place->_name.c_str());
		nameList.push_back(keynode->_place->_name);
		std::vector<uint8_t>& vPosList = posList[bPos];
		for (int i = 0; i < vPosList.size(); ++i) {
			uint8_t vPos = vPosList[i];
			const LYPlace* place = bagPlan->GetPosPlace(vPos);
			MJ::PrintInfo::PrintLog("SPath::Dump, %s(%s)", place->_ID.c_str(), place->_name.c_str());
			nameList.push_back(place->_name);
		}
		if (bPos + 1 == posList.size()) {
			keynode = bagPlan->GetKey(bPos + 1);
			MJ::PrintInfo::PrintLog("SPath::Dump, %s(%s)", keynode->_place->_ID.c_str(), keynode->_place->_name.c_str());
			nameList.push_back(keynode->_place->_name);
		}
	}
	MJ::PrintInfo::PrintLog("SPath::Dump, %s", ToolFunc::join2String(nameList, "_").c_str());
	return 0;
}

int SPath::PushV(uint8_t bPos, uint8_t vPos, int hot, int dur, bit160 bid, uint8_t vType) {
	if (bPos >= m_blockNum || vPos >= m_nodeNum) {
		MJ::PrintInfo::PrintErr("SPath::PushV, %d vs %d, %d vs %d", bPos, m_blockNum, vPos, m_nodeNum);
		return 1;
	}
	if (m_len >= m_nodeNum) {
		MJ::PrintInfo::PrintErr("SPath::PushV, %d vs %d", m_len, m_nodeNum);
		return 1;
	}
	m_vPos[m_len++] = vPos;
	m_bPos[vPos] = bPos;
	m_durList[bPos] -= dur;
	if (!(vType & NODE_FUNC_PLACE_RESTAURANT)) {
		++m_vNumList[bPos];
	}
	m_bid |= bid;
	m_hot += hot;
	if (vType & NODE_FUNC_PLACE_RESTAURANT) {
		--m_rNumList[bPos];
	//	if (m_rNeedList[bPos] & NODE_FUNC_PLACE_REST_LUNCH && vType & NODE_FUNC_PLACE_REST_LUNCH) {
	//		m_rNeedList[bPos] &= ~NODE_FUNC_PLACE_REST_LUNCH;
	//	} else if (m_rNeedList[bPos] & NODE_FUNC_PLACE_REST_SUPPER && vType & NODE_FUNC_PLACE_REST_SUPPER) {
	//		m_rNeedList[bPos] &= ~NODE_FUNC_PLACE_REST_SUPPER;
	//	}
		if (vType == NODE_FUNC_PLACE_REST_LUNCH || vType == NODE_FUNC_PLACE_REST_SUPPER) {
			m_rNeedList[bPos] &= ~vType;
		}
	}
	return 0;
}

int SPath::Clear() const {
	m_isInit = false;
	return 0;
}

int SPath::Copy(const SPath* sp) {
	m_nodeNum = sp->m_nodeNum;
	m_blockNum = sp->m_blockNum;
	m_len = sp->m_len;
	m_hot = sp->m_hot;
	m_dist = sp->m_dist;
	m_cross = sp->m_cross;
	m_clash = sp->m_clash;
	m_firstL = sp->m_firstL;
	m_score = sp->m_score;

	m_next = sp->m_next;
	m_missLevel = sp->m_missLevel;
	m_hash = sp->m_hash;
	m_hashPointSet = sp->m_hashPointSet;
	m_hashFrom = sp->m_hashFrom;
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = sp->m_vPos[i];
		m_vPos[i] = vPos;
		m_bPos[vPos] = sp->m_bPos[vPos];
	}
	for (int i = 0; i < m_blockNum; ++i) {
		m_durList[i] = sp->m_durList[i];
		m_vNumList[i] = sp->m_vNumList[i];
		m_rNeedList[i] = sp->m_rNeedList[i];
		m_rNumList[i] = sp->m_rNumList[i];
	}

	m_bid = sp->m_bid;
	m_isInit = true;
	return 0;
}

int SPath::Init(BagPlan* bagPlan) {
	m_nodeNum = bagPlan->m_nodeNum;
	m_blockNum = bagPlan->GetBlockNum();
	m_len = 0;
	m_hot = 0;
	m_dist = 0;
	m_cross = 0;
	m_score = 0;
	m_clash = 0;
	m_firstL = 0;

	m_next = 0;
	m_missLevel = 0;
	m_hash = 0;
	m_hashPointSet = 0;
	m_hashFrom = 0;
	for (int i = 0; i < m_blockNum; ++i) {
		TimeBlock* block = bagPlan->GetBlock(i);
		m_durList[i] = block->_avail_dur;
		m_vNumList[i] = 0;
		m_rNeedList[i] = block->_restNeed;
		m_rNumList[i] = block->_restNum;
	}

	m_bid.reset();
	m_isInit = true;
	return 0;
}

uint32_t SPath::Hash() const {
	return m_hash;
}

uint32_t SPath::HashPointSet() const {
	return m_hashPointSet;
}

uint32_t SPath::Hash(std::vector<uint32_t>& tVec) const {
	uint32_t hash = 0;
	for (int i = 0; i < tVec.size(); ++i) {
		if (i & 1) {
			hash ^= (~((hash << 11) ^ (tVec[i]) ^ (hash >> 5)));
		} else {
			hash ^= ((hash << 7) ^ (tVec[i]) ^ (hash >> 3));
		}
	}
	return (hash & 0x7FFFFFFF);
}

uint32_t SPath::Hash(bit160& tVec){
	uint32_t hash = 0;
	for (int i = 0; i < tVec.size(); ++i) {
		if (i & 1) {
			hash ^= (~((hash << 11) ^ (tVec[i]) ^ (hash >> 5)));
		} else {
			hash ^= ((hash << 7) ^ (tVec[i]) ^ (hash >> 3));
		}
	}
	return (hash & 0x7FFFFFFF);
}

//order是指block是顺序的,且每个block里的景点是按索引顺序的
int SPath::CalHashPointOrder() const {
	if (m_hash) return 0;
	std::vector<std::vector<uint8_t> > posList(m_blockNum, std::vector<uint8_t>());
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = m_vPos[i];
		uint8_t bPos = m_bPos[vPos];
		if (bPos >= 0 && bPos < m_blockNum) {
			posList[bPos].push_back(vPos);
		}
	}
	std::vector<uint32_t> vPosList;
	for (int i = 0; i < posList.size(); ++i) {
		std::vector<uint8_t>& dayPosList = posList[i];
		for (int j = 0; j < dayPosList.size(); ++j) {
			vPosList.push_back(dayPosList[j]);
		}
		vPosList.push_back(0);
	}
	m_hash = Hash(vPosList);
	return 0;
}

int SPath::CalHashPointSet(BagPlan* bagPlan) const {
	if (m_hashPointSet) return 0;
	bit160 vPathBid;
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = m_vPos[i];
		vPathBid |= bagPlan->GetPosBid(vPos);
	}
	m_hashPointSet = Hash(vPathBid);
	return 0;
}

int SPath::CalHashDayOrder(BagPlan* bagPlan) const {
	if (m_hash) return 0;
   	if (!bagPlan) return 0;
	std::vector<bit160> dayBidList(m_blockNum);
	for (int i = 0; i < m_len; ++i) {
		uint8_t vPos = m_vPos[i];
		uint8_t bPos = m_bPos[vPos];
		dayBidList[bPos] |= bagPlan->GetPosBid(vPos);
	}
	std::vector<uint32_t> dayHashList;
	for (int i = 0; i < dayBidList.size(); ++i) {
		uint32_t dayHash = Hash(dayBidList[i]);
		dayHashList.push_back(dayHash);
	}
	m_hash = Hash(dayHashList);
	return 0;
}

int SPath::SetHashFrom(uint32_t hashFrom) const {
	m_hashFrom = hashFrom;
}
