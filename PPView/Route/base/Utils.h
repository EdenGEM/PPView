#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>
#include <string>
#include <vector>
#include "define.h"
#include "ToolFunc.h"
#include "LYConstData.h"


// 当前keynode与上一keynode是否直接相连
#define KEY_NODE_CONTI_NO 0
#define KEY_NODE_CONTI_OK 1


// 与指定/推荐时间比较，node可调整范围
enum ADJUST_LEVEL {
	ADJUST_NO,  // 不可调整
	ADJUST_FINE,  // 微调 <=10min
	ADJUST_FIX,  // 可调整至(1 - fix)，普通情况下压缩比
	ADJUST_MIN,  // 特别紧迫情况压缩比，可调整至mincost
	ADJUST_HALF_HOUR
};

enum PLACE_STATS {
	PLACE_STATS_THROW_OTHER, //其他原因扔点
	PLACE_STATS_THROW_REST_LIMIT, //餐馆限制扔点
	PLACE_STATS_THROW_ALLOC_FAIL, //分配失败扔点
	PLACE_STATS_THROW_RICH, //rich扔点
	PLACE_STATS_THROW_DFS, //dfs扔点
	PLACE_STATS_THROW_ROUTE, //route扔点
	PLACE_STATS_THROW_GREEDY //greedy扔点
};

template <class T1, class T2, class Pred = std::less<T2> > struct sort_pair_first {
	bool operator()(const std::pair<T1,T2>&left, const std::pair<T1,T2>&right) {
		Pred p;
		return p(left.first, right.first);
	}
};

template <class T1, class T2, class Pred = std::less<T2> > struct sort_pair_second {
	bool operator()(const std::pair<T1,T2>&left, const std::pair<T1,T2>&right) {
		Pred p;
		return p(left.second, right.second);
	}
};

class BasePlan;
class PathView;

class Line {
public:
	Line(const LYPlace* from_place, const LYPlace* to_place, const TrafficItem* traffic) {
		_from_place = from_place;
		_to_place = to_place;
		if (_from_place && _to_place) {
			_dist = LYConstData::CaluateSphereDist(_from_place, to_place);
		} else {
			_dist = -1;
		}
		_traffic = traffic;
	}
	Line(const Line* ptr) {
		_from_place = ptr->_from_place;
		_to_place = ptr->_to_place;
		_dist = ptr->_dist;
		_traffic = ptr->_traffic;
	}
	int Dump();
public:
	const LYPlace* _from_place;
	const LYPlace* _to_place;
	int _dist;  // 球面距离
	const TrafficItem* _traffic;
};

class TimeBlock {
public:
	TimeBlock(time_t start, time_t stop, const std::string& trafDate, int avail_dur, uint8_t restNeed, double time_zone) {
		_start = start;
		_stop = stop;
		_interval_dur = stop - start;
		_avail_dur = avail_dur;
		_left_avail_dur = 0;
		_time_zone = time_zone;
		_trafDate = trafDate;
		_used_dur = 0;
		_restNeed = restNeed;
		_restNum = ToolFunc::CountBit(_restNeed);
		OpenClose* openClose = new OpenClose(0, 0, 0, 2);	//午餐
		m_restOpenCloseList.push_back(openClose);

		openClose = new OpenClose(0, 0, 0, 4); //晚餐
		m_restOpenCloseList.push_back(openClose);
		_is_not_play = false;
	}
	~TimeBlock() {
		Release();
		for (int i = 0; i < m_restOpenCloseList.size(); ++i) {
			if (m_restOpenCloseList[i]) {
				delete m_restOpenCloseList[i];
			}
		}
		m_restOpenCloseList.clear();
	}
	TimeBlock(const TimeBlock* block) {
		Release();
		_start = block->_start;
		_stop = block->_stop;
		_interval_dur = block->_interval_dur;
		_avail_dur = block->_avail_dur;
		_left_avail_dur = block->_left_avail_dur;
		_used_dur = block->_used_dur;
		m_pInfoList = block->m_pInfoList;
		m_pInfoMap = block->m_pInfoMap;
		_nodeFuncMap = block->_nodeFuncMap;
		_time_zone = block->_time_zone;
		_trafDate = block->_trafDate;
		_stable_list.assign(block->_stable_list.begin(), block->_stable_list.end());
		for (int i = 0; i < block->_stable_line_list.size(); ++i) {
			_stable_line_list.push_back(new Line(block->_stable_line_list[i]));
		}
		_restNeed = block->_restNeed;
		_restNum = block->_restNum;

		for (int i = 0; i < block->m_restOpenCloseList.size(); ++i) {
			m_restOpenCloseList.push_back(new OpenClose(*(block->m_restOpenCloseList[i])));
		}
		_is_not_play = block->_is_not_play;
	}
public:
	void SetNotPlay(bool notPlay = true){_is_not_play = notPlay;};
	bool IsNotPlay(){ return _is_not_play; }
	int Dump(bool log = false) const;
	int ClearNode();
	int PushLine(const LYPlace* from_place, const LYPlace* to_place, const TrafficItem* traffic);
	int PushStable(const LYPlace* place, int alloc_dur);
	bool Has(const std::string& id) const;
	unsigned int GetFunc(const std::string& id) const;
	const PlaceInfo* GetPlaceInfo(const std::string& id) const;

private:
	int Release();
public:
	time_t _start;
	time_t _stop;
	int _interval_dur;  // 间隔时长
	int _avail_dur;  // 真实可用时长
	int _left_avail_dur;  // 包含当前block可用时长
	std::vector<const PlaceInfo*> m_pInfoList;
	std::tr1::unordered_map<std::string, const PlaceInfo*> m_pInfoMap;
	std::tr1::unordered_map<std::string, unsigned int> _nodeFuncMap;  // 在当前block内的infoType
	double _time_zone;
	std::string _trafDate;  // jDayList归属日期

