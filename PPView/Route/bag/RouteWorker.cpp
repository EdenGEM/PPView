#include <iostream>
#include "Route/base/TrafRoute.h"
#include "Route/base/PathView.h"
#include "Route/base/PathEval.h"
#include "Route/base/PathUtil.h"
#include "RouteWorker.h"

const int rwDbg = 0;

int RouteWorker::doWork(ThreadMemPool* tmPool) {
//	if (m_plan->IsRouteTimeOut()) return 1;
	LYConstData::SetQueryParam(&(m_plan->m_qParam));

	return DoRoute(m_plan, m_dfsPath, m_path);
}

int RouteWorker::DoRoute(BagPlan* bagPlan, const SPath* dfsPath, PathView* path) {
	int ret = 0;
	// 0 构造PlaceOrder
	std::vector<PlaceOrder> orderList;
	ret = bagPlan->AdaptOrder(m_dfsPath, orderList);

	// 1 造PathView
	if (ret == 0) {
		TrafRoute tRoute;
		ret = tRoute.DoRoute(bagPlan, orderList, path);
	}
	if (rwDbg & 1) { fprintf(stderr, "jjj pathRoute finish!\n"); }

	if (rwDbg & 1) { path->Dump(bagPlan); }
	// 2 加rich与拉伸
	if (ret == 0) {
		ret = PathUtil::TimeEnrich(bagPlan, *path);
	}

	path->m_hashFrom = dfsPath->Hash();
//	path->m_bestCnt = dfsPath->GetBestCnt();
	if (rwDbg & 1) { path->Dump(bagPlan); }

	return ret;
}

int RouteWorker::Release() {
	if (m_path) {
		delete m_path;
		m_path = NULL;
	}
	return 0;
}
