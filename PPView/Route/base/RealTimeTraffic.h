#ifndef _REAL_TIME_TRAFFFIC_H_
#define _REAL_TIME_TRAFFFIC_H_

#include <iostream>
#include "BasePlan.h"
#include "PathUtil.h"

class RealTimeTraffic {
public:
	static int DoReplace(BasePlan* basePlan, PathView& path);
private:
	static int GetRealTimeTraf(BasePlan* basePlan, PathView& path);
	static int Replace(BasePlan* basePlan, PathView& path);
	static int ChangeTraf(BasePlan* basePlan, PathView& path);
	static int ChangeTrafPos(BasePlan* basePlan, PathView& path);
	static int ChangeTrafRev(BasePlan* basePlan, PathView& path);
	static int ModLuggageHotel(BasePlan* basePlan, PathView& path);
};

#endif
