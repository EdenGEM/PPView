#ifndef __PATH_CROSS_H__
#define __PATH_CROSS_H__

#include <iostream>
#include "define.h"
#include "BasePlan.h"

enum CROSS_TYPE {
	CROSS_NO, // 不交叉
	CROSS_OVERLAP,  // 共点
	CROSS_YES
};

class PathCross {
public:
	static int GetCrossCnt(std::vector<const LYPlace*>& place_list, std::tr1::unordered_map<std::string, int>& cross_map);
	static int IsCross(const LYPlace* pa, const LYPlace* pb, const LYPlace* pc, const LYPlace* pd, std::tr1::unordered_map<std::string, int>& cross_map);
	static int GetCrossCnt(std::vector<const LYPlace*>& place_list);
	static int IsCross(const LYPlace* pa, const LYPlace* pb, const LYPlace* pc, const LYPlace* pd);
private:
	static int IsCross(const Point& pa, const Point& pb, const Point& pc, const Point& pd);
	static double GetDirection(const Point& pa, const Point& pb, const Point& pc);
	static bool IsOnSegment(const Point& pa, const Point& pb, const Point& pc);
};

#endif
