#include <iostream>
#include "BasePlan.h"
#include "MyTime.h"
#include "PathView.h"
#include "PathEval.h"

PlanItem::PlanItem() {
	_place = NULL;
	_departTraffic = NULL;
	_arriveTraffic = NULL;
	_trafDate = "";
	_arriveTime = 0;
	_departTime = 0;
	_durS = NULL;
	_openTime = 0;
	_closeTime = 0;
	_cost = 0;
	_type = NODE_FUNC_NULL;
	_remind_tag = "";
	_idle = 0;
	_timeZone = 8;
	m_hasAttach = ATTACH_TYPE_NULL;
}


int PlanItem::Copy(const PlanItem& item) {
	Copy(&item);
	return 0;
}

int PlanItem::Copy(const PlanItem* p) {
	_place = p->_place;
	_arvID = p->_arvID;
	_deptID = p->_deptID;
	_departTraffic = p->_departTraffic;
	_arriveTraffic = p->_arriveTraffic;
	_trafDate = p->_trafDate;
	_arriveTime = p->_arriveTime;
	_departTime = p->_departTime;
	_durS = p->_durS;
	_arriveDate = p->_arriveDate;
	_departDate = p->_departDate;
	_openTime = p->_openTime;
	_closeTime = p->_closeTime;
	_cost = p->_cost;
	_type = p->_type;
	_remind_tag = p->_remind_tag;
	_idle = p->_idle;
	_timeZone = p->_timeZone;
	m_hasAttach = p->m_hasAttach;
	return 0;
}

int PlanItem::Copy(const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, const std::string& trafDate, time_t arriveTime, time_t departTime, const DurS* durS, time_t openTime, time_t closeTime, double cost, int type, double timeZone) {
	_place = place;
	_arvID = arvID;
	_deptID = deptID;
	_arriveTraffic = arriveTraffic;
	_departTraffic = departTraffic;
	_trafDate = trafDate;
	_arriveTime = arriveTime;
	_departTime = departTime;
	_durS = durS;
	_timeZone = timeZone;
	_arriveDate = MJ::MyTime::toString(_arriveTime, _timeZone).substr(0, 8);
	_departDate = MJ::MyTime::toString(_departTime, _timeZone).substr(0, 8);
	_openTime = openTime;
	_closeTime = closeTime;
	_cost = cost;
	_type = type;
	return 0;
}

int PlanItem::GetDur() const {
	return _departTime - _arriveTime;
}

int PlanItem::Dump(BasePlan* basePlan, bool log) const {
	if (log) {
		MJ::PrintInfo::PrintLog("PlanItem::Dump, %s(%s)|L%d|%s-->%s|%s|%s|Mt%X|Md%X|%s|%.2f|T%08x", _place->_ID.c_str(), _place->_name.c_str(), basePlan->GetHot(_place), MJ::MyTime::toString(_arriveTime, _timeZone).substr(4).c_str(), MJ::MyTime::toString(_departTime, _timeZone).substr(4).c_str(), (_departTraffic != NULL) ? MJ::MyTime::toString(_departTime + _departTraffic->_time, _timeZone).substr(4).c_str() : "K", _departTraffic ? ToolFunc::NormSeconds(_departTraffic->_time).c_str() : "K", m_hasAttach & RESTAURANT_TYPE_ALL, m_hasAttach >> 16, ToolFunc::NormSeconds(_departTime - _arriveTime).c_str(), (_type & NODE_FUNC_PLACE) ? (_departTime - _arriveTime) * 1.0 / basePlan->GetRcmdDur(_place) : 0, _type);
	} else {
		MJ::PrintInfo::PrintDbg("PlanItem::Dump, %s(%s)|L%d|%s-->%s|%s|%s|Mt%X|Md%X|%s|%.2f|T%08x", _place->_ID.c_str(), _place->_name.c_str(), basePlan->GetHot(_place), MJ::MyTime::toString(_arriveTime, _timeZone).substr(4).c_str(), MJ::MyTime::toString(_departTime, _timeZone).substr(4).c_str(), (_departTraffic != NULL) ? MJ::MyTime::toString(_departTime + _departTraffic->_time, _timeZone).substr(4).c_str() : "", _departTraffic ? ToolFunc::NormSeconds(_departTraffic->_time).c_str() : "", m_hasAttach & RESTAURANT_TYPE_ALL, m_hasAttach >> 16, ToolFunc::NormSeconds(_departTime - _arriveTime).c_str(), (_type & NODE_FUNC_PLACE) ? (_departTime - _arriveTime) * 1.0 / basePlan->GetRcmdDur(_place) : 0, _type);
	}
	return 0;
}

