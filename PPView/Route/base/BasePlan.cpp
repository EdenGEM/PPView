#include <cmath>
#include <cstring>
#include "RouteConfig.h"
#include "TimeIR.h"
#include "TrafficData.h"
#include "BagParam.h"
#include "BasePlan.h"

bool debug = false;

const RestaurantTime BasePlan::m_breakfastTime(RESTAURANT_TYPE_BREAKFAST, 8 * 3600, 9 * 3600, 0.5 * 3600, 0.5 * 3600);
const RestaurantTime BasePlan::m_afternoonTeaTime(RESTAURANT_TYPE_AFTERNOON_TEA, 15.5 * 3600, 16.5 * 3600, 0.5 * 3600, 0.5 * 3600);
//const RestaurantTime BasePlan::m_lunchTime(RESTAURANT_TYPE_LUNCH, 11 * 3600, 14 * 3600, 1.5 * 3600, 45 * 60);
//const RestaurantTime BasePlan::m_supperTime(RESTAURANT_TYPE_SUPPER, 18 * 3600, 21 * 3600, 1.5 * 3600, 45 * 60);
const RestaurantTime BasePlan::m_lunchTime(RESTAURANT_TYPE_LUNCH, 11 * 3600, 14 * 3600, 45 * 60, 45 * 60);
const RestaurantTime BasePlan::m_supperTime(RESTAURANT_TYPE_SUPPER, 18 * 3600, 21 * 3600, 0 * 60, 0 * 60);

BasePlan::BasePlan() {
	// 请求相关
	m_AdultCount = 2;
	m_City = NULL;
	m_arvNullPlace = NULL;
	m_deptNullPlace = NULL;
	m_TimeZone = 1;
	m_ArrivePlace = NULL;
	m_DepartPlace = NULL;
	m_ArriveTime = 0;
	m_DepartTime = 0;

	//返回的页数 每页数量
	m_pageIndex = 0;
	m_numEachPage = 0;

	m_sortMode = 0;

	m_key = "";
	m_privateFilter = 0;
	// 请求选项
	m_QuerySource = QUERY_SOURCE_WEB;

	// 预设时长 todo: 不同city不同value
	m_ATRTimeCost = 0.5 * 3600;
	m_ATRDist = 1500;
	//机场
	m_EntryAirportTimeCost = 1.5 * 3600;
	m_ExitAirportTimeCost = 2.5 * 3600;
	//机场极限
	m_EntryZipAirportTimeCost = 1 * 3600;
	m_ExitZipAirportTimeCost = 1 * 3600;
	//火车站
	m_EntryStationTimeCost = 0.5 * 3600;
	m_ExitStationTimeCost = 0.5 * 3600;
	//火车站极限
	m_EntryZipStationTimeCost = 0.25 * 3600;
	m_ExitZipStationTimeCost = 0.25 * 3600;
	//长途汽车站
	m_EntryBusStationTimeCost = 15 * 60;
	m_ExitBusStationTimeCost = 15 * 60;
	//极限长途汽车站
	m_EntryZipBusStationTimeCost = 15 * 60;
	m_ExitZipBusStationTimeCost = 15 * 60;
	//轮船
	m_EntrySailStationTimeCost = 15 * 60;
	m_ExitSailStationTimeCost = 15 * 60;
	//极限轮船
	m_EntryZipSailStationTimeCost = 15 * 60;
	m_ExitZipSailStationTimeCost = 15 * 60;
	//租车
	m_EntryCarStoreTimeCost = 15 * 60;
	m_ExitCarStoreTimeCost = 15 * 60;
	//极限租车
	m_EntryZipCarStoreTimeCost = 15 * 60;
	m_ExitZipCarStoreTimeCost = 15 * 60;
	//coreHotel的时长
	m_EntryHotelTimeCost = 0;
	m_ExitHotelTimeCost = 0;

	m_MinSleepTimeCost = 3600 * 4;  // 最少睡4小时
	m_MaxSleepTimeCost = 3600 * 12;  // 最多晚8点-早8点
	m_AvgSleepTimeCost = 3600 * 10;
	m_sleepCut = 30 * 60;
	m_LeaveLuggageTimeCost = 0.5 * 3600;  // 办入住，放行李
	m_ReclaimLuggageTimeCost = 0.5 * 3600;  // 取行李
	m_Day0LeastViewTimeCost = 3600 * 1.5;  // 放完行李如果出去玩，最少得玩1.5小时
	m_MaxWaitOpenTimeCost = 3 * 3600;  // 最大等开门时间

	m_HotelOpenTime = 20 * 3600;
	m_HotelCloseTime = 9 * 3600;

	// 局部调整比例
	m_KeyNodeFixRatio = 0.75;

	// 偏好
	m_trafPrefer = TRAF_PREFER_AI;
	m_scalePrefer = 1.0;

	// POI点-hotel
	// POI点-shop
	m_shopIntensity = SHOP_INTENSITY_NULL;
	m_ShopNeedNum = 0;
	// POI点-rest
	m_DefaultRestaurantComp = RESTAURANT_TYPE_LUNCH | RESTAURANT_TYPE_SUPPER;
	m_RestNeedNum = 0;
	m_RestaurantTimeList.push_back(m_breakfastTime);
	m_RestaurantTimeList.push_back(m_lunchTime);
	m_RestaurantTimeList.push_back(m_afternoonTeaTime);
	m_RestaurantTimeList.push_back(m_supperTime);
	// POI点-view
	m_MinViewDur = 30 * 60;
	// POI点

	// 性能统计
	m_CutThres = 10000000;
	m_CutNullThres = m_CutThres * 0.5;

	m_reqMode = 0;

	m_runType = RUN_NULL;
	m_availDur = 0;

	m_richPlace = false;
	m_faultTolerant = false;
	//SMZ
	m_realTrafNeed = REAL_TRAF_NULL;
	m_useKpi = false;
	m_useStaticTraf = false;
	m_travalType = TRAVAL_MODE_NORMAL;
	m_useDay17Limit = false;
	m_notPlanCityPark = false;
	m_planAfterZip = false;//极限压缩的情况 不规划
	m_dayOneLast3hNotPlan = false;//day1 和 day last 的3h可用游玩时间不够时 不规划
	//SMZ END
	m_CutTimer.start();

	m_useCBP = true;
	m_useRealTraf = true;
	m_useTrafShow = true;

	//add by yangshu
	m_isPlan = true;
	//yangshu end

	m_keepTime = true; //时间相关的是否保持,包括开放日期、到达时刻和游玩时长
	m_keepPlayRange = true; //是否保持每日出发返回时间 智能优化时根据此标志 将HotelOpen CloseTime赋给酒店
	pthread_mutex_init(&m_durLocker, NULL);

    // 列表页过滤项
    m_utime = -1;
    m_maxDist = -1;
	//是否直接去游玩 s125 extra 为true表示直接游玩
	m_isImmediately = false;
	isChangeTraffic = false;
	clashDelPoi = false;
}

int BasePlan::Release() {
	pthread_mutex_destroy(&m_durLocker);
	ClearBlock();
	for (int i = 0; i < m_BlockList.size(); ++i) {
		delete m_BlockList[i];
	}
	m_BlockList.clear();
	for (int i = 0; i < m_varPlaceInfoList.size(); ++i) {
		delete m_varPlaceInfoList[i];
	}
	m_varPlaceInfoList.clear();
	for (int i = 0; i < m_RouteBlockList.size(); ++i) {
		delete m_RouteBlockList[i];
	}
	m_RouteBlockList.clear();
	std::tr1::unordered_map<std::string, TrafficItem*>::iterator it;
	for (it = m_TrafficMap.begin(); it != m_TrafficMap.end(); ++it ) {
		delete it->second;
	}
	m_TrafficMap.clear();

	std::tr1::unordered_map<std::string, TrafficItem*>::iterator customTrafIt;
	for (customTrafIt = m_customTrafMap.begin(); customTrafIt != m_customTrafMap.end(); ++customTrafIt ) {
		delete customTrafIt->second;
	}
	m_customTrafMap.clear();

	for (auto it = m_lastTripTrafMap.begin(); it != m_lastTripTrafMap.end(); it++) {
		delete it->second;
	}
	m_lastTripTrafMap.clear();

	for (auto customPoiIt = m_customPlaceMap.begin(); customPoiIt != m_customPlaceMap.end(); ++customPoiIt ) {
		delete (customPoiIt->second);
	}
	m_customPlaceMap.clear();

	for (int i = 0 ;i < m_KeyNode.size(); ++i) {
		delete m_KeyNode[i];
	}
	m_KeyNode.clear();

	for (std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.begin(); it != m_durSMap.end(); ++it) {
		delete it->second;
	}
	m_durSMap.clear();

	for (auto it = m_multiPlaceMap.begin(); it != m_multiPlaceMap.end(); it ++) {
		delete it->second;
	}
	m_multiPlaceMap.clear();

	m_placeNumberMap.clear();
	return 0;
}

