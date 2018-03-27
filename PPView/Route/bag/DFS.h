#ifndef _DFS_H_
#define _DFS_H_

#include <iostream>
#include <vector>
#include "SPath.h"
#include "SPathHeap.h"
#include "BagPlan.h"
#include "SPathAlloc.h"
#include "Route/base/BagParam.h"
#include <utility>

class DFS {
public:
	int DoSearch(BagPlan* bagPlan, const SPath* richPath, int blockIdx, std::vector<const SPath*>& sPathList, SPathAlloc* spAlloc);
private:
	int Init(BagPlan* bagPlan, const SPath* richPath, int blockIdx);
	int Search(BagPlan* bagPlan, SPathAlloc* spAlloc);
	int PushPath(BagPlan* bagPlan, SPathAlloc* spAlloc);
	int SearchView(BagPlan* bagPlan, int idx, SPathAlloc* spAlloc);
	int SearchRest(BagPlan* bagPlan, int idx, SPathAlloc* spAlloc);
	int Init4Clash(BagPlan *bagPlan, int blockIdx);
	int CalClash(BagPlan* bagPlan, int& clash, int& firstL);
	int GetScore(int firstL = 0);
private:
	int m_nodeNum;
	int m_len;
	int m_totDist;
	int m_nonWalkCnt;
	int m_walkDist;
	bool m_visited[BagParam::m_dayNodeLimit];
	const TrafficItem* m_traf[BagParam::m_dayNodeLimit * BagParam::m_dayNodeLimit];
	uint8_t m_vPosList[BagParam::m_dayNodeLimit];
	int m_allocDur[BagParam::m_dayNodeLimit];
	uint8_t m_leftRestNeed;
	uint8_t m_blockRestNeed;
	int m_path[BagParam::m_dayNodeLimit];
	uint8_t m_vType[BagParam::m_dayNodeLimit];
	SPathHeap m_heap;
	unsigned int m_sCnt;
	unsigned int m_tCnt;
	uint8_t m_bPos;
	// 预判
	int m_zipDurList[BagParam::m_dayNodeLimit];
	std::vector<std::vector<const OpenClose*> > m_openCloseList;
	time_t m_start;
	time_t m_stop;
	static const uint32_t m_maxTorlance = 3*3600;//3 hours
};

#endif