int PathView::Init(int capacity) {
	if (_capacity > 0 || _item_list) {
		MJ::PrintInfo::PrintErr("PathView::Init, PathView had inited, you must release before init again");
		return 1;
	}
	_item_list = new PlanItem*[_capacity];
	for (int i = 0; i < _capacity; ++i) {
		PlanItem* p = new PlanItem;
		_item_list[i] = p;
	}
	Reset();
	return 0;
}

int PathView::IncreaseCapacity(int length) {
	if (length <= 0) return 0;
	int new_capacity = _capacity + length;
	PlanItem** new_item_list = new PlanItem*[new_capacity];
	for (int i = 0; i < new_capacity; ++i) {
		if (i < _capacity) {
			new_item_list[i] = _item_list[i];
		} else {
			PlanItem* p = new PlanItem;
			new_item_list[i] = p;
		}
	}
	_capacity = new_capacity;
	delete[] _item_list;
	_item_list = NULL;
	_item_list = new_item_list;
	new_item_list = NULL;
	return 0;
}

int PathView::Reset() {
	_score = 0;
	_dist = 0;
	_time = 0;
	_hot = 0;
	m_missLevel = 0;
	_blank = 0;
	_cross = 0;
	_len = 0;
	_placeNum = 0;
	_deleted = false;
	_error_str.clear();
	_debug_str.clear();
//zt add for SA alg
	m_dfsScore = 0;

	m_hotValue = 0;
	m_trafDist = 0;
	m_trafDur = 0;
	m_trafBusCnt = 0;
	m_walkDistPlus = 0;
	m_hashFrom = 0;
	m_isValid = false;
	return 0;
}

int PathView::Copy(const PathView& path) {
	if (_capacity < path._len) {
		IncreaseCapacity(path._len - _capacity);
	}
	_len = path._len;
	_placeNum = path._placeNum;
	_deleted = path._deleted;
	_score = path._score;
	_dist = path._dist;
	_time = path._time;
	_hot = path._hot;
	m_missLevel = path.m_missLevel;
	_blank = path._blank;
	_cross = path._cross;
	for (int i = 0; i < _len; ++i) {
		const PlanItem* item = path.GetItemIndex(i);
		_item_list[i]->Copy(item);
	}
	_debug_str = path._debug_str;
	_error_str = path._error_str;
//zt add for SA alg
	m_dfsScore = path.m_dfsScore;

	m_hotValue = path.m_hotValue;
	m_trafDist = path.m_trafDist;
	m_trafDur = path.m_trafDur;
	m_trafBusCnt = path.m_trafBusCnt;
	m_walkDistPlus = path.m_walkDistPlus;

	m_hashFrom = path.m_hashFrom;
	m_isValid = path.m_isValid;
	return 0;
}

int PathView::Copy(const PathView* path) {
	Copy(*path);
	return 0;
}

const PlanItem* PathView::GetItemIndex(int index) const {
	if (index < 0 || index >= _len) return NULL;
	return _item_list[index];
}
PlanItem* PathView::GetItemIndex(int index) {
	if (index < 0 || index >= _len) return NULL;
	return _item_list[index];
}

PlanItem* PathView::GetItemLast() {
	if (_len <= 0) return NULL;
	return _item_list[_len - 1];
}

