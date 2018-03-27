#include <iostream>
#include "MJCommon.h"
#include "Utils.h"
#include "BasePlan.h"


int TimeBlock::Dump(bool log) const {
	std::vector<std::string> avail_id_list;
	for (int i = 0; i < m_pInfoList.size(); ++i) {
		avail_id_list.push_back(m_pInfoList[i]->m_vPlace->_ID + "(" + m_pInfoList[i]->m_vPlace->_name + ")");
	}
	if (log) {
		MJ::PrintInfo::PrintLog("TimeBlock::Dump, %s-->%s, avail: %.2f, left: %.2f, interval: %.2f, rest: %d, date: %s, notPlay: %d", MJ::MyTime::toString(_start, _time_zone).c_str(), MJ::MyTime::toString(_stop, _time_zone).c_str(), _avail_dur / 3600.0, _left_avail_dur / 3600.0, _interval_dur / 3600.0, _restNeed, _trafDate.c_str(), _is_not_play);
		if (!avail_id_list.empty()) {
			MJ::PrintInfo::PrintLog("TimeBlock::Dump, node: %s", ToolFunc::join2String(avail_id_list, "_").c_str());
		}
	}
	return 0;
}

int TimeBlock::ClearNode() {
	m_pInfoList.clear();
	m_pInfoMap.clear();
}

int TimeBlock::Release() {
	for (int i = 0; i < _stable_line_list.size(); ++i) {
		delete _stable_line_list[i];
	}
	_stable_line_list.clear();
	return 0;
}
int TimeBlock::PushLine(const LYPlace* from_place, const LYPlace* to_place, const TrafficItem* traffic) {
	_stable_line_list.push_back(new Line(from_place, to_place, traffic));
	return 0;
}
int TimeBlock::PushStable(const LYPlace* place, int alloc_dur) {
	_stable_list.push_back(place);
	_used_dur += alloc_dur;
	return 0;
}

bool TimeBlock::Has(const std::string& id) const {
	if (_nodeFuncMap.find(id) != _nodeFuncMap.end()) return true;
	return false;
}

unsigned int TimeBlock::GetFunc(const std::string& id) const {
	unsigned int nodeFunc = NODE_FUNC_NULL;
	std::tr1::unordered_map<std::string, unsigned int>::const_iterator it = _nodeFuncMap.find(id);
	if (it != _nodeFuncMap.end()) {
		nodeFunc = it->second;
	}
	return nodeFunc;
}

const PlaceInfo* TimeBlock::GetPlaceInfo(const std::string& id) const {
	const PlaceInfo* ret = NULL;
	std::tr1::unordered_map<std::string, const PlaceInfo*>::const_iterator it = m_pInfoMap.find(id);
	if (it != m_pInfoMap.end()) {
		ret = it->second;
	}
	return ret;
}

int KeyNode::Dump(bool log) const {
	if (log) {
		MJ::PrintInfo::PrintLog("KeyNode::Dump, ID:%s(%s), open:%s(%s)-%s, trafDate:%s, dur:%d(%d-%d), conti:%d, cost:%.2f, type:%d, adj:%d, delet:%d", _place->_ID.c_str(), _place->_name.c_str(), MJ::MyTime::toString(m_openClose->m_open, _time_zone, "%m%d_%R").c_str(), MJ::MyTime::toString(m_openClose->m_latestArv, _time_zone, "%m%d_%R").c_str(), MJ::MyTime::toString(m_openClose->m_close, _time_zone, "%m%d_%R").c_str(), _trafDate.c_str(), GetRcmdDur(), GetMinDur(), GetMaxDur(), _continuous, _cost, _type, _adjustable, _deletable);
	} else {
		MJ::PrintInfo::PrintDbg("KeyNode::Dump, ID:%s(%s), open:%s(%s)-%s, trafDate:%s, dur:%d(%d-%d), conti:%d, cost:%.2f, type:%d, adj:%d, delet:%d", _place->_ID.c_str(), _place->_name.c_str(), MJ::MyTime::toString(m_openClose->m_open, _time_zone, "%m%d_%R").c_str(), MJ::MyTime::toString(m_openClose->m_latestArv, _time_zone, "%m%d_%R").c_str(), MJ::MyTime::toString(m_openClose->m_close, _time_zone, "%m%d_%R").c_str(), _trafDate.c_str(), GetRcmdDur(), GetMinDur(), GetMaxDur(), _continuous, _cost, _type, _adjustable, _deletable);
	}
	return 0;
}

bool KeyNode::deletable() const {
	return _deletable;
}

int KeyNode::SetOpen(time_t open) {
	_open = open;
	_open_date = MJ::MyTime::toString(_open, _time_zone, "%Y%m%d");
	m_openClose->m_open = _open;
	m_openClose->m_latestArv = _close - m_durS->m_min;
	return 0;
}

int KeyNode::SetClose(time_t close) {
	_close = close;
	_close_date = MJ::MyTime::toString(_close, _time_zone, "%Y%m%d");
	m_openClose->m_close = _close;
	m_openClose->m_latestArv = _close - m_durS->m_min;
	return 0;
}

