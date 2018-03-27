#ifndef _BAG_PLAN_H_
#define _BAG_PLAN_H_

#include <iostream>
#include <vector>
#include <tr1/unordered_map>
#include <limits>
#include "Route/base/PlaceGroup.h"
#include "Route/base/BasePlan.h"
#include "Route/base/BagParam.h"
#include "SPath.h"
#include "Route/base/LYConstData.h"

class BagPlan: public BasePlan {
public:
	BagPlan() {
		m_runType = RUN_PLANNER_BAG;
		m_maxHot = 0;
		m_maxDist = 0;
		m_maxTime = 0;
		m_nodeNum = 0;
		qType = "ssv005_rich";
	}
	virtual ~BagPlan() {
		Release();
	}
	int AdaptOrder(const SPath* sPath, std::vector<PlaceOrder>& orderList);
public:
	double NormScore(int hot, int dist, int time = 0);
	int SetMaxHot(int maxHot);
	int SetMaxDist(int maxDist);
	int SetMaxTime(int maxTime);
	int MaxHot() const;
	int MaxDist() const;
	int MaxTime() const;
	int DoPos();
	bool IsRootTimeOut();
	bool IsRichTimeOut();
	bool IsDFSTimeOut();
	bool IsRouteTimeOut();
private:
	int Release() {
		return 0;
	}
public:
	BagCost m_bagCost;
	BagStat m_bagStat;

	//按步骤卡时
	MJ::MyTimer m_rootTimer;
	MJ::MyTimer m_richTimer;
	MJ::MyTimer m_dfsTimer;
	MJ::MyTimer m_routeTimer;

	std::vector<const LYPlace*> m_futureList;
	std::tr1::unordered_set<std::string> m_futureSet;  // 用来卡包

	int m_estPlaceNum;  // 预估行程包含的view+shop+rest数
private:
	int m_maxHot; // benchmark
	int m_maxDist;
	int m_maxTime;
public:
	int m_nodeNum;
	int m_kNum;
	int m_blockNum;
	std::vector<const LYPlace*> m_posPList;
	std::deque<bool> m_isPosNonMiss;
	std::vector<uint8_t> m_vTypeList;  // 对应nodeType
	std::tr1::unordered_map<std::string, int> m_posMap;

	std::vector<bit160> m_bidPList;
	std::tr1::unordered_map<std::string, bit160> m_bidMap;
	bit160 m_userBid;
	bit160 m_richBid;
	static bool m_track;
	std::string qType;
public:
	int PosNum() const;
	const LYPlace* GetPosPlace(uint8_t vPos);
	bool IsPosNonMiss(uint8_t vPos);
	int GetPosMissLevel(uint8_t vPos);
	bit160 GetPosBid(uint8_t vPos);
	const TrafficItem* GetPosTraf(uint8_t bPos, uint8_t iPos, uint8_t jPos);
	uint8_t GetPos(const std::string& id);
	uint8_t GetVType(uint8_t bPos, uint8_t vPos);
};

class PathOpt {
public:
	PathOpt(const void* bPath) {
		m_path = bPath;
		m_hot = 0;
		m_dist = 0;
		m_cross = 0;
		m_time = 0;
		m_score = 0;
	}
public:
	int Dump(bool log = false) const;
public:
	const void* m_path;
	int m_hot;
	int m_dist;
	int m_cross;
	int m_time;
	double m_score;
};

#endif