// 在index位置插入节点
int PathView::Insert(int index, const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, time_t arriveTime, time_t departTime, const DurS* durS, const std::string& trafDate, time_t openTime, time_t closeTime, double cost, int type, double timeZone) {
	if (index < 0 || index >= _len) return 1;
	if (_len >= _capacity && IncreaseCapacity(_len - _capacity + std::max(1, static_cast<int>(0.3 * _capacity))) != 0) return 1;

	_item_list[_len]->Copy(place, arvID, deptID, arriveTraffic, departTraffic, trafDate, arriveTime, departTime, durS, openTime, closeTime, cost, type, timeZone);
	PlanItem* insert_item = _item_list[_len];

	for (int i = _len; i > index; --i) {
		_item_list[i] = _item_list[i - 1];
	}
	_item_list[index] = insert_item;
	++_len;

	return 0;
}

// 添加节点，交通由外层管理
int PathView::Append(const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, time_t arriveTime, time_t departTime, const DurS* durS, const std::string& trafDate, time_t openTime, time_t closeTime, double cost, int type, double timeZone) {
	if (_len >= _capacity && IncreaseCapacity(_len - _capacity + std::max(1, static_cast<int>(0.3 * _capacity))) != 0) return 1;
	_item_list[_len++]->Copy(place, arvID, deptID, arriveTraffic, departTraffic, trafDate, arriveTime, departTime, durS, openTime, closeTime, cost, type, timeZone);
/*jjj
	if (arriveTraffic) {
		_dist += arriveTraffic->_dist;
		_time += arriveTraffic->_time;
	}
jjj*/
	return 0;
}


int PathView::Append(const PlanItem* item) {
	if (_len >= _capacity && IncreaseCapacity(_len - _capacity + std::max(1, static_cast<int>(0.3 * _capacity))) != 0) return 1;
	_item_list[_len++]->Copy(item);
/*jjj
	if (item->_arriveTraffic) {
		_dist += item->_arriveTraffic->_dist;
		_time += item->_arriveTraffic->_time;
	}
jjj*/
	return 0;
}

int PathView::Length() const {
	return _len;
}

int PathView::Dump(BasePlan* basePlan, bool log) const {
	PathEval::Eval(basePlan, const_cast<PathView *>(this));
	if (log) {
		MJ::PrintInfo::PrintLog("PathView::Dump, [score: %.4f, dist: %d, time: %s, playDur: %d, hot: %d, miss: %d, blank: %s, cross: %d, hashFrom: %d]", _score, _dist, ToolFunc::NormSeconds(_time).c_str(), _playDur, _hot, m_missLevel, ToolFunc::NormSeconds(_blank).c_str(), _cross, m_hashFrom);
		MJ::PrintInfo::PrintLog("PathView::Dump, %s", GetIDStr().c_str());
	} else {
		MJ::PrintInfo::PrintLog("PathView::Dump, [score: %.4f, dist: %d, time: %s, playDur: %d, hot: %d, miss: %d, blank: %s, cross: %d, hashFrom: %d]", _score, _dist, ToolFunc::NormSeconds(_time).c_str(), _playDur, _hot, m_missLevel, ToolFunc::NormSeconds(_blank).c_str(), _cross, m_hashFrom);
		MJ::PrintInfo::PrintDbg("PathView::Dump, %s", GetIDStr().c_str());
	}
	std::vector<std::string> id_list;
	for (int i = 0; i < _len; ++i) {
		const PlanItem* item = _item_list[i];
		item->Dump(basePlan, log);
		id_list.push_back(item->_place->_name);
	}
	if (_error_str.size() > 0) {
		if (log) {
			MJ::PrintInfo::PrintLog("PathView::Dump, failed: %s", _error_str.c_str());
		} else {
			MJ::PrintInfo::PrintDbg("PathView::Dump, failed: %s", _error_str.c_str());
		}
	}
	return 0;
}

// 删除节点，交通由外层逻辑管理
int PathView::Erase(int index) {
	if (index >= _len) return 1;
	PlanItem* erase_item = _item_list[index];
	for (int i = index + 1; i < _len; ++i) {
		_item_list[i - 1] = _item_list[i];
	}
	_item_list[--_len] = erase_item;
	return 0;
}

int PathView::Pop() {
	return Erase(_len - 1);
}

