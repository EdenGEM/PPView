#ifndef __PATH_EVAL_H__
#define __PATH_EVAL_H__

#include <iostream>
#include "PathView.h"
#include "BasePlan.h"

class PathEval {
public:
	static int CrossDist(int dist, int cross);
	static const int m_maxWalkDay = 3000;
	static const int m_firstLPenalty = 3000;
	static const int m_nonWalkPenalty = 2000;
private:
	static int Eval(BasePlan* basePlan, PathView* path);
	static int GetMaxBlank(const PathView* path);

	friend class PathView;
};
#endif