	// daygroup迁移的变量
	std::vector<const LYPlace*> _stable_list;
	int _used_dur;
	std::vector<Line*> _stable_line_list;
	uint8_t _restNeed;  // 上一block与当前block间隔内包含的rest类型
	int8_t _restNum;
	std::vector<const OpenClose*> m_restOpenCloseList;
public:
	bool _is_not_play;
};

class KeyNode {
public:
	KeyNode() {
		_place = NULL;
		_open = 0;
		_close = 0;
		_continuous = KEY_NODE_CONTI_NO;
		_cost = 0;
		_type = NODE_FUNC_NULL;
		_adjustable = ADJUST_NO;
		_deletable = false;
		_time_zone = 8;
		m_openClose = new OpenClose;
		m_openCloseList.push_back(m_openClose);
		m_durS = new DurS;
		_notPlan = 0;
	}

	KeyNode(const LYPlace* place, time_t open, time_t close, const std::string& trafDate, int minDur, int zipDur, int rcmdDur, int extendDur, int maxDur, int continuous, int cost, int type, int adjustable, bool deletable, double time_zone) {
		_place = place;
		_trafDate = trafDate;
		_continuous = continuous;
		_cost = cost;
		_type = type;
		_adjustable = adjustable;
		_deletable = deletable;
		_time_zone = time_zone;
		m_openClose = new OpenClose;
		m_durS = new DurS(minDur, zipDur, rcmdDur, extendDur, maxDur);
		m_openCloseList.push_back(m_openClose);
		SetOpen(open);
		SetClose(close);
		_notPlan = 0;
	}
	~KeyNode() {
		Release();
	}
private:
	int Release();
public:
	int Dump(bool log = false) const;
	bool deletable() const;
	int SetOpen(time_t open);
	int SetClose(time_t close);
	int SetDur(int minDur, int zipDur, int rcmdDur, int extendDur, int maxDur);
	int GetMinDur() const;
	int GetZipDur() const;
	int GetRcmdDur() const;
	int GetExtendDur() const;
	int GetMaxDur() const;
	const DurS* GetDurS() const;
	int FixDurS();
	int SetTrafDate(const std::string& newTrafDate);
public:
	const LYPlace* _place;

	time_t _open;
	time_t _close;  // 必须在哪个时间段内活动
	std::string _trafDate;  // jDayList归属日期 影响实时交通 不一定是open或close的日期
	std::string _open_date;
	std::string _close_date;

	int _notPlan; //该keynode和之前的keynode之间是否规划
	int _continuous;		// 此keynode与上个keynode之间是否能插入其他活动(0/1) 仅考虑逻辑 不考虑时间
	double _cost;			// money花费
	int _type;				// bitsarray:hotel, view, resta, shop, station
	int _adjustable;		// 是否可缩放dur
	bool _deletable;  // 是否可删除
	double _time_zone;
	std::vector<const OpenClose*> m_openCloseList;
private:
	OpenClose* m_openClose;
	DurS* m_durS;
};

