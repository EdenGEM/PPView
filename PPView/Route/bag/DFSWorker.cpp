#include <iostream>
#include <algorithm>
#include <limits>
#include "Route/base/PathCross.h"
#include "DFS.h"
#include "DFSWorker.h"

int dwDbg = 0;

int DFSWorker::doWork(ThreadMemPool* tmPool) {
	LYConstData::SetQueryParam(&(m_plan->m_qParam));
	if (m_plan->IsDFSTimeOut()) return 1;
	m_tmPool = tmPool;
	uint32_t richPointHash = m_richPath->HashPointSet();
	m_dfsHeap = tmPool->GetDFSHeap(richPointHash);

	std::vector<std::vector<const SPath*> > daySPList(m_plan->GetBlockNum());
	for (int blockIdx = 0; blockIdx < m_plan->GetBlockNum(); ++blockIdx) {
		std::vector<const SPath*>& sPathList = daySPList[blockIdx];
		DFS dfs;
		dfs.DoSearch(m_plan, m_richPath, blockIdx, sPathList, m_tmPool->m_spAlloc);
		//_INFO("blockIdx:%d size:%d wow",blockIdx,sPathList.size());
	}

	// 多天合并 + 入堆
	std::vector<const SPath*> tmpList;
	JoinDayPath(daySPList, tmpList, 0);

    for (int blockIdx = 0; blockIdx < daySPList.size(); ++blockIdx) {
		std::vector<const SPath*>& sPathList = daySPList[blockIdx];
		for (int j = 0; j < sPathList.size(); ++j) {
			const SPath* sPath = sPathList[j];
			m_tmPool->m_spAlloc->Delete(sPath);
		}
	}

	return 0;
}

// 合并多天dfs结果
int DFSWorker::JoinDayPath(std::vector<std::vector<const SPath*> >& daySPList, std::vector<const SPath*>& tmpList, int blockIdx) {
	// 所有天都已经遍历
	if (blockIdx >= daySPList.size()) {
		int totHot = m_richPath->m_hot;
		double totScore = 0;
		int totDist = 0;
		int totClash = 0;
		int totFirstL = 0;
		for (int i = 0; i < tmpList.size(); ++i) {
			const SPath* daySP = tmpList[i];
			totDist += daySP->m_dist;
			totClash += daySP->m_clash;
			totFirstL += daySP->m_firstL;
			totScore += daySP->m_score;
		}
		if (dwDbg == 0) {
			if (!m_dfsHeap->CanPush(totScore)) return 0;
		}

		SPath* joinPath = m_tmPool->m_spAlloc->New();
		joinPath->Init(m_plan);
		for (int i = 0; i < tmpList.size(); ++i) {
			const SPath* daySP = tmpList[i];
			for (int j = 0; j < daySP->m_len; ++j) {
				uint8_t vPos = daySP->GetVPos(j);
				uint8_t bPos = daySP->GetBPos(vPos);
				joinPath->PushV(bPos, vPos);
			}
		}
		joinPath->SetHot(totHot);
		joinPath->SetDist(totDist);
		joinPath->SetClash(totClash);
		joinPath->SetFirstL(totFirstL);
		joinPath->SetScore(totScore);
		joinPath->SetMissLevel(m_richPath->m_missLevel);
		joinPath->SetHashFrom(m_richPath->Hash());
		joinPath->CalHashPointOrder();
		joinPath->CalHashPointSet(m_plan);

		if (dwDbg & 1) { fprintf(stderr, "jjj hash set: %d, from: %d, now: %d\n", m_richPath->HashPointSet(), m_richPath->Hash(), joinPath->Hash()); joinPath->Dump(m_plan); }

		const SPath* dPath = m_dfsHeap->Push(joinPath);
		if (dPath) {
			m_tmPool->m_spAlloc->Delete(dPath);
		}
		return 0;
	}
//	if (m_plan->IsDFSTimeOut()) return 1;

	// 遍历
	std::vector<const SPath*>& sPathList = daySPList[blockIdx];
	for (int i = 0; i < sPathList.size(); ++i) {
		const SPath* sPath = sPathList[i];
		tmpList.push_back(sPath);
		JoinDayPath(daySPList, tmpList, blockIdx + 1);
		tmpList.pop_back();
	}
	return 0;
}

int CrossWorker::doWork(ThreadMemPool* tmPool) {
	int totCross = 0;
	std::vector<const LYPlace*> placeList;
	placeList.reserve(20);

	int lastBPos = -1;
	for (int i = 0; i < m_sPath->m_len; ++i) {
		uint8_t vPos = m_sPath->GetVPos(i);
		uint8_t bPos = m_sPath->GetBPos(vPos);
		if (bPos != lastBPos) {
			const LYPlace* ePlace = m_plan->GetKey(bPos)->_place;
			placeList.push_back(ePlace);
			if (lastBPos >= 0) {
				totCross += PathCross::GetCrossCnt(placeList);
			}
			placeList.clear();
			placeList.push_back(ePlace);
			lastBPos = bPos;
		}
		const LYPlace* place = m_plan->GetPosPlace(vPos);
		placeList.push_back(place);
	}
	const LYPlace* ePlace = m_plan->GetKey(m_plan->GetKeyNum() - 1)->_place;
	placeList.push_back(ePlace);
	totCross += PathCross::GetCrossCnt(placeList);

	m_sPath->SetCross(totCross);
//	if (totCross > 0) {
//		m_sPath->Dump(m_plan);
//	}
	return 0;
}
