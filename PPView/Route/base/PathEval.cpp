#include <iostream>
#include <algorithm>
#include "PathCross.h"
#include "PathEval.h"

int PathEval::GetMaxBlank(const PathView* path) {
	std::vector<int> blank_list;
	int last_key_idx = -1;
	for (int key_idx = 0; key_idx < path->Length(); ++key_idx) {
		const PlanItem* item = path->GetItemIndex(key_idx);
		if (path->GetItemIndex(key_idx)->_type & NODE_FUNC_KEY) {
			if (last_key_idx >= 0) {
				int avail = path->GetItemIndex(key_idx)->_arriveTime - path->GetItemIndex(last_key_idx)->_departTime;
				int used = 0;
				for (int i = last_key_idx; i < key_idx; ++i) {
					const PlanItem* item = path->GetItemIndex(i);
					if (item->_departTraffic) {
						used += item->_departTraffic->_time;
					}
					if (i != last_key_idx) {
						used += (item->_departTime - item->_arriveTime);
					}
				}
				int blank = avail - used;
				blank_list.push_back(blank);
			}
			last_key_idx = key_idx;
		}
	}

	if (blank_list.empty()) return 0;
	return *std::max_element(blank_list.begin(), blank_list.end());
}


int PathEval::Eval(BasePlan* basePlan, PathView* path) {
	double ret = 0;

	// 1 著名度 越大越好
	int hot_level = 0;
	int placeNum = 0;
	int play_dur = 0;
	std::tr1::unordered_set<const LYPlace*> plannedSet;
	for (int i = 0; i < path->Length(); ++i) {
		const PlanItem* item = path->GetItemIndex(i);
		const LYPlace* place = item->_place;
		if (item->_type & NODE_FUNC_PLACE
				&& place->_t & LY_PLACE_TYPE_VAR_PLACE) {
			const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
			plannedSet.insert(vPlace);
			hot_level += basePlan->GetHot(vPlace);
			play_dur += item->_departTime - item->_arriveTime;
		}
		if (item->_type & NODE_FUNC_PLACE) {
			++placeNum;
		}
	}
	int missLevel = 0;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (!plannedSet.count(place)) {
			missLevel += basePlan->GetMissLevel(place);
		}
	}
	// 2 距离 越小越好
	int dist_sum = 0;
	int time_sum = 0;
	for (int i = 0; i < path->Length(); ++i) {
		const PlanItem* item = path->GetItemIndex(i);
		if (item->_departTraffic) {
			dist_sum += item->_departTraffic->_dist;
			time_sum += item->_departTraffic->_time;
		}
	}

	// 3 交叉
	int last_key_idx = -1;
	int cross_cnt = 0;
	for (int key_idx = 0; key_idx < path->Length(); ++key_idx) {
		const PlanItem* item = path->GetItemIndex(key_idx);
		if (item->_type & NODE_FUNC_KEY) {
			if (last_key_idx >= 0) {
				std::vector<const LYPlace*> place_list;
				for (int i = last_key_idx; i <= key_idx; ++i) {
					const LYPlace* place = path->GetItemIndex(i)->_place;
					if (place != LYConstData::m_attachRest) {
						place_list.push_back(place);
					}
				}
				cross_cnt += PathCross::GetCrossCnt(place_list, basePlan->GetCrossMap());
			}
			last_key_idx = key_idx;
		}
	}

	// 4 平衡 越小越好
	int max_blank = GetMaxBlank(path);

	path->_dist = dist_sum;
	path->_time = time_sum;
	path->_hot = hot_level;
	path->_playDur = play_dur;
	path->m_missLevel = missLevel;
	path->_blank = max_blank;
	path->_cross = cross_cnt;
	path->_score = CrossDist(path->_dist, path->_cross);
	path->_placeNum = placeNum;
//	path->_score = dist_sum + cross_cnt * 1500 + group_penalty * 1000;

	return 0;
}

int PathEval::CrossDist(int dist, int cross) {
	return dist + 1500 * cross;
}