std::pair<time_t, time_t> BasePlan::GetDayRange(const std::string& date) {
	std::tr1::unordered_map<std::string, std::pair<time_t, time_t> >::iterator it;
	if ((it = m_dayRangeMap.find(date)) != m_dayRangeMap.end()) {
		return it->second;
	} else {
		char buff[1000] = {};
		snprintf(buff, sizeof(buff), "%s_%02d:%02d",date.c_str(), m_HotelCloseTime / 3600, m_HotelCloseTime % 3600 / 60);
		time_t closetime = MJ::MyTime::toTime(std::string(buff), m_TimeZone);
		snprintf(buff, sizeof(buff), "%s_%02d:%02d",date.c_str(), m_HotelOpenTime / 3600, m_HotelOpenTime % 3600 / 60);
		time_t opentime = MJ::MyTime::toTime(std::string(buff), m_TimeZone);
		return std::pair<time_t, time_t> (closetime, opentime);
	}
}

int BasePlan::GetBlockNum() const {
	return m_BlockList.size();
}
TimeBlock* BasePlan::GetBlock(int index) const {
	if (index >= m_BlockList.size()) {
		return NULL;
	} else {
		return m_BlockList[index];
	}
}
int BasePlan::PushBlock(time_t start, time_t stop, const std::string& trafDate, int avail_dur, uint8_t restNeed, double time_zone) {
	m_BlockList.push_back(new TimeBlock(start, stop, trafDate, avail_dur, restNeed, time_zone));
	return 0;
}
int BasePlan::ClearBlock() {
	for (int i = 0; i < m_BlockList.size(); ++i) {
		delete m_BlockList[i];
	}
	m_BlockList.clear();
	return 0;
}


// 在指定日期是否可拓展
int BasePlan::IsUserDateAvail(const LYPlace* place, const std::string& date) {
	std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> >::iterator it = m_PlaceDateMap.find(place->_ID);
	if (it == m_PlaceDateMap.end()) {
		return true;//没指定日期返回可用
	} else {
		std::tr1::unordered_set<std::string>& avail_date_set = it->second;
		if (avail_date_set.find(date) != avail_date_set.end()) {
			return true;
		} else {
			return false;
		}
	}
}

// 解析vPlace的开关门时间与价格，填写varPlaceInfo
int BasePlan::VarPlace2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo) {
	int nodeFunc = NODE_FUNC_PLACE;
	if (vPlace->_t & LY_PLACE_TYPE_VIEW) {
		View2PlaceInfo(vPlace, date, pInfo);
		nodeFunc = NODE_FUNC_PLACE_VIEW;
	} else if (vPlace->_t & LY_PLACE_TYPE_RESTAURANT) {
		Restaurant2PlaceInfo(vPlace, date, pInfo);
		nodeFunc = NODE_FUNC_PLACE_RESTAURANT;
	} else if (vPlace->_t & LY_PLACE_TYPE_SHOP) {
		Shop2PlaceInfo(vPlace, date, pInfo);
		nodeFunc = NODE_FUNC_PLACE_SHOP;
	} else if (vPlace->_t & LY_PLACE_TYPE_TOURALL) {
		Tour2PlaceInfo(vPlace, date, pInfo);
		nodeFunc = NODE_FUNC_PLACE_TOUR;
	}

	if (m_faultTolerant && !pInfo) {
		pInfo = new PlaceInfo;
		pInfo->m_vPlace = vPlace;
		pInfo->m_timeZone = m_TimeZone;
		pInfo->m_type = nodeFunc;
		pInfo->m_cost = 0;
		time_t day0_time = MJ::MyTime::toTime(date + "_00:00", pInfo->m_timeZone);
		OpenClose* openClose = new OpenClose(day0_time, day0_time, day0_time);
		pInfo->m_openCloseList.push_back(openClose);
		pInfo->m_arvID = vPlace->_ID;
		pInfo->m_deptID = pInfo->m_arvID;

		m_varPlaceInfoList.push_back(pInfo);
	}
	return 0;
}

int BasePlan::View2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo) {
	int ret = 0;
	char buff[100];

	if (m_lookSet.find(vPlace) != m_lookSet.end()) {
		pInfo = new PlaceInfo;
		pInfo->m_vPlace = vPlace;
		pInfo->m_timeZone = m_TimeZone;
		pInfo->m_type = NODE_FUNC_PLACE_VIEW;
		snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
		time_t day0_time = MJ::MyTime::toTime(buff, pInfo->m_timeZone);
		OpenClose* openClose = new OpenClose(day0_time, day0_time + 24 * 3600, day0_time + 24 * 3600 - GetMinDur(vPlace));
		pInfo->m_openCloseList.push_back(openClose);
		pInfo->m_arvID = vPlace->_ID;
		pInfo->m_deptID = pInfo->m_arvID;

		m_varPlaceInfoList.push_back(pInfo);
		return 0;
	}

	pInfo = new PlaceInfo;
	pInfo->m_vPlace = vPlace;
	pInfo->m_timeZone = m_TimeZone;
	pInfo->m_type = NODE_FUNC_PLACE_VIEW;
	pInfo->m_cost = 0;
	pInfo->m_arvID = vPlace->_ID;
	pInfo->m_deptID = vPlace->_ID;

	time_t open;
	time_t close;
	std::vector<std::pair<int, int>> datelist;
	ret = GetOpenCloseTime(date, vPlace, datelist);
	if (ret != 0 || datelist.size() == 0) {
		delete pInfo;
		pInfo = NULL;
		MJ::PrintInfo::PrintDbg("[%s]BasePlan::View2PlaceInfo, GetOpenCloseTime err, vid: %s", m_qParam.log.c_str(), vPlace->_ID.c_str());
		return 1;
	}
	open = datelist.front().first;
	close = datelist.back().second;
	time_t latestArv = 0;
	if (m_vPlaceOpenMap.find(vPlace->_ID) != m_vPlaceOpenMap.end()) {
		latestArv = open;
	} else if (m_UserDurMap.find(vPlace->_ID) != m_UserDurMap.end()) {
		latestArv = close - GetZipDur(vPlace);
	} else {
		latestArv = close - GetMinDur(vPlace);
	}
	OpenClose* openClose = new OpenClose(open, close, latestArv);
	pInfo->m_openCloseList.push_back(openClose);

	m_varPlaceInfoList.push_back(pInfo);

	return 0;
}

int BasePlan::Tour2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo) {
	int ret = 0;
	char buff[100];

	//add by yangshu
	{
		const Tour* tour = dynamic_cast<const Tour*>(vPlace);
		if(tour) {
			time_t open, close;
			std::vector<std::pair<int, int>> datelist;
			int ret = GetOpenCloseTime(date, vPlace, datelist);
			if (ret != 0 || datelist.size() == 0) return 0;

			pInfo = new PlaceInfo;
			pInfo->m_vPlace = vPlace;
			pInfo->m_timeZone = m_TimeZone;
			pInfo->m_type = NODE_FUNC_PLACE_TOUR;
			pInfo->m_cost = 0;

			for (std::pair<int, int> openClosePair : datelist) {
				open = openClosePair.first;
				close = openClosePair.second;
				int dur = close - open;
				if (m_UserDurMap.find(tour->_ID) != m_UserDurMap.end()) {
					m_UserDurMap[tour->_ID] = dur;
				}
				OpenClose* openClose = new OpenClose(open, close, open, 0);
				pInfo->m_openCloseList.push_back(openClose);
			}

			pInfo->m_arvID = tour->_ID;
			pInfo->m_deptID = pInfo->m_arvID;
			m_varPlaceInfoList.push_back(pInfo);
		}
	}

	return 0;
}

