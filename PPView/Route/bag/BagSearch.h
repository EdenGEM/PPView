#ifndef _BAG_SEARCH_H_
#define _BAG_SEARCH_H_

#include <iostream>
#include "BagPlan.h"
#include "MyThreadPool.h"
#include "ThreadMemPool.h"
#include "MJCommon.h"

class BagSearch {
public:
	static int DoSearch(BagPlan* bagPlan, MyThreadPool* threadPool);
private:
	static int AllocDur(BagPlan* bagPlan);
	static int CalDur(BagPlan* bagPlan);
	static int GetBenchMark(BagPlan* bagPlan);
	static int GetEstPlace(BagPlan* bagPlan);
	static int GetCBP(BagPlan* bagPlan, MyThreadPool* threadPool);
};
#endif
