#ifndef _DFS_WORKER_H_
#define _DFS_WORKER_H_

#include <iostream>
#include <tr1/unordered_set>
#include "Route/base/PathEval.h"
#include "Route/base/Utils.h"
#include "BagPlan.h"
#include "DFS.h"
#include "MyThreadPool.h"
#include "MJCommon.h"

class DFSWorker : public MyWorker {
public:
	DFSWorker(BagPlan* bagPlan, const SPath* richPath) : m_plan(bagPlan), m_richPath(richPath) {
		m_tmPool = NULL;
		m_dfsHeap = NULL;
	}
	virtual ~DFSWorker() {
	}
public:
	virtual int doWork(ThreadMemPool* tmPool);
private:
	int JoinDayPath(std::vector<std::vector<const SPath*> >& daySPList, std::vector<const SPath*>& tmpList, int blockIdx);

public:
	ThreadMemPool* m_tmPool;
	SPathHeap* m_dfsHeap;
private:
	BagPlan* const m_plan;
	const SPath* const m_richPath;
};

class SortWorker : public MyWorker {
public:
	SortWorker(std::vector<const SPath*>& sList) : m_sList(sList) {
	}
	virtual ~SortWorker() {}
public:
	virtual int doWork(ThreadMemPool* tmPool) {
		std::sort(m_sList.begin(), m_sList.end(), SPathCmp());
		std::reverse(m_sList.begin(), m_sList.end());
		return 0;
	}
private:
	std::vector<const SPath*>& m_sList;
};

class DelWorker : public MyWorker {
public:
	DelWorker(ThreadMemPool* tmPool) {
		m_tmPool = tmPool;
	}
	virtual ~DelWorker() {}
public:
	virtual int doWork(ThreadMemPool* tmPool) {
		SPathAlloc* spAlloc = m_tmPool->m_spAlloc;
		std::vector<const SPath*>& sList = m_tmPool->m_sList;
		for (int j = 0; j < sList.size(); ++j) {
			spAlloc->Delete(sList[j]);
		}
		return 0;
	}
private:
	ThreadMemPool* m_tmPool;
};

class CrossWorker : public MyWorker {
public:
	CrossWorker(BagPlan* bagPlan, SPath* sPath) : m_plan(bagPlan), m_sPath(sPath) {}
	virtual ~CrossWorker() {}
public:
	virtual int doWork(ThreadMemPool* tmPool);
private:
	BagPlan* const m_plan;
	SPath* m_sPath;
};

#endif