int BasePlan::Shop2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo) {
	int ret = 0;
	char buff[100];

	if (m_lookSet.find(vPlace) != m_lookSet.end()) {
		pInfo = new PlaceInfo;
		pInfo->m_vPlace = vPlace;
		pInfo->m_timeZone = m_TimeZone;
		pInfo->m_type = NODE_FUNC_PLACE_SHOP;
		snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
		time_t day0_time = MJ::MyTime::toTime(buff, pInfo->m_timeZone);
		OpenClose* openClose = new OpenClose(day0_time, day0_time + 24 * 3600, day0_time + 24 * 3600 - GetMinDur(vPlace), 0);
		pInfo->m_openCloseList.push_back(openClose);

		pInfo->m_arvID = vPlace->_ID;
		pInfo->m_deptID = pInfo->m_arvID;
		m_varPlaceInfoList.push_back(pInfo);
		return 0;
	}

	// 2 开关门时间
	pInfo = new PlaceInfo;
	pInfo->m_vPlace = vPlace;
	pInfo->m_timeZone = m_TimeZone;
	pInfo->m_type = NODE_FUNC_PLACE_SHOP;
	pInfo->m_cost = 0;

	time_t open;
	time_t close;
	std::vector<std::pair<int, int>> datelist;
	ret = GetOpenCloseTime(date, vPlace, datelist);
	if (ret != 0 || datelist.size() == 0) {
		delete pInfo;
		pInfo = NULL;
		MJ::PrintInfo::PrintErr("[%s]BasePlan::Shop2PlaceInfo, GetOpenCloseTime err, shopId: %s", m_qParam.log.c_str(), vPlace->_ID.c_str());
		return 1;
	}
	open = datelist.front().first;
	close = datelist.back().second;
	time_t latestArv = 0;
	if (m_vPlaceOpenMap.find(vPlace->_ID) != m_vPlaceOpenMap.end()) {
		latestArv = open;
	} else {
		latestArv = close - GetMinDur(vPlace);
	}
	OpenClose* openClose = new OpenClose(open, close, latestArv, 0);
	pInfo->m_openCloseList.push_back(openClose);

	pInfo->m_arvID = vPlace->_ID;
	pInfo->m_deptID = pInfo->m_arvID;
	m_varPlaceInfoList.push_back(pInfo);

	return 0;
}

int BasePlan::Restaurant2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo) {
	int ret = 0;
	char buff[100];

	int restType = m_DefaultRestaurantComp;
	if (m_RestTypeMap.find(vPlace->_ID) != m_RestTypeMap.end()) {
		restType = m_RestTypeMap[vPlace->_ID];
	}

	//能获取开关门
	pInfo = new PlaceInfo;
	for (int i = 0; i < m_RestaurantTimeList.size(); ++i) {
		const RestaurantTime& restTime = m_RestaurantTimeList[i];
		if (!(restType & restTime._type)) continue;
		//hy 目前只要午餐和晚餐
		if (restTime._type != RESTAURANT_TYPE_LUNCH && restTime._type != RESTAURANT_TYPE_SUPPER) continue;

		pInfo->m_vPlace = vPlace;
		pInfo->m_timeZone = m_TimeZone;
		pInfo->m_cost = 0;
		pInfo->m_arvID = vPlace->_ID;
		pInfo->m_deptID = pInfo->m_arvID;

		time_t open;
		time_t close;
		std::vector<std::pair<int, int>> datelist;
		ret = GetOpenCloseTime(date, vPlace, datelist);
		if (ret != 0 || datelist.size() == 0) continue;

		open = datelist.front().first;
		close = datelist.back().second;
		OpenClose* openClose = new OpenClose(open, close, close - restTime._min_time_cost, restTime._type);
		int dur = openClose->m_close - openClose->m_open;
		pInfo->m_dur = dur;

		//只有用户未指定时才严格检查 ssv006
		if (m_UserDurMap.find(vPlace->_ID) == m_UserDurMap.end()) {
			snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
			time_t day0_time = MJ::MyTime::toTime(buff, m_TimeZone);
			time_t start_time = day0_time + restTime._begin;
			time_t stop_time = day0_time + restTime._end;
			if (openClose->m_open < start_time) {
				openClose->m_open = start_time;
			}
			if (openClose->m_close > stop_time) {
				openClose->m_close = stop_time;
			}
			// 餐厅为必选 但该段开关门有问题 将openClose设为标准的
			if (openClose->m_close < openClose->m_open && m_userMustPlaceSet.count(vPlace)) {
				openClose->m_open = start_time;
				openClose->m_close = stop_time;
				openClose->m_latestArv = close - restTime._min_time_cost;
			}
		}
		openClose->m_latestArv = openClose->m_close - restTime._min_time_cost;
		if (openClose->m_close < openClose->m_open) {
			delete openClose;
			openClose = NULL;
			continue;
		}

		if (restTime._type == RESTAURANT_TYPE_BREAKFAST) {
			pInfo->m_type |= NODE_FUNC_PLACE_REST_BREAKFAST;
		} else if (restTime._type == RESTAURANT_TYPE_LUNCH) {
			pInfo->m_type |= NODE_FUNC_PLACE_REST_LUNCH;
		} else if (restTime._type == RESTAURANT_TYPE_AFTERNOON_TEA) {
			pInfo->m_type |= NODE_FUNC_PLACE_REST_AFTERNOON_TEA;
		} else if (restTime._type == RESTAURANT_TYPE_SUPPER) {
			pInfo->m_type |= NODE_FUNC_PLACE_REST_SUPPER;
		}

		pInfo->m_openCloseList.push_back(openClose);
	}
	if (pInfo->m_openCloseList.empty()) {
		delete pInfo;
		pInfo = NULL;
		//也不在随便看看集合中
		if (m_lookSet.find(vPlace) == m_lookSet.end()) {
			return 1;
		}
	} else {
		m_varPlaceInfoList.push_back(pInfo);
		return 0;
	}

	// 未能获取开关门信息且在随便看看集合中 容错
	if (m_lookSet.find(vPlace) != m_lookSet.end()) {
		for (int i = 0; i < m_RestaurantTimeList.size(); ++i) {
			const RestaurantTime& restTime = m_RestaurantTimeList[i];
			if (!(restType & restTime._type)) continue;
			//hy 目前只要午餐和晚餐
			if (restTime._type != RESTAURANT_TYPE_LUNCH && restTime._type != RESTAURANT_TYPE_SUPPER) continue;

			pInfo = new PlaceInfo;
			pInfo->m_vPlace = vPlace;
			pInfo->m_timeZone = m_TimeZone;
			pInfo->m_type = NODE_FUNC_PLACE_RESTAURANT;
			pInfo->m_cost = 0;

			snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
			time_t day0_time = MJ::MyTime::toTime(buff, pInfo->m_timeZone);
			OpenClose* openClose = new OpenClose(day0_time + restTime._begin, day0_time + restTime._end, day0_time + restTime._end - restTime._min_time_cost, restTime._type);
			pInfo->m_openCloseList.push_back(openClose);

			if (restTime._type == RESTAURANT_TYPE_BREAKFAST) {
				pInfo->m_type |= NODE_FUNC_PLACE_REST_BREAKFAST;
			} else if (restTime._type == RESTAURANT_TYPE_LUNCH) {
				pInfo->m_type |= NODE_FUNC_PLACE_REST_LUNCH;
			} else if (restTime._type == RESTAURANT_TYPE_AFTERNOON_TEA) {
				pInfo->m_type |= NODE_FUNC_PLACE_REST_AFTERNOON_TEA;
			} else if (restTime._type == RESTAURANT_TYPE_SUPPER) {
				pInfo->m_type |= NODE_FUNC_PLACE_REST_SUPPER;
			}

			pInfo->m_arvID = vPlace->_ID;
			pInfo->m_deptID = pInfo->m_arvID;
			m_varPlaceInfoList.push_back(pInfo);
		}
		return 0;
	}
	return 1;
}

