#ifndef _PLACE_INFO_ALLOC_H_
#define _PLACE_INFO_ALLOC_H_

#include <iostream>
#include <tr1/unordered_set>
#include "BasePlan.h"

class PlaceInfoAlloc {
public:
	PlaceInfoAlloc() {
		_allocated_rest_cnt = 0;
		_allocated_shop_cnt = 0;
	}
public:
	int DoAlloc(BasePlan* basePlan);
	int Alloc(BasePlan* basePlan);
	bool IsAllocable(BasePlan* basePlan, const VarPlace* vPlace);
private:
	std::tr1::unordered_set<std::string> _allocable_place_set;
	std::tr1::unordered_set<std::string> _unallocable_place_set;
	int _allocated_rest_cnt;
	int _allocated_shop_cnt;
};

#endif
