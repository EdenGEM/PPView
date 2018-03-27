#include <iostream>
#include <limits>
#include "Route/base/BagParam.h"
#include "Route/base/PathEval.h"
#include "SPathHeap.h"
#include "DFSWorker.h"
#include "DFSearch.h"

int dfsearchDbg = 0;

struct SPathDFSCmp {
	bool operator()(const SPath* spa, const SPath* spb) {
		if (spa->m_hot != spb->m_hot) {
			return (spa->m_hot > spb->m_hot);
		} else if (spa->Hash() != spb->Hash()) {
			return (spa->Hash() < spb->Hash());
		} else {
			return (spa->m_score > spb->m_score);//hash值相同的路线之间才比较分数
		}
	}
};

int DFSearch::DoSearch(BagPlan* bagPlan, std::vector<const SPath*>& richList, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool) {
	int ret = 0;

	std::vector<MyWorker*> jobs;
	for (int i = 0; i < richList.size(); ++i) {
		const SPath* richPath = richList[i];
		DFSWorker* dfsWorker = new DFSWorker(bagPlan, richPath);
		jobs.push_back(dynamic_cast<MyWorker*>(dfsWorker));
		threadPool->add_worker(dynamic_cast<MyWorker*>(dfsWorker));
	}

	threadPool->wait_worker_done(jobs);

	MergeDFS(bagPlan, jobs, dfsList, threadPool);

	for (int i = 0; i < jobs.size(); ++i) {
		DFSWorker* dfsWorker = dynamic_cast<DFSWorker*>(jobs[i]);
		delete dfsWorker;
	}
	jobs.clear();

	// 按 热度->点集->评分 排序
	std::sort(dfsList.begin(), dfsList.end(), SPathDFSCmp());
	if (dfsList.size() > bagPlan->m_cbP.m_cbDFSHeapLimit) {
		for (int i = bagPlan->m_cbP.m_cbDFSHeapLimit; i < dfsList.size(); ++i) {
			if (dfsList[i]) {
				delete dfsList[i];
			}
		}
		dfsList.erase(dfsList.begin() + bagPlan->m_cbP.m_cbDFSHeapLimit, dfsList.end());
	}

	MJ::PrintInfo::PrintLog("[%s]DFSearch::DoSearch, PathLen_dfs: %d", bagPlan->m_qParam.log.c_str(), dfsList.size());
	for (int i = 0; i < dfsList.size(); ++i) {
		if (!BagPlan::m_track && i > 10) break;
		MJ::PrintInfo::PrintLog("[%s]DFSearch::DoSearch, dfs[%d]\t%d\t%d", bagPlan->m_qParam.log.c_str(), i, dfsList[i]->Hash(), dfsList[i]->HashPointSet());
		dfsList[i]->Dump(bagPlan, true);
	}

	return 0;
}

// 合并多个heap结果
int DFSearch::MergeDFS(BagPlan* bagPlan, std::vector<MyWorker*>& jobList, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool) {
	// 0 各线程使用的内存池
	std::tr1::unordered_set<ThreadMemPool*> tmPoolSet;
	for (int i = 0; i < jobList.size(); ++i) {
		DFSWorker* dfsWorker = dynamic_cast<DFSWorker*>(jobList[i]);
		ThreadMemPool* tmPool = dfsWorker->m_tmPool;
		if (tmPool && tmPoolSet.find(tmPool) == tmPoolSet.end()) {
			tmPoolSet.insert(tmPool);
		}
	}

	// 1 各线程结果合并
	std::tr1::unordered_map<uint32_t, SPathHeap*> dfsHeapMap;
	for (std::tr1::unordered_set<ThreadMemPool*>::iterator it = tmPoolSet.begin(); it != tmPoolSet.end(); ++it) {
		ThreadMemPool* tmPool = *it;
		for (std::tr1::unordered_map<uint32_t, SPathHeap*>::iterator ii = tmPool->m_dfsHeapMap.begin(); ii != tmPool->m_dfsHeapMap.end(); ++ii) {
			uint32_t richPointHash = ii->first;
			if (dfsHeapMap.find(richPointHash) == dfsHeapMap.end()) {
				dfsHeapMap[richPointHash] = new SPathHeap(BagParam::m_dfsSPSHashHeapLimit);
			}
			SPathHeap* dfsHeap = dfsHeapMap[richPointHash];

			SPathHeap* tDFSHeap = ii->second;
			std::vector<const SPath*> sPathList;
			tDFSHeap->CopyVal(sPathList);

			for (int i = 0; i < sPathList.size(); ++i) {
				dfsHeap->Push(sPathList[i]);
				if (dfsearchDbg & 1) { sPathList[i]->Dump(bagPlan); }
			}
		}
	}

	for (std::tr1::unordered_map<uint32_t, SPathHeap*>::iterator it = dfsHeapMap.begin(); it != dfsHeapMap.end(); ++it) {
		SPathHeap* dfsHeap = it->second;
		std::vector<const SPath*> sPathList;
		dfsHeap->CopyVal(sPathList);
		dfsList.insert(dfsList.end(), sPathList.begin(), sPathList.end());
	}

	// 2 内存释放
	for (int i = 0; i < dfsList.size(); ++i) {
		const SPath* oriPath = dfsList[i];
		SPath* nPath = new SPath(oriPath);
		dfsList[i] = nPath;
	}

	for (std::tr1::unordered_map<uint32_t, SPathHeap*>::iterator it = dfsHeapMap.begin(); it != dfsHeapMap.end(); ++it) {
		if (it->second) {
			delete it->second;
		}
	}
	dfsHeapMap.clear();

	for (std::tr1::unordered_set<ThreadMemPool*>::iterator it = tmPoolSet.begin(); it != tmPoolSet.end(); ++it) {
		ThreadMemPool* tmPool = *it;
		tmPool->Clear();
	}

	return 0;
}

// 给路线评价排序
// 未被使用
int DFSearch::EvalDFS(BagPlan* bagPlan, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool) {
	std::vector<MyWorker*> jobs;
	jobs.reserve(dfsList.size());
	for (int i = 0; i < dfsList.size(); ++i) {
		SPath* sPath = const_cast<SPath*>(dfsList[i]);
		CrossWorker* eWorker = new CrossWorker(bagPlan, sPath);
		jobs.push_back(dynamic_cast<MyWorker*>(eWorker));
		threadPool->add_worker(dynamic_cast<MyWorker*>(eWorker));
	}
	threadPool->wait_worker_done(jobs);
	for (int i = 0; i < jobs.size(); ++i) {
		CrossWorker* eWorker = dynamic_cast<CrossWorker*>(jobs[i]);
		delete eWorker;
	}
	jobs.clear();

	// 各路径得分
	for (int i = 0; i < dfsList.size(); ++i) {
		SPath* sPath = const_cast<SPath*>(dfsList[i]);
		int crossDist = PathEval::CrossDist(sPath->m_dist, sPath->m_cross);
		double score = bagPlan->NormScore(sPath->m_hot, crossDist);
		sPath->SetScore(score);
	}

	std::sort(dfsList.begin(), dfsList.end(), SPathCmp());
	return 0;
}