int KeyNode::SetDur(int minDur, int zipDur, int rcmdDur, int extendDur, int maxDur) {
	m_durS->Set(minDur, zipDur, rcmdDur, extendDur, maxDur);
	m_openClose->m_latestArv = _close - m_durS->m_min;
	return 0;
}

int KeyNode::GetMinDur() const {
	return m_durS->m_min;
}

int KeyNode::GetZipDur() const {
	return m_durS->m_zip;
}

int KeyNode::GetRcmdDur() const {
	return m_durS->m_rcmd;
}

int KeyNode::GetExtendDur() const {
	return m_durS->m_extend;
}

int KeyNode::GetMaxDur() const {
	return m_durS->m_max;
}


const DurS* KeyNode::GetDurS() const {
	return m_durS;
}

int KeyNode::FixDurS() {
	m_durS->SetMax(_close - _open);
	m_durS->Fix();
	m_openClose->m_latestArv = _close - m_durS->m_min;
	return 0;
}

int KeyNode::Release() {
	for (int i = 0; i < m_openCloseList.size(); ++i) {
		if (m_openCloseList[i]) {
			delete m_openCloseList[i];
		}
	}
	m_openCloseList.clear();
	if (m_durS) {
		delete m_durS;
		m_durS = NULL;
	}
	return 0;
}

int Line::Dump() {
	MJ::PrintInfo::PrintDbg("Line::Dump, %s(%s)-->%s(%s), SphereDist: %d, traffic: %d", _from_place->_ID.c_str(), _from_place->_name.c_str(), _to_place->_ID.c_str(), _to_place->_name.c_str(), _dist, _traffic->_time);
	return 0;
}


int KeyNode::SetTrafDate(const std::string& newTrafDate) {
	_trafDate = newTrafDate;
	return 0;
}



int PlanStats::GetStats(Json::Value& jStats) {

	jStats["durList"].resize(0);
	std::tr1::unordered_map<const LYPlace*, int>::iterator itPoiDur;
	for (itPoiDur = m_placeDurMap.begin(); itPoiDur != m_placeDurMap.end(); itPoiDur ++) {
		Json::Value jPlace;
		jPlace["id"] = itPoiDur->first->_ID;
		jPlace["name"] = itPoiDur->first->_name;
		jPlace["dur"] = itPoiDur->second;
		jStats["durList"].append(jPlace);
	}

	Json::Value jStatsList;
	jStatsList.resize(0);
	jStats["totalThrow"] = 0;
	std::tr1::unordered_map<const LYPlace*, int>::iterator itStat;
	for (itStat = m_placeStatsMap.begin(); itStat != m_placeStatsMap.end(); itStat ++) {
		Json::Value jPlace;
		std::string statsS = "";
		statsS = StatsToString(itStat->second);
		jPlace["id"] = itStat->first->_ID;
		jPlace["name"] = itStat->first->_name;
		jPlace["statsS"] = statsS;
		jStatsList.append(jPlace);
		jStats["totalThrow"] = jStats["totalThrow"].asInt() + 1;
	}
	jStats["statsList"] = jStatsList;
	jStats["leftTime"] = m_leftTime;
	jStats["trafTime"] = m_trafTime;
	return 0;
}

std::string PlanStats::StatsToString(int stats) {
	switch(stats) {
		case PLACE_STATS_THROW_OTHER: return "PLACE_STATS_THROW_OTHER"; break;//其他原因扔点
		case PLACE_STATS_THROW_REST_LIMIT: return "PLACE_STATS_THROW_REST_LIMIT"; break;//餐馆限制扔点
		case PLACE_STATS_THROW_ALLOC_FAIL: return "PLACE_STATS_THROW_ALLOC_FAIL"; break;//开关门扔点
		case PLACE_STATS_THROW_RICH: return "PLACE_STATS_THROW_RICH"; break;//rich扔点
		case PLACE_STATS_THROW_DFS: return "PLACE_STATS_THROW_DFS"; break;//dfs扔点
		case PLACE_STATS_THROW_ROUTE: return "PLACE_STATS_THROW_ROUTE"; break;//route扔点
		case PLACE_STATS_THROW_GREEDY: return "PLACE_STATS_THROW_GREEDY"; break;//greedy扔点
		default: return "other";break;
	}
}

int PlanStats::SetPlaceStats(const LYPlace* place, int stats) {
	if (m_placeStatsMap.find(place) == m_placeStatsMap.end()) {
		m_throwNum ++;
	}
	m_placeStatsMap[place] = stats;
	return 0;
}

int PlanStats::DelPlaceStats(const LYPlace* place) {
	if (m_placeStatsMap.find(place) != m_placeStatsMap.end()) {
		m_throwNum --;
		m_placeStatsMap.erase(m_placeStatsMap.find(place));
	}
	return 0;
}

int PlanStats::SetPlaceDur(const LYPlace* place, int dur) {
	m_placeDurMap[place] = dur;
	return 0;
}

int PlanStats::DelPlaceDur(const LYPlace* place) {
	if (m_placeDurMap.find(place) != m_placeDurMap.end()) {
		m_placeDurMap.erase(m_placeDurMap.find(place));
	}
	return 0;
}

bool PlanStats::HasPlaceStats(const LYPlace* place) {
	if (m_placeStatsMap.find(place) != m_placeStatsMap.end()) {
		return true;
	}
	return false;
}
