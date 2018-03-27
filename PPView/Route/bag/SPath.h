#ifndef _SPATH_H_
#define _SPATH_H_

#include <iostream>
#include <vector>
#include "Route/base/define.h"

class BagPlan;

// bfs过程中不断复制修改
class SPath {
public:
	SPath() {
		m_nodeNum = 0;
		m_blockNum = 0;
		m_hot = 0;
		m_dist = 0;
		m_cross = 0;
		m_score = 0;
		m_len = 0;
		m_next = 0;
		m_missLevel = 0;
		m_isInit = false;
		m_bid.reset();
		m_hash = 0;
		m_hashPointSet = 0;
		m_hashFrom = 0;
		m_clash = 0;
		m_firstL = 0;

		m_hotValue = 0;
		m_trafDist = 0;
		m_trafDur = 0;
		m_nonWalkCnt = 0;
		m_walkPlus = 0;
	}
	SPath(const SPath* ptr) {
		Copy(ptr);
	}
	uint8_t GetVPos(int i) const;
	uint8_t GetBPos(uint8_t vPos) const;
	int GetDur(uint8_t bPos) const;
	uint8_t GetRNeed(uint8_t bPos) const;
	int8_t GetRNum(uint8_t bPos) const;
	uint8_t GetVNum(uint8_t bPos) const;
	int ForwardNext() const;
	int AddMissLevel(int missLevel) const;
	int SetMissLevel(int missLevel);
	int SetScore(double score);
	int SetDist(int dist);
	int SetHot(int hot);
	int SetCross(int cross);
	int SetClash(int clash);
	int SetFirstL(int firstL);
	int DumpDetial(BagPlan* bagPlan, bool log = false) const;
	int Dump(BagPlan* bagPlan, bool log = false) const;
	int PushV(uint8_t bPos, uint8_t vPos, int hot = 0, int dur = 0, bit160 bid = 0, uint8_t vType = NODE_FUNC_NULL);
	int Clear() const;
	int Copy(const SPath* sp);
	int Init(BagPlan* bagPlan);
	uint32_t Hash() const;
	uint32_t HashFrom() const;
	uint32_t HashPointSet() const;
	int CalHashPointSet(BagPlan* bagPlan) const;  // 天内天间无序，视为一个大点集
	int CalHashDayOrder(BagPlan* bagPlan) const;  // 天内无序，天间有序，适用rich
	int CalHashPointOrder() const;  // 点与点有序，适用最终路径
	int SetHashFrom(uint32_t hashFrom) const;
private:
	uint32_t Hash(std::vector<uint32_t>& tVec) const;
public:
	static uint32_t Hash(bit160& tVec);
	static const int kPathLimit = 100;  // 一个行程最大VarPlace数量
	static const int kBlockLimit = 40;  // block数量最大 对应天数
public:
	int m_nodeNum;
	int m_blockNum;
	int m_len;
	int m_hot;
	int m_dist;
	int m_cross;
	int m_clash;
	int m_firstL;  // 留空白 未游玩 先去午餐的数量
	double m_score;
	mutable int m_next;
	bit160 m_bid;
	mutable int m_missLevel;

	double m_hotValue;
	int m_trafDist;
	int m_trafDur;
	int m_nonWalkCnt;
	int m_walkPlus;
protected:
	uint8_t m_vPos[kPathLimit];  // vPlace按次序加入
	uint8_t m_bPos[kPathLimit];  // vPlace[i] 加入了哪个bPos
	uint8_t m_vNumList[kBlockLimit];  // 各个block的景点数
	int m_durList[kBlockLimit];  // 各个block的剩余dur
	uint8_t m_rNeedList[kBlockLimit];  // 剩余餐厅类型
	int8_t m_rNumList[kBlockLimit];  // 剩余餐厅数量

	mutable bool m_isInit;
	mutable uint32_t m_hash;  // 路线hash 不同阶段有不同含义
	mutable uint32_t m_hashPointSet;  // 点集hash
	mutable uint32_t m_hashFrom;  // 上级结点路径hash值
};

struct SPathCmp {
	bool operator()(const SPath* spa, const SPath* spb) {
		if (spa->m_missLevel != spb->m_missLevel) {
			return (spa->m_missLevel < spb->m_missLevel);
		} else if (spa->m_clash != spb->m_clash) {
			return (spa->m_clash < spb->m_clash);
		} else if (fabs(spa->m_score - spb->m_score) > 1e-6) {
			return (spa->m_score > spb->m_score);
		} else {
			return (spa->Hash() < spb->Hash());
		}
	}
};

#endif
