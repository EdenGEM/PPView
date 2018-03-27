#ifndef _ROUTE_WORKER_H_
#define _ROUTE_WORKER_H_

#include <iostream>
#include "BagPlan.h"
#include "ThreadMemPool.h"
#include "MyThreadPool.h"

class RouteWorker : public MyWorker {
public:
	RouteWorker(BagPlan* bagPlan, const SPath* dfsPath) : m_plan(bagPlan), m_dfsPath(dfsPath) {
		m_path = new PathView;
	}
	virtual ~RouteWorker() {
		Release();
	}
public:
	virtual int doWork(ThreadMemPool* tmPool);
private:
	int DoRoute(BagPlan* bagPlan, const SPath* dfsPath, PathView* path);
	int Release();
public:
	PathView* m_path;
private:
	BagPlan* const m_plan;
	const SPath* const m_dfsPath;
};
#endif