int PathView::Release() {
	if (_item_list) {
		for (int i = 0; i < _capacity; ++i) {
			delete _item_list[i];
		}
		delete[] _item_list;
		_item_list = NULL;
	}
	_capacity = 0;
	Reset();
	return 0;
}

int PathView::ResetLen(int new_len) {
	if (new_len >= 0 && _len > new_len) {
		_len = new_len;
	}
}

std::string PathView::GetIDStr() const {
	std::string ret;
	for (int i = 0; i < _len; ++i) {
		const PlanItem* item = _item_list[i];
		if (i != 0) {
			ret += "_";
		}
		ret += item->_place->_name;
	}
	return ret;
}

// 将Path的[begIdx, endIdx]段拷贝给当前对象
int PathView::CopyN(const PathView& path, int begIdx, int endIdx) {
	Reset();
	for (int i = begIdx; i <= endIdx; ++i) {
		const PlanItem* item = path.GetItemIndex(i);
		if (item) {
			Append(item);
		} else {
			MJ::PrintInfo::PrintErr("PathView::CopyN, Pos[%d] is null in range[%d, %d]", i, begIdx, endIdx);
		}
	}
	return 0;
}

// 将当前对象的[begIdx, endIdx]替换为Path
int PathView::SubN(const PathView& path, int begIdx, int endIdx) {
	if (begIdx < 0 || endIdx < 0 || begIdx >= Length() || endIdx >= Length()) {
		MJ::PrintInfo::PrintErr("PathView::SubN, err idx [%d, %d], len: %d", begIdx, endIdx, Length());
		return 1;
	}
	if (path.Length() != endIdx - begIdx + 1) {
		MJ::PrintInfo::PrintErr("PathView::SubN, _len %d != %d - %d + 1", _len, endIdx, begIdx);
		return 1;
	}
	for (int i = 0; i < path.Length(); ++i) {
		const PlanItem* copyItem = path.GetItemIndex(i);
		int idx = begIdx + i;
		PlanItem* item = GetItemIndex(idx);
		if (item && copyItem) {
			item->Copy(copyItem);
		} else {
			MJ::PrintInfo::PrintErr("PathView::SubN, Pos[%s] is null, Len: %d", idx, Length());
		}

	}
	return 0;
}

PlanItem* PathView::GetItemLastHotel(int index, int &indexLast) {
	if (index < 0 || index >= _len) return NULL;
	int ret_index = index - 1;
	while (ret_index >= 0 && !(_item_list[ret_index]->_place->_t & LY_PLACE_TYPE_HOTEL)) {
		--ret_index;
	}
	if (ret_index < 0 || ret_index >= _len) {
		return NULL;
	}
	indexLast = ret_index;
	return _item_list[ret_index];
}
PlanItem* PathView::GetItemNextHotel(int index, int &indexNext) {
	if (index < 0 || index >= _len) return NULL;
	int ret_index = index + 1;
	while (ret_index < _len && !(_item_list[ret_index]->_place->_t & LY_PLACE_TYPE_HOTEL)) {
		++ret_index;
	}
	if (ret_index < 0 || ret_index >= _len) {
		return NULL;
	}
	indexNext = ret_index;
	return _item_list[ret_index];
}

PlanItem* PathView::GetItemNextDayChange(int index,int &indexNext) {
	if (index < 0 || index >= _len) return NULL;
	int ret_index = index + 1;
	while (ret_index < _len && (_item_list[ret_index]->_departDate == _item_list[index]->_arriveDate)) {
		++ret_index;
	}
	if (ret_index < 0 || ret_index >= _len) {
		return NULL;
	}
	indexNext = ret_index;
	return _item_list[ret_index];
}

int PathView::GetDayIdxByIndex(int index) {
	if (index >= _len) return 0;
	int dayIdx = -1;
	std::string date = "";
	for (int i = 0; i <= index; i++ ) {
		PlanItem* planItem = GetItemIndex(i);
		if (planItem) {
			if (date != planItem->_trafDate) {
				dayIdx ++;
				date = planItem->_trafDate;
			}
		}
	}
	return dayIdx;
}