// 按规则获取当天及前后天开关门时间
int BasePlan::GetAdjacentDaysOpenCloseTime(const std::string& date, const LYPlace* place, std::vector<std::pair<int, int>>& openCloseList) {
	const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
	if (vPlace == NULL) return 1;
	for (int i = -1; i <= 1; i++) {
		std::string date_0 = MJ::MyTime::datePlusOf(date,i);
		std::vector<std::pair<int, int>> datelist;
		if (GetOpenCloseTime(date_0, vPlace, datelist) == 0) {
			for (auto datell: datelist) {
				std::cerr << "open : " << MJ::MyTime::toString(datell.first, m_TimeZone) << "  close: " << MJ::MyTime::toString(datell.second, m_TimeZone) << std::endl;
			}
			if (openCloseList.size() and datelist.front().first - openCloseList.back().second <= 10 * 60) {
				datelist.front().first = openCloseList.back().first;
				openCloseList.pop_back();
			}
			openCloseList.insert(openCloseList.end(), datelist.begin(), datelist.end());
		}
	}
	if (openCloseList.size() == 0) return 1;
	return 0;
}
int BasePlan::GetOpenCloseTime(const std::string& date, const VarPlace* vPlace, std::vector<std::pair<int, int>>& openCloseList) {
	int ret = 0;

	//1. 先用_time_rules 判断是否开门 (玩乐无效)
	//2. 如果是用户必去点或者玩乐, 指定了游玩时间 用该时间判断
	//3. 玩乐可用, 放入所有的times
	time_t open, close;
	std::vector<std::string> time_rules;
	TimeIR::getTheDateOpenTimeRange(vPlace->_time_rule, date, time_rules);
	for (int i = 0; i < time_rules.size(); i++) {
		std::string time_rule = time_rules[i];
		ret = ToolFunc::getOpenCloseTime(date, time_rule, m_TimeZone, open, close);
		std::string strOpen = MJ::MyTime::toString(open, m_TimeZone);
		std::string strClose = MJ::MyTime::toString(close, m_TimeZone);
		if (open >= close || time_rule.size() < strlen("20140827_09:00-20140827_18:00") || ToolFunc::FormatChecker::CheckTime(strOpen) || ToolFunc::FormatChecker::CheckTime(strClose)) {
			MJ::PrintInfo::PrintErr("[%s]BasePlan::GetOpenCloseTime error, Place: %s(%s), date %s, _time_rule: %s, time_rule: %s , open %s, close %s\n",
					m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str(), date.c_str(), vPlace->_time_rule.c_str(), time_rule.c_str(),
					MJ::MyTime::toString(open, m_TimeZone).c_str(),
					MJ::MyTime::toString(close, m_TimeZone).c_str());
		}
		if (ret == 0 && open < close && !ToolFunc::FormatChecker::CheckTime(strOpen) && !ToolFunc::FormatChecker::CheckTime(strClose)) {
			openCloseList.push_back(std::make_pair(open,close));
		}
	}
	if(openCloseList.size()<=0
			&& m_vPlaceOpenMap.find(vPlace->_ID) == m_vPlaceOpenMap.end()
			&& !(vPlace->_t & LY_PLACE_TYPE_TOURALL)) {
		return 1;
	}
	//租车和玩乐均不会被放入lookSet 景点无法满足用户要求的到达时间时，会被放入lookSet
	if (m_vPlaceOpenMap.find(vPlace->_ID) != m_vPlaceOpenMap.end() && m_lookSet.find(vPlace) == m_lookSet.end() ) {
		std::pair<int, int>& offset = m_vPlaceOpenMap[vPlace->_ID];
		time_t day0 = MJ::MyTime::toTime(date + "_00:00", m_TimeZone);
		open = day0 + offset.first;
		close = day0 + offset.second;
		if (vPlace->_t & LY_PLACE_TYPE_TOURALL) {
			openCloseList.clear();
			openCloseList.push_back(std::make_pair(open, close));
			const Tour* tour = dynamic_cast<const Tour*>(vPlace);
			if (tour == NULL) return 1;
			return not LYConstData::IsTourAvailable(tour, date);
		}
		for(auto it = openCloseList.begin();it != openCloseList.end(); it++){
			time_t rule_open=it->first;
			time_t rule_close=it->second;
			int rcmdDur = GetRcmdDur(vPlace);
			if(open >= rule_open && open + rcmdDur <= rule_close){
				close = open+rcmdDur;
				openCloseList.clear();
				openCloseList.push_back(std::make_pair(open, close));
				return 0;
			}
		}
		openCloseList.clear();
	}
	if(!openCloseList.empty())return 0;
	//主动规划获取多个场次
	if (vPlace->_t & LY_PLACE_TYPE_TOURALL) {
		const Tour* tour = dynamic_cast<const Tour*>(vPlace);
		if (tour == NULL) return 1;
		if (not LYConstData::IsTourAvailable(tour, date)) return 1;
		time_t day0 = MJ::MyTime::toTime(date + "_00:00", m_TimeZone);
		for (int i = 0; i < tour->m_srcTimes.size(); i ++) {
			if (tour->m_srcTimes[i]["t"].isString()) {
				std::string calTime = tour->m_srcTimes[i]["t"].asString();
				if (not calTime.empty()) {
					int dur = 1800;
					if (tour->_rcmd_intensity) {
						dur = tour->_rcmd_intensity->_dur;
					}
					int startOffset = 0;
					ToolFunc::toIntOffset(calTime, startOffset);
					int endOffset = startOffset+dur;
					openCloseList.push_back(std::make_pair(day0+startOffset, day0+endOffset));
				} else {
					//无场次玩乐
					openCloseList.clear();
					open = day0;
					close = day0 + 3600*24;
					openCloseList.push_back(std::make_pair(open, close));
					return 0;
				}
			}
		}
	}
	if(!openCloseList.empty())return 0;
	MJ::PrintInfo::PrintErr("[%s]BasePlan::GetOpenCloseTime error, Place: %s(%s), date %s, _time_rule: %s\n",m_qParam.log.c_str(), vPlace->_ID.c_str(), vPlace->_name.c_str(), date.c_str(), vPlace->_time_rule.c_str());
	return 1;
}

// 获取点的平均交通时间
// 平均时间随选点而改变
int BasePlan::GetAvgTrafTime(const std::string& id) {
	if (id == LYConstData::m_attachRest->_ID || id == LYConstData::m_segmentPlace->_ID) return 0;
	std::tr1::unordered_map<std::string, int>::iterator it = m_AvgTrafTimeMap.find(id);
	if (it != m_AvgTrafTimeMap.end()) {
		return m_AvgTrafTimeMap[id];
	}
	return m_ATRTimeCost;
}

int BasePlan::GetAvgTrafDist(const std::string& id) {
	if (id == LYConstData::m_attachRest->_ID || id == LYConstData::m_segmentPlace->_ID) return 0;
	std::tr1::unordered_map<std::string, int>::iterator it = m_AvgTrafDistMap.find(id);
	if (it != m_AvgTrafDistMap.end()) {
		return m_AvgTrafDistMap[id];
	}
	return m_ATRDist;
}

int BasePlan::GetAllocDur(const LYPlace* place) {
	if (m_allocDurMap.find(place->_ID) != m_allocDurMap.end()) {
		return m_allocDurMap[place->_ID];
	} else {
		return GetAvgAllocDur(place);
	}
}

int BasePlan::GetAvgAllocDur(const LYPlace* place) {
	return (GetRcmdDur(place) + GetAvgTrafTime(place->_ID));
}

int BasePlan::GetMinAllocDur(const LYPlace* place) {
	return (GetMinDur(place) + GetAvgTrafTime(place->_ID));
}

int BasePlan::GetRcmdDur(const LYPlace* place) {
	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
	if (it == m_durSMap.end()) {
		const DurS durS = GetDurS(place);
		return durS.m_rcmd;
	}
	return it->second->m_rcmd;
}
int BasePlan::GetMinDur(const LYPlace* place) {
	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
	if (it == m_durSMap.end()) {
		const DurS durS = GetDurS(place);
		return durS.m_min;
	}
	return it->second->m_min;
}
int BasePlan::GetMaxDur(const LYPlace* place) {
	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
	if (it == m_durSMap.end()) {
		const DurS durS = GetDurS(place);
		return durS.m_max;
	}
	return it->second->m_max;
}
int BasePlan::GetZipDur(const LYPlace* place) {
	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
	if (it == m_durSMap.end()) {
		const DurS durS = GetDurS(place);
		return durS.m_zip;
	}
	return it->second->m_zip;
}

