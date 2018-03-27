#ifndef _DFSEARCH_H_
#define _DFSEARCH_H_

#include <iostream>
#include "SPath.h"
#include "BagPlan.h"
#include "MyThreadPool.h"
#include "MJCommon.h"

class DFSearch {
public:
	static int DoSearch(BagPlan* bagPlan, std::vector<const SPath*>& richList, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool);
private:
	static int MergeDFS(BagPlan* bagPlan, std::vector<MyWorker*>& jobList, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool);
	static int EvalDFS(BagPlan* bagPlan, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool);

};
#endif
