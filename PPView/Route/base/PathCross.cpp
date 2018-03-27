#include <iostream>
#include <cmath>
#include "PathCross.h"

int PathCross::GetCrossCnt(std::vector<const LYPlace*>& place_list, std::tr1::unordered_map<std::string, int>& cross_map) {
	return 0;
	int ret = 0;
	for (int i = 0; i + 1 < place_list.size(); ++i) {
		const LYPlace* place_a = place_list[i];
		const LYPlace* place_b = place_list[i + 1];
		for (int j = i + 2; j + 1 < place_list.size(); ++j) {
			const LYPlace* place_c = place_list[j];
			const LYPlace* place_d = place_list[j + 1];
			int cross_type = IsCross(place_a, place_b, place_c, place_d, cross_map);
			if (cross_type == 2) ++ret;
		}
	}
	return ret;
}

int PathCross::GetCrossCnt(std::vector<const LYPlace*>& place_list) {
	int ret = 0;
	for (int i = 0; i + 1 < place_list.size(); ++i) {
		const LYPlace* place_a = place_list[i];
		const LYPlace* place_b = place_list[i + 1];
		for (int j = i + 2; j + 1 < place_list.size(); ++j) {
			const LYPlace* place_c = place_list[j];
			const LYPlace* place_d = place_list[j + 1];
			int cross_type = IsCross(place_a, place_b, place_c, place_d);
			if (cross_type == 2) ++ret;
		}
	}
	return ret;
}

// 判断四点是否相交
int PathCross::IsCross(const LYPlace* pa, const LYPlace* pb, const LYPlace* pc, const LYPlace* pd, std::tr1::unordered_map<std::string, int>& cross_map) {
	int ret = 0;
	char buff[100];
	// 有共点情况
	if (!(pa != pb && pa != pc && pa != pd && pb != pc && pb != pd && pc != pd)) {
		return CROSS_OVERLAP;
	}
	
	snprintf(buff, sizeof(buff), "%s_%s|%s_%s", pa->_ID.c_str(), pb->_ID.c_str(), pc->_ID.c_str(), pd->_ID.c_str());
	std::tr1::unordered_map<std::string, int>::iterator it = cross_map.find(buff);
	if (it != cross_map.end()) {
		return it->second;
	}

	ret = IsCross(pa->_point, pb->_point, pc->_point, pd->_point);
	cross_map[buff] = ret;
	
	snprintf(buff, sizeof(buff), "%s_%s|%s_%s", pa->_ID.c_str(), pb->_ID.c_str(), pd->_ID.c_str(), pc->_ID.c_str());
	cross_map[buff] = ret;
	snprintf(buff, sizeof(buff), "%s_%s|%s_%s", pb->_ID.c_str(), pa->_ID.c_str(), pc->_ID.c_str(), pd->_ID.c_str());
	cross_map[buff] = ret;
	snprintf(buff, sizeof(buff), "%s_%s|%s_%s", pb->_ID.c_str(), pa->_ID.c_str(), pd->_ID.c_str(), pc->_ID.c_str());
	cross_map[buff] = ret;

	return ret;
}

// 判断四点是否相交
int PathCross::IsCross(const LYPlace* pa, const LYPlace* pb, const LYPlace* pc, const LYPlace* pd) {
	// 有共点情况
	if (!(pa != pb && pa != pc && pa != pd && pb != pc && pb != pd && pc != pd)) {
		return CROSS_OVERLAP;
	}
	return IsCross(pa->_point, pb->_point, pc->_point, pd->_point);
}

int PathCross::IsCross(const Point& pa, const Point& pb, const Point& pc, const Point& pd) {
	double direction_abc = GetDirection(pa, pb, pc);
	double direction_abd = GetDirection(pa, pb, pd);
	double direction_cda = GetDirection(pc, pd, pa);
	double direction_cdb = GetDirection(pc, pd, pb);
	if (direction_abc * direction_abd < 0 && direction_cda * direction_cdb < 0) {
		return CROSS_YES;
	} else if (fabs(direction_abc) < 1e-100 && IsOnSegment(pa, pb, pc)) {
		return CROSS_OVERLAP;
	} else if (fabs(direction_abd) < 1e-100 && IsOnSegment(pa, pb, pd)) {
		return CROSS_OVERLAP;
	} else if (fabs(direction_cda) < 1e-100 && IsOnSegment(pc, pd, pa)) {
		return CROSS_OVERLAP;
	} else if (fabs(direction_cdb) < 1e-100 && IsOnSegment(pc, pd, pb)) {
		return CROSS_OVERLAP;
	}
	return CROSS_NO;
}

// 计算 ab ac的向量积，判断从ab->ac是顺时针还是逆时针
double PathCross::GetDirection(const Point& pa, const Point& pb, const Point& pc) {
	return (pb._x - pa._x) * (pc._y - pa._y) - (pc._x - pa._x) * (pb._y - pa._y);
}

// a, b, c共线, 判断c在线段ab上, 还是在ab的延长线上
bool PathCross::IsOnSegment(const Point& pa, const Point& pb, const Point& pc) {
	if (pc._x < std::min(pa._x, pb._x) 
			|| pc._x > std::max(pa._x, pb._x) 
			|| pc._y < std::min(pa._y, pb._y) 
			|| pc._y > std::max(pa._y, pb._y)) {
		// 延长线
	   	return false;
	} else {
		return true;
	}
}