int BasePlan::GetExtendDur(const LYPlace* place) {
	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
	if (it == m_durSMap.end()) {
		const DurS durS = GetDurS(place);
		return durS.m_extend;
	}
	return it->second->m_extend;
}

const DurS* BasePlan::GetDurSP(const LYPlace* place) {
	if (m_durSMap.find(place->_ID) != m_durSMap.end()) {
		return m_durSMap[place->_ID];
	}
	return NULL;
}

int BasePlan::SetDurS(const LYPlace* place, const DurS* durS) {
	int ret = SetDurS(place, *durS);
	if (ret) {
		return 1;
	}
	return 0;
}

int BasePlan::SetDurS(const LYPlace* place, const DurS durS) {
	int ret = SetDurS(place, durS.m_min, durS.m_zip, durS.m_rcmd, durS.m_extend, durS.m_max);
	if (ret) {
		return 1;
	}
	return 0;
}

int BasePlan::SetDurS(const LYPlace* place, int min, int zip, int rcmd, int extend, int max) {
	DurS* durS = new DurS(min, zip, rcmd, extend, max);
	if (m_durSMap.find(place->_ID) != m_durSMap.end()) {
		delete m_durSMap[place->_ID];
	}
	MJ::PrintInfo::PrintDbg("[%s]BasePlan::SetDurS %s(%s) min = %d, zip = %d, rcmd = %d, extend = %d, max = %d", m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str(), durS->m_min, durS->m_zip, durS->m_rcmd, durS->m_extend, durS->m_max);
	m_durSMap[place->_ID] = durS;
	return 0;
}

//const DurS BasePlan::GetDurS(const LYPlace* place) {
//	std::tr1::unordered_map<std::string, DurS*>::iterator it = m_durSMap.find(place->_ID);
//	if (it == m_durSMap.end()) {
//		CalDur(place);
//	}
//	it = m_durSMap.find(place->_ID);
//	if (it == m_durSMap.end()) return NULL;
//	return it->second;
//}

const DurS BasePlan::GetDurS(const LYPlace* place) {
	if (m_durSMap.find(place->_ID) != m_durSMap.end()) {
		return (*m_durSMap[place->_ID]);
	}
	int rcmdDur = 30 * 60;
	int extendDur = 30 * 60;
	int zipDur = 30 * 60;
	int minDur = 15 * 60;
	int maxDur = 60 * 60;
	if (m_UserDurMap.find(place->_ID) != m_UserDurMap.end()) {
		int userDur = m_UserDurMap[place->_ID];
		minDur = userDur;
		zipDur = userDur;
		rcmdDur = userDur;
		extendDur = userDur;
		maxDur = userDur;
		//if(place->getRawType() & LY_PLACE_TYPE_CAR_STORE)
		//{
		//	minDur *= 0.5;
		//	zipDur *= 0.5;
		//}
	} else if (m_vPlaceOpenMap.find(place->_ID) != m_vPlaceOpenMap.end()) {
		int userDur = m_vPlaceOpenMap[place->_ID].second - m_vPlaceOpenMap[place->_ID].first;
		rcmdDur = userDur;
		minDur = userDur - 10 * 60;
		maxDur = userDur + 10 * 60;
		zipDur = userDur;
		extendDur = userDur;
	} else if (place->_t & LY_PLACE_TYPE_RESTAURANT) {
		rcmdDur = 2 * 3600;
		minDur = 45 * 60;
		maxDur = 2 * 3600;
		zipDur = 1.5 * 3600;
		extendDur = 2 * 3600;
	} else if (place->_t & LY_PLACE_TYPE_VAR_PLACE) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
		if (!vPlace->_intensity_list.empty() && vPlace->_rcmd_intensity) {
			rcmdDur = vPlace->_rcmd_intensity->_dur;
			//add by yangshu
			{
				if (m_scalePrefer < 0.9) {
					//压缩
					//游玩时长压缩为推荐值的70%,按0.5h为单位取近似值
					//tag为主题乐园,或推荐游玩时长大于8h的,游玩时长不压缩
					//推荐游玩时长为1h的,压缩为30min;推荐时长为30min,15min的,不压缩
					//std::cerr << "yangshu zip" << std::endl;
					if(rcmdDur > 1800 && rcmdDur < 3600*8) {
						rcmdDur *= m_scalePrefer;
						int hour = rcmdDur / 1800;
						int second = rcmdDur % 1800;
						if (second < 900) {
							rcmdDur = hour*1800;
						} else {
							rcmdDur = (hour+1)*1800;
						}
						if(rcmdDur <= 0) {
							rcmdDur = 900;
						}
					}
				} else if (m_scalePrefer > 1.1) {
					//拉伸
					//游玩时长拉伸为推荐值的150%,按0.5h为单位取近似值
					//推荐游玩时长大于等于6h的不拉伸
					//推荐游玩时长为15min的拉伸为30min,30min的拉伸为1h
					//std::cerr << "yangshu lashen" << std::endl;
					if (rcmdDur <= 3600*6) {
						rcmdDur *= (m_scalePrefer+0.01);
						int hour = rcmdDur / 1800;
						int second = rcmdDur % 1800;
						if (second < 900) {
							rcmdDur = hour*1800;
						} else {
							rcmdDur = (hour+1)*1800;
						}
						if(rcmdDur > 3600*8) {
							rcmdDur = 3600*8;
						}
					}
				}
			}
			//add end
			minDur = vPlace->_intensity_list.front()->_dur;
			maxDur = vPlace->_intensity_list.back()->_dur;
			if (vPlace->_ranking <= 3) {
//				std::cerr << "jjj zip ranking < 3 0.9\t" << vPlace->_ID << " " << vPlace->_hot_rank << " " << vPlace->_name << std::endl;
				zipDur = rcmdDur * 0.9;
				extendDur = rcmdDur * 1.9;
			} else if (vPlace->_hot_rank < 0.1) {
				//有价格信息 0.8 1.8
				//无价格信息 0.7 1.6
				//std::cerr << "jjj zip hot_rank < 0.1 0.7\t" << vPlace->_ID << " " << vPlace->_hot_rank << " " << vPlace->_name << std::endl;
				zipDur = rcmdDur * 0.7;
				extendDur = rcmdDur * 1.6;
			} else if (vPlace->_hot_rank < 0.3) {
//				std::cerr << "jjj zip hot_rank < 0.3 0.5\t" << vPlace->_ID << " " << vPlace->_hot_rank << " " << vPlace->_name << std::endl;
				zipDur = rcmdDur * 0.5;
				extendDur = rcmdDur * 1.5;
			} else {
//				std::cerr << "jjj zip other min\t" << vPlace->_ID << " " << vPlace->_hot_rank << " " << vPlace->_name << std::endl;
				zipDur = minDur;
				extendDur = rcmdDur * 1.0;
			}
			// tag控制
			// 主题乐园不压缩
			std::string tagpark = LYConstData::GetTagStrByName("主题乐园");
			if ( tagpark != "" && vPlace->m_tagSmallStr != "" && (vPlace->m_tagSmallStr.find(tagpark) != std::string::npos)) {
				zipDur = rcmdDur;
				minDur = rcmdDur;
				extendDur = maxDur;
			}
			const Tour *tour = dynamic_cast<const Tour *>(place);
			if (tour != NULL) {
				minDur += tour->m_preTime;
				maxDur += tour->m_preTime;
				zipDur += tour->m_preTime;
				extendDur += tour->m_preTime;
				rcmdDur += tour->m_preTime;
			}

			zipDur = std::max(zipDur, minDur);
			extendDur = std::min(extendDur, maxDur);
//			std::cerr << "jjj zip " << vPlace->_ID << " " << zipDur << " " << rcmdDur << " " << extendDur << std::endl;
//			std::cerr << "jjj zip " << vPlace->_ID << " " << minDur << " " << maxDur << std::endl;
		}
	}
	const DurS durS(minDur, zipDur, rcmdDur, extendDur, maxDur);
	MJ::PrintInfo::PrintDbg("[%s]BasePlan::GetDurS %s(%s) min = %d, zip = %d, rcmd = %d, extend = %d, max = %d", m_qParam.log.c_str(), place->_ID.c_str(), place->_name.c_str(), durS.m_min, durS.m_zip, durS.m_rcmd, durS.m_extend, durS.m_max);
	return durS;
}