class PlaceOrder {
public:
	PlaceOrder(const LYPlace* place, const std::string& date, int index, unsigned char type) {
		_place = place;
		_date = date;
		_index = index;
		_type = type;
	}
	PlaceOrder(const PlaceOrder* ptr) {
		_place = ptr->_place;
		_date = ptr->_date;
		_index = ptr->_index;
		_type = ptr->_type;
	}
	PlaceOrder(const PlaceOrder& placeOrder) {
		_place = placeOrder._place;
		_date = placeOrder._date;
		_index = placeOrder._index;
		_type = placeOrder._type;
	}
	int Dump(bool log = false) {
		if (log) {
			MJ::PrintInfo::PrintLog("PlaceOrder::Dump, %s(%s), %s, %d, %d", _place->_ID.c_str(), _place->_name.c_str(), _date.c_str(), _index, _type);
		} else {
			MJ::PrintInfo::PrintDbg("PlaceOrder::Dump, %s(%s), %s, %d, %d", _place->_ID.c_str(), _place->_name.c_str(), _date.c_str(), _index, _type);
		}
		return 0;
	}
	bool operator==(PlaceOrder placeOrder) {
		if (placeOrder._place == _place
			&& placeOrder._date == _date
			&& placeOrder._index == _index
			&& placeOrder._type == _type)
			return true;
		return false;
	}
	bool operator!=(PlaceOrder placeOrder) {
		if (placeOrder._place != _place
			|| placeOrder._date != _date
			|| placeOrder._index != _index
			|| placeOrder._type != _type)
			return true;
		return false;
	}
public:
	const LYPlace* _place;
	std::string _date;
	int _index;
	unsigned char _type;
};

class RestaurantTime {
public:
	RestaurantTime(int type, int begin, int end, int time_cost, int min_time_cost) {
		Copy(type, begin, end, time_cost, min_time_cost);
	}
	RestaurantTime& operator= (const RestaurantTime& restTime) {
		if (this == &restTime) {
			return *this;
		}
		Copy(restTime._type, restTime._begin, restTime._end, restTime._time_cost, restTime._min_time_cost);
		return *this;
	}
	int Copy(int type, int begin, int end, int time_cost, int min_time_cost) {
		_type = type;
		_begin = begin;
		_end = end;
		_time_cost = time_cost;
		_min_time_cost = min_time_cost;
		return 0;
	}
public:
	int _type;
	int _begin;
	int _end;
	int _time_cost;
	int _min_time_cost;
};


class RouteBlock {
public:
	RouteBlock() {
		_arrive_place = NULL;
		_depart_place = NULL;
		_need_hotel = true;
		_hotel_info = NULL;
		_day_limit = 0;
		_city_idx = 0;
		_arrive_dur = 0;
		_depart_dur = 0;
		_checkIn = "";
		_checkOut = "";
		_delete_luggage_feasible = false;
	}
	~RouteBlock() {
		for (int i = 0; i < _hInfo_list.size(); ++i) {
			HInfo* hInfo = _hInfo_list[i];
			if (hInfo) {
				delete hInfo;
				hInfo = NULL;
			}
		}
	}
public:
	time_t _arrive_time;
	time_t _depart_time;
	time_t _arrive_dur;  // 到达地停留时间
	time_t _depart_dur;  // 出发地停留时间
	std::string _checkIn;	//入住日期,用于补全酒店
	std::string _checkOut;	//退房日期
	const LYPlace* _arrive_place;
	const LYPlace* _depart_place;
	std::vector<const LYPlace*>  _hotel_list;
	bool _need_hotel;
	const HotelInfo* _hotel_info; //暂时不用
	std::vector<HInfo*> _hInfo_list;//存储hotel的checkIn和checkOut信息

	int _day_limit;
	std::vector<std::string> _dates;
	std::vector<time_t> _day0_times;

	bool _delete_luggage_feasible;  // 在景点时间过短条件下可删除放行李酒店

	int _city_idx;
};

class PlanStats {
public:
	PlanStats() {
		m_trafTime = 0;
		m_leftTime = 0;
		m_throwNum = 0;
	}
public:
	int m_trafTime;
	int m_leftTime;
	int m_throwNum;
	std::tr1::unordered_map<const LYPlace*, int> m_placeStatsMap;
	std::tr1::unordered_map<const LYPlace*, int> m_placeDurMap;
public:
	int SetPlaceStats(const LYPlace* place, int stats);
	int DelPlaceStats(const LYPlace* place);
	bool HasPlaceStats(const LYPlace* place);
	int SetPlaceDur(const LYPlace* place, int dur);
	int DelPlaceDur(const LYPlace* place);
	int GetStats(Json::Value& stats);
	static std::string StatsToString(int stats);
};

#endif
