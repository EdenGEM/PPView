#ifndef _PATH_VIEW_H_
#define _PATH_VIEW_H_

#include <iostream>
#include "define.h"

class BasePlan;

class PlanItem {
public:
	PlanItem();
	PlanItem(const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, const std::string& trafDate, time_t arriveTime, time_t departTime, const DurS* durS, time_t openTime, time_t closeTime, double cost, int type, double timeZone) {
		Copy(place, arvID, deptID, arriveTraffic, departTraffic, trafDate, arriveTime, departTime, durS, openTime, closeTime, cost, type, timeZone);
	}
	PlanItem(const PlanItem* p) {
		Copy(p);
	}
	PlanItem(const PlanItem& item) {
		Copy(item);
	}
	virtual ~PlanItem() {}
public:
	int Copy(const PlanItem* p);
	int Copy(const PlanItem& item);
	int Copy(const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, const std::string& trafDate, time_t arriveTime, time_t departTime, const DurS* durS, time_t openTime, time_t closeTime, double cost, int type, double timeZone);
	int Dump(BasePlan* basePlan, bool log = false) const;
	int GetDur() const;
public:
	const LYPlace* _place;				// 景点基本信息
	std::string _arvID;
	std::string _deptID;
	const TrafficItem* _departTraffic;	// 离开景点的交通信息
	const TrafficItem* _arriveTraffic;	// 到达景点的交通信息
	time_t _arriveTime;					// 到达时间
	time_t _departTime;					// 出发时间 0表示尚未确定离开时间
	const DurS* _durS;
	std::string _trafDate;   // 当前item与下一item的交通归属日期
	std::string _arriveDate;
	std::string _departDate;
	double _timeZone;
	time_t _openTime;
	time_t _closeTime;
	double _cost;							// 景点或酒店的花费
	int _type;
	std::string _remind_tag;
	std::string _remind_tag_app;
	int _idle;  // 与下node的空余时间
	int m_hasAttach;
};

class PathView {
public:
	PathView() {
		_len = 0;
		_capacity = 0;
		_item_list = NULL;
		Reset();
	}
	PathView(int capacity) {
		_capacity = 0;
		_item_list = NULL;
		Reset();
		Init(capacity);
	}
	virtual ~PathView() {
		Release();
	}
public:
	int Init(int capacity);
	int IncreaseCapacity(int length);
	int Reset();
	int Copy(const PathView& path);
	int Copy(const PathView* path);
	int CopyN(const PathView& path, int begIdx, int endIdx);
	PlanItem* GetItemIndex(int index);
	const PlanItem* GetItemIndex(int index) const;
	PlanItem* GetItemLast();
	PlanItem* GetItemLastHotel(int index, int &indexLast);
	PlanItem* GetItemNextHotel(int index, int &indexNext);
	PlanItem* GetItemNextDayChange(int index,int &indexNext);
	int Insert(int index, const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, time_t arriveTime, time_t departTime, const DurS* durS, const std::string& trafDate, time_t openTime, time_t closeTime, double cost, int type, double timeZone);
	int SubN(const PathView& path, int begIdx, int endIdx);
	int Append(const LYPlace* place, const std::string& arvID, const std::string& deptID, const TrafficItem* arriveTraffic, const TrafficItem* departTraffic, time_t arriveTime, time_t departTime, const DurS* durS, const std::string& trafDate, time_t openTime, time_t closeTime, double cost, int type, double timeZone);
	int Append(const PlanItem* item);
	int Length() const;
	int Dump(BasePlan* basePlan, bool log = false) const;
	int Erase(int index);
	int Pop();
	int ResetLen(int new_len);
	std::string GetIDStr() const;
	int GetDayIdxByIndex(int index);
private:
	int Release();

private:
	PlanItem** _item_list;
	int _len;
public:
	int _capacity;
	double _score;
	int _dist;
	int _time;
	int _playDur;//游玩时长
	int _blank;
	int _hot;
	int m_missLevel;
	int _cross;
	std::string _debug_str;
	std::string _error_str;
	bool _deleted;
	int _placeNum;
//zt add for SA alg
	double m_dfsScore;

	double m_hotValue;
	double m_newHotlevel;
	int m_trafDist;
	int m_trafDur;
	int m_trafBusCnt;
	int m_walkDistPlus;
	uint32_t m_hashFrom;
	bool m_isValid;
};

#endif