//获取热度
int BasePlan::GetHot(const LYPlace* place) {
	if (m_hotMap.find(place->_ID) != m_hotMap.end()) {
		return m_hotMap[place->_ID];
	}
	return 0;
}

int BasePlan::DumpPath(bool log) {
	m_PlanList.Dump(this, log);
	return 0;
}

int BasePlan::GetKeyNum() const {
	return m_KeyNode.size();
}

KeyNode* BasePlan::GetKey(int index) const {
	if (index >= m_KeyNode.size()) return NULL;
	return m_KeyNode[index];
}


std::tr1::unordered_map<std::string, int>& BasePlan::GetCrossMap() {
	return m_crossMap;
}

const TrafficItem* BasePlan::GetTraffic(const std::string& ida, const std::string& idb) {
	std::string cutIda = GetCutId(ida);
	std::string cutIdb = GetCutId(idb);
	if (cutIda == LYConstData::m_attachRest->_ID || cutIdb == LYConstData::m_attachRest->_ID
			|| cutIda == LYConstData::m_segmentPlace->_ID || cutIdb == LYConstData::m_segmentPlace->_ID
			|| cutIda == "arvNullPlace" || cutIdb == "arvNullPlace"
			|| cutIda == "deptNullPlace" || cutIdb == "deptNullPlace") {
		return TrafficData::_blank_traffic_item;
	}
	std::string oriKey = cutIda + "_" + cutIdb;
	std::tr1::unordered_map<std::string, TrafficItem*>::iterator it = m_TrafficMap.find(oriKey);
	if (it == m_TrafficMap.end()) {
		if (cutIda == cutIdb) {
			return TrafficData::_self_traffic_item;
		}
		if (1) {
			MJ::PrintInfo::PrintErr("[%s]BasePlan::GetTraffic, No Traffic Info\t%s", m_qParam.log.c_str(), oriKey.c_str());
		}
		return NULL;
	}
	return it->second;
}

const TrafficItem* BasePlan::GetTraffic(const std::string& ida, const std::string& idb, const std::string& date) {
	std::string cutIda = GetCutId(ida);
	std::string cutIdb = GetCutId(idb);
	if (date.empty()
			|| !LYConstData::IsRealID(cutIda)
			|| !LYConstData::IsRealID(cutIdb)) {
		return GetTraffic(cutIda, cutIdb);
	}
	if (!m_customTrafMap.empty()) {
		const TrafficItem* retTraf = GetCustomTraffic(cutIda + "_" + cutIdb);
		if (retTraf) return retTraf;
	}
	std::string trafKey = cutIda + "_" + cutIdb;
	if (m_lastTripTrafMap.find(trafKey) != m_lastTripTrafMap.end()) {
		const TrafficItem* lastTraf = m_lastTripTrafMap[trafKey];
		if (m_qParam.type == "s131") return lastTraf;
		if (m_date2trafPrefer.find(date) != m_date2trafPrefer.end()) {
			int trafPrefer = m_date2trafPrefer[date];
			//租车不出公交
			if (trafPrefer == TRAF_PREFER_TAXI and lastTraf->_mode != TRAF_MODE_BUS) {
				return lastTraf;
			//公交不出原驾车
			} else if (trafPrefer != TRAF_PREFER_TAXI and lastTraf->_mode != TRAF_MODE_DRIVING) {
				return lastTraf;
			}
		}
	}
	trafKey = cutIda + "_" + cutIdb + "_" + date;
	std::tr1::unordered_map<std::string, TrafficItem*>::iterator it = m_TrafficMap.find(trafKey);
	if (it != m_TrafficMap.end()) {
		return it->second;
	}
	return GetTraffic(cutIda, cutIdb);
}

const TrafficItem* BasePlan::GetCustomTraffic(const std::string& trafKey) {
	std::tr1::unordered_map<std::string, TrafficItem*>::iterator it = m_customTrafMap.find(trafKey);
	if (it != m_customTrafMap.end()) {
		return it->second;
	}
	else {
		//MJ::PrintInfo::PrintErr("[%s]BasePlan::GetCustomTraffic, No Traffic Info\t%s", m_qParam.log.c_str(), trafKey.c_str());
		return NULL;
	}
}

bool BasePlan::IsVarPlaceOpen(const VarPlace* varplace) {
	bool ret = true;
	for (int i = 0; i < m_RouteBlockList.size(); ++i) {
		const RouteBlock* route_block = m_RouteBlockList[i];
		for (int j = 0; j < route_block->_dates.size(); ++j) {
			std::vector<std::string> time_rule;
			ret = IsVarPlaceOpen(varplace, route_block->_dates[j], time_rule);
			if (!ret || time_rule.empty()) continue;
			return true;
		}
	}
	MJ::PrintInfo::PrintDbg("[%s]BasePlan::IsVarPlaceOpen, VarPlace not open: %s(%s), date: %s~%s, rawRule: %s", m_qParam.log.c_str(), varplace->_ID.c_str(), varplace->_name.c_str(), m_RouteBlockList.front()->_dates.front().c_str(), m_RouteBlockList.back()->_dates.back().c_str(), varplace->_time_rule.c_str());
	return false;
}

bool BasePlan::IsVarPlaceOpen(const VarPlace* varplace, const std::string& date, std::vector<std::string>& time_rule) {
	time_rule.clear();
	TimeIR::getTheDateOpenTimeRange(varplace->_time_rule, date, time_rule);
	if (time_rule.empty()) return false;
	return true;
}

int BasePlan::RemoveNotPlanPlace(std::vector<const LYPlace*>& placeList) {
	for (auto it = placeList.begin(); it != placeList.end();) {
		std::string id = (*it)->_ID;
		if (m_notPlanSet.find(id) != m_notPlanSet.end()) {
			std::cerr << "remove place " << (*it)->_ID << "  name " <<  (*it)->_name << std::endl;
			it = placeList.erase(it);
		} else {
			it ++;
		}
	}
	return 0;
}


bool BasePlan::IsRestaurant(const LYPlace* place) {
	if (!place) return false;
	if (place == LYConstData::m_attachRest) return true;
	if (place->_t & LY_PLACE_TYPE_RESTAURANT
			&& m_RestTypeMap.find(place->_ID) != m_RestTypeMap.end()) {
		return true;
	}
	return false;
}

int BasePlan::GetRestType(const LYPlace* place, time_t arrive, time_t depart) {
	int restType = RESTAURANT_TYPE_NULL;
	if (place == LYConstData::m_attachRest) {
		restType = RESTAURANT_TYPE_ALL;
	} else if (m_RestTypeMap.find(place->_ID) != m_RestTypeMap.end()) {
		restType = m_RestTypeMap[place->_ID];
	}
	if (restType == RESTAURANT_TYPE_NULL) return RESTAURANT_TYPE_NULL;
	if (restType == RESTAURANT_TYPE_LUNCH) return RESTAURANT_TYPE_LUNCH;
	if (restType == RESTAURANT_TYPE_SUPPER) return RESTAURANT_TYPE_SUPPER;

	time_t zeroTime = MJ::MyTime::toTime(MJ::MyTime::toString(arrive, m_TimeZone).substr(0, 8) + "_00:00", m_TimeZone);
	for (int i = m_RestaurantTimeList.size() - 1; i >= 0; --i) {
		RestaurantTime& restTime = m_RestaurantTimeList[i];
		if (restTime._type & restType && arrive >= zeroTime + restTime._begin) {
			return restTime._type;
		}
	}
	return RESTAURANT_TYPE_NULL;
}

//并无必要？？？
int BasePlan::DelPlace(const LYPlace* dPlace) {
	if (!IsDeletable(dPlace)) return 1;
	MJ::PrintInfo::PrintDbg("[%s]BasePlan::DelPlace, %s(%s)", m_qParam.log.c_str(), dPlace->_ID.c_str(), dPlace->_name.c_str());
	auto it = std::find(m_waitPlaceList.begin(), m_waitPlaceList.end(), dPlace);
//	if (it != m_waitPlaceList.end()) m_waitPlaceList.erase(it);
	return 0;
}

bool BasePlan::IsTimeOut() {
	if (m_CutTimer.cost() / 1000 > BagParam::m_timeOut) return true;
	return false;
}

bool BasePlan::IsDeletable(const LYPlace* dPlace) {
	// 非varPlace不可删
	if (dPlace == NULL || !(dPlace->_t & LY_PLACE_TYPE_VAR_PLACE)) return false;
	if (m_userMustPlaceSet.count(dPlace)) return false;
	return true;
}

// 每6小时一档
int BasePlan::AvailDurLevel() {
	return (m_availDur / 3600.0 / 6);
}

int BasePlan::GetMissLevel(const LYPlace* place) {
	if (m_missLevelMap.find(place) != m_missLevelMap.end()) {
		return m_missLevelMap[place];
	}
	return 0;
}

const LYPlace* BasePlan::GetLYPlace(const std::string& id) {
	if (m_multiPlaceMap.find(id) != m_multiPlaceMap.end()) {
		return m_multiPlaceMap[id];
	}

	if (m_customPlaceMap.find(id) != m_customPlaceMap.end()) {
		return m_customPlaceMap[id];
	}

	const LYPlace* place = LYConstData::GetLYPlace(id, m_qParam.ptid);
	return place;
}

const LYPlace* BasePlan::SetCustomPlace(const int& type, const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, const int& customMod) {
	LYPlace* place = NULL;
	if (m_customPlaceMap.find(id) != m_customPlaceMap.end()) {//已有此id对应的点 直接更新数据
		place = m_customPlaceMap[id];
		place->_name = name;
		place->_lname = lname;
		place->_enname = lname;
		place->_poi = coord;
		place->m_custom = customMod;
		return place;
	} else {//未找到此点 自制
		switch(type) {
			case LY_PLACE_TYPE_HOTEL: {
				place = LYConstData::MakeHotel(id, name, lname, coord, customMod);
				break;
			}
			case LY_PLACE_TYPE_VIEW: {
				place = LYConstData::MakeView(id, name, lname, coord, customMod);
				break;
			}
			case LY_PLACE_TYPE_SHOP: {
				place = LYConstData::MakeShop(id, name, lname, coord, customMod);
				break;
			}
			case LY_PLACE_TYPE_RESTAURANT: {
				place = LYConstData::MakeRestaurant(id, name, lname, coord, customMod);
				break;
			}
			case LY_PLACE_TYPE_AIRPORT:
			case LY_PLACE_TYPE_SAIL_STATION:
			case LY_PLACE_TYPE_BUS_STATION:
			case LY_PLACE_TYPE_STATION:{
				place = LYConstData::MakeStation(id,type, name, lname, coord, customMod);
				break;
			}
			case LY_PLACE_TYPE_CAR_STORE: {
				place = LYConstData::MakeCarStore(id,name,lname,coord,customMod);
				break;
			}
			case LY_PLACE_TYPE_ACTIVITY:
			case LY_PLACE_TYPE_PLAY:{
				place = LYConstData::MakeTour(id,name,lname,coord,type,customMod);
				break;
			default:
				place = LYConstData::MakeNullPlace(id,name,lname,coord,type,customMod);
				break;
			}
		}
		if (place) {
			m_customPlaceMap[id] = place;
		}
	}
	return place;
}

const int BasePlan::GetEntryTime(const LYPlace* place) {
	if (place->_t == LY_PLACE_TYPE_AIRPORT) {
		return m_EntryAirportTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_STATION) {
		return m_EntryStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_BUS_STATION) {
		return m_EntryBusStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_CAR_STORE || place->_t != LY_PLACE_TYPE_HOTEL) {
		return m_EntryCarStoreTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_HOTEL) {
		return m_EntryHotelTimeCost;
	}
}

const int BasePlan::GetExitTime(const LYPlace* place) {
	if (place->_t == LY_PLACE_TYPE_AIRPORT) {
		return m_ExitAirportTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_STATION) {
		return m_ExitStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_BUS_STATION) {
		return m_ExitBusStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_CAR_STORE || place->_t != LY_PLACE_TYPE_HOTEL) {
		return m_ExitCarStoreTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_HOTEL) {
		return m_ExitHotelTimeCost;
	}
}

const int  BasePlan::GetEntryZipTime(const LYPlace* place) {
	if (place->_t == LY_PLACE_TYPE_AIRPORT) {
		return m_EntryZipAirportTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_STATION) {
		return m_EntryZipStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_BUS_STATION) {
		return m_EntryZipBusStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_CAR_STORE || place->_t != LY_PLACE_TYPE_HOTEL) {
		return m_EntryZipCarStoreTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_HOTEL) {
		return m_EntryHotelTimeCost;
	}
}

const int  BasePlan::GetExitZipTime(const LYPlace* place) {
	if (place->_t == LY_PLACE_TYPE_AIRPORT) {
		return m_ExitZipAirportTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_STATION) {
		return m_ExitZipStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_BUS_STATION) {
		return m_ExitZipBusStationTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_CAR_STORE || place->_t != LY_PLACE_TYPE_HOTEL) {
		return m_ExitZipCarStoreTimeCost;
	} else if (place->_t == LY_PLACE_TYPE_HOTEL) {
		return m_ExitHotelTimeCost;
	}
}

//SMZ 景点多次游玩的过程
std::string BasePlan::Insert2MultiMap(const std::string& id) {
	if (id == "") {
		return "";
	}
	const LYPlace* place = GetLYPlace(id);
	if (place == NULL) {
		MJ::PrintInfo::PrintLog("[%s]BasePlan::Insert2MulitMaptiMap %s, GetPlaceFailed !", m_qParam.log.c_str(), id.c_str());
		return "";
	}//得到 新的id 后直接准备转换

	if (m_placeNumberMap.find(id) == m_placeNumberMap.end()) {
		m_placeNumberMap[id] = 0;
		return id;
	}

	LYPlace* newPlace = NULL;
	if (place->_t & LY_PLACE_TYPE_VIEW) {
		const View* view = dynamic_cast<const View*>(place);
		if (view == NULL) {
			return "";
		}
		View* newView = new View(*view);
		if (newView) {
			newPlace = newView;
		}
	} else if (place->_t & LY_PLACE_TYPE_SHOP) {
		const Shop* shop = dynamic_cast<const Shop*>(place);
		if (shop == NULL) {
			return "";
		}
		Shop* newShop = new Shop(*shop);
		if (newShop) {
			newPlace = newShop;
		}
	} else if (place->_t & LY_PLACE_TYPE_RESTAURANT) {
		const Restaurant* restaurant = dynamic_cast<const Restaurant*>(place);
		if (restaurant == NULL) {
			return "";
		}
		Restaurant* newRestaurant = new Restaurant(*restaurant);
		if (newRestaurant) {
			newPlace = newRestaurant;
		}
	} else if (place->_t == LY_PLACE_TYPE_VIEWTICKET) {
		const ViewTicket *viewTicket = dynamic_cast<const ViewTicket*>(place);
		//std::cerr << "in ViewTicket" << std::endl;
		if (viewTicket == NULL) {
			//std::cerr << "ViewTicket is NULL" << std::endl;
			return "";
		}
		ViewTicket *newViewTicket = new ViewTicket(*viewTicket);
		if (newViewTicket) {
			newPlace = newViewTicket;
		}
	} else if (place->_t & LY_PLACE_TYPE_TOURALL) {
		//std::cerr << "in TOUR" << std::endl;
		const Tour* tour = dynamic_cast<const Tour*>(place);
		if (tour == NULL) {
			//std::cerr << "Tour is NULL" << std::endl;
			return "";
		}
		Tour* newTour = new Tour(*tour);
		if (newTour) {
			newPlace = newTour;
		}
	}
	//车站会多次游玩
	else if (place->_t & LY_PLACE_TYPE_ARRIVE) {
		const Station* st = dynamic_cast<const Station*>(place);
		if (st == NULL) {
			return "";
		}
		Station* newSt = new Station(*st);
		if (newSt) {
			newPlace = newSt;
		}
	}
	else if (place->_t & LY_PLACE_TYPE_HOTEL) {
		const Hotel* hotel = dynamic_cast<const Hotel*>(place);
		if (hotel == NULL) {
			return "";
		}
		Hotel* newHotel = new Hotel(*hotel);
		if (newHotel) {
			newPlace = newHotel;
		}
	}
	else if(place->_t & LY_PLACE_TYPE_CAR_STORE){
		//租车和还车点不会多次游玩
		const CarStore* carStore = dynamic_cast<const CarStore*>(place);
		if (carStore == NULL) {
			return "";
		}
		CarStore* newCar = new CarStore(*carStore);
		if (newCar) {
			newPlace = newCar;
		}
	}
	//如果得到了转换后的 id 直接 进行里面逻辑 失败了就报错
	if (newPlace) {//先进行序号编号
		m_placeNumberMap[newPlace->_ID] ++;
		char buff[1000];
		snprintf(buff, sizeof(buff), "%s#%02d", newPlace->_ID.c_str(), m_placeNumberMap[newPlace->_ID]);
		newPlace->_ID = std::string(buff);
		if (m_multiPlaceMap.find(newPlace->_ID) != m_multiPlaceMap.end()) {
			MJ::PrintInfo::PrintLog("[%s] Warning %s source: %s , Make Multi Place Failed", m_qParam.log.c_str(), newPlace->_ID.c_str(), id.c_str());
			return "";
		}
		m_multiPlaceMap[newPlace->_ID] = newPlace;
//		newPlace->m_custom = POI_CUSTOM_MODE_MAKE;
		MJ::PrintInfo::PrintLog("[%s] %s source: %s , Insert successful!", m_qParam.log.c_str(), newPlace->_ID.c_str(), id.c_str());
		return newPlace->_ID;
	}
	return "";
}

std::string BasePlan::GetCutId(const std::string& id) {
	int pos = id.find("#");
	std::string ret = "";
	if (pos >= 0) {
		ret = id.substr(0, pos);
	} else {
		ret = id;
	}
	return ret;
}

std::string BasePlan::GetRealID(const std::string& id) {
	std::string cutId = GetCutId(id);
	return LYConstData::GetRealID(cutId);
}

int BasePlan::GetProdTicketsListByPlace(const LYPlace* place, std::vector<const TicketsFun*> &tickets, const std::string& date) {
	std::string id = place->_ID;
	if (m_pid2ticketIdAndNum.find(id) != m_pid2ticketIdAndNum.end()) {
		auto &list = m_pid2ticketIdAndNum[id];
		for (auto it = list.begin(); it != list.end(); ++it) {
			const TicketsFun *ticket = NULL;
			LYConstData::GetProdTicketsByPlaceAndId(place, it->first, ticket);
			if (ticket) {
				tickets.push_back(ticket);
			}
		}
	} else {
		const TicketsFun *ticket = NULL;
		const LYPlace* newPlace = GetLYPlace(GetCutId(place->_ID));
		LYConstData::IsTourHasTicketofTheDate(newPlace, date, ticket);
		if (ticket) {
			tickets.push_back(ticket);
		}
	}
	return 0;
}

const HInfo* BasePlan::GetHotelByDidx(int didx) {
	const HInfo* hotelInfo = NULL;
	std::vector<HInfo*> hotelInfoList;
	hotelInfoList.assign(m_RouteBlockList.front()->_hInfo_list.begin(), m_RouteBlockList.front()->_hInfo_list.end());
	for (int i = 0; i < hotelInfoList.size(); i++) {
		const HInfo* hInfo = hotelInfoList[i];
		if (didx >= hInfo->m_dayStart && didx < hInfo->m_dayEnd) {
			hotelInfo = hInfo;
		}
	}
	return hotelInfo;
}
//玩乐当前日期可用,并且票有报价
//仅在规划必去玩乐时调用,ReqParser
bool BasePlan::IsTourAvailable(const Tour* tour, const std::string& date) {
	const LYPlace* newPlace = GetLYPlace(GetCutId(tour->_ID));
	const Tour* newTour = dynamic_cast<const Tour*>(newPlace);
	if (newTour == NULL) return false;
	bool tourAvailable = LYConstData::IsTourAvailable(newTour, date);
	float price = 0;
	const TicketsFun* ticket = NULL;
	bool hasTicket = LYConstData::IsTourHasTicketofTheDate(newTour, date, ticket);
	return tourAvailable and hasTicket;
}

//A->B 有序
//B->A A->B 返回结果不同
//返回  tour离散点   -> poi  		的最短距离
//或者  poi 		 -> tour集合点	的最短距离
//或者  tour离散点   -> tour集合点	的最短距离
//transList 中为 1.离散点(若有) 2.集合点(若有 且不同于1)
double BasePlan::SelectTransPois (const LYPlace* placeA, const LYPlace* placeB, std::vector<const LYPlace*>& transList) {
	std::tr1::unordered_set<const LYPlace*> gatherPoiSet;
	std::tr1::unordered_set<const LYPlace*> dismissPoiSet;
	if (placeA->_t & LY_PLACE_TYPE_TOURALL) {
		const Tour* tourA = dynamic_cast<const Tour*>(placeA);
		if (tourA) {
			AddTourGatherOrLeftPoi(tourA, dismissPoiSet, true);
		}
	}
	if (dismissPoiSet.size() == 0) dismissPoiSet.insert(placeA);

	if (placeB->_t & LY_PLACE_TYPE_TOURALL) {
		const Tour* tourB = dynamic_cast<const Tour*>(placeB);
		if (tourB) {
			AddTourGatherOrLeftPoi(tourB, gatherPoiSet);
		}
	}
	if (gatherPoiSet.size() == 0) gatherPoiSet.insert(placeB);

	const LYPlace* selectDismiss = placeA;
	const LYPlace* selectGather = placeB;
	double minDist = LYConstData::earthRadius;
	for (auto dismiss:dismissPoiSet) {
		for (auto gather:gatherPoiSet) {
			double temp = LYConstData::CaluateSphereDist(gather, dismiss);
			if (temp < minDist) {
				minDist = temp;
				selectDismiss = dismiss;
				selectGather = gather;
			}
		}
	}
	if (selectDismiss != placeA) {
		transList.push_back(selectDismiss);
		if (debug) _INFO("from %s to %s: select tour jiesong poi: %s", placeA->_ID.c_str(), placeB->_ID.c_str(), selectDismiss->_ID.c_str());
	}
	if (selectGather != placeB and selectGather != selectDismiss) {
		transList.push_back(selectGather);
		if (debug) _INFO("from %s to %s: select tour gather poi: %s", placeA->_ID.c_str(), placeB->_ID.c_str(), selectGather->_ID.c_str());
	}
	return LYConstData::CaluateSphereDist(selectDismiss, selectGather);
}

//酒店接送的两个玩乐dist == 0 ?????
double BasePlan::GetDist (const LYPlace* placeA, const LYPlace* placeB) {
	std::vector<const LYPlace*> poiList;
	return SelectTransPois(placeA, placeB, poiList);
}

int BasePlan::AddTourJieSongPoi (const Tour* tour, std::tr1::unordered_set<std::string>& PoiSet) {
	if (tour->m_jiesongType) {
		//gather
		std::tr1::unordered_set<const LYPlace*> placeSet;
		AddTourGatherOrLeftPoi(tour, placeSet);
		//dismiss
		AddTourGatherOrLeftPoi(tour, placeSet, true);
		for (auto place: placeSet) {
			if (place and !PoiSet.count(place->_ID)) PoiSet.insert(place->_ID);
		}
	}
	return 0;
}

int BasePlan::AddTourJieSongHotel (std::tr1::unordered_set<const LYPlace*>& PoiSet) {
	if (!m_City) return 0;
	const LYPlace* place = LYConstData::GetCoreHotel(m_City->_ID);
	const HInfo* hInfo = GetHotelByDidx(0);
	if (hInfo) {
		place = hInfo->m_hotel;
	}

	if (place and !PoiSet.count(place)) {
		if (debug) _INFO("add tour jiesong hotel: %s", place->_ID.c_str());
		PoiSet.insert(place);
	}
	return 0;
}

int BasePlan::AddTourGatherOrLeftPoi (const Tour* tour, std::tr1::unordered_set<const LYPlace*>& PoiSet, bool isDismiss) {
	if (tour->m_jiesongType == 2) {
		AddTourJieSongHotel(PoiSet);
	} else if (tour->m_jiesongType == 1) {
		std::tr1::unordered_set<std::string> poiLocal = tour->m_gatherLocal;
		if (isDismiss) {
			poiLocal = tour->m_dismissLocal;
		}
		for (auto placeId: poiLocal) {
			auto place = GetLYPlace(placeId);
			if (place and !PoiSet.count(place)) {
				PoiSet.insert(place);
				if (debug) _INFO("Add tour jiesong poi: %s", place->_ID.c_str());
			}
		}
	}
}
