#include "TourSet.h"
#include "PostProcessor.h"

int TourCityInfo::DoPlan(const QueryParam& param, BasePlan* basePlan, Json::Value& req, Json::Value& resp) {
	int ret = 0;
	ret = ReqParser::DoParse(param, req, basePlan);
	if (ret == 0) {
		ret = GetReqInfo(param, req, basePlan);
	}
	if (ret == 0) {
		ret = InitTourItem(req, basePlan);
		ret = ChangeHotel(basePlan);
	}
	if (ret == 0) {
		ret = GetTraffic(basePlan);
	}
	if (ret == 0) {
		ret = ExpandView(basePlan);
	}
	if (ret == 0) {
		ret = MakeOutView(basePlan, req, resp);
	}
	if (!basePlan->m_tryErrorMap.empty()) {
		PostProcessor::AddErrors(basePlan, resp["data"]);
		ret = 1;
	}
	return ret;
}
int TourCityInfo::MakeOutView(BasePlan* basePlan, Json::Value& req, Json::Value& resp) {
	Json::Value view = Json::Value();
	Json::Value& jDay = view["day"];
	MakejDayList(basePlan, jDay);
	PostProcessor::AddDoWhat(jDay, basePlan);
	Json::Value& jSummary = view["summary"];
	MakejSummary(basePlan, req, jSummary, jDay);
	view["expire"] = 0;
	resp["data"]["view"] = view;
	resp["data"]["arv_poi"] = req["city"]["arv_poi"];
	resp["data"]["arvCustom"] = req["city"]["arvCustom"];
	resp["data"]["arv_time"] = req["city"]["arv_time"];
	resp["data"]["dept_poi"] = req["city"]["dept_poi"];
	resp["data"]["deptCustom"] = req["city"]["deptCustom"];
	resp["data"]["dept_time"] = req["city"]["dept_time"];
	resp["data"]["checkout"] = req["city"]["checkout"];
	resp["data"]["checkin"] = req["city"]["checkin"];
	resp["data"]["cid"] = req["city"]["cid"];
	resp["data"]["traffic_in"] = req["city"]["traffic_in"];
	return 0;
}
int TourCityInfo::MakejSummary (BasePlan* basePlan, Json::Value& req, Json::Value& jSummary, Json::Value& jDayList) {
	Json::Value jWarningList = Json::arrayValue;
	PostProcessor::AddTrafficWarning(basePlan, jDayList, jWarningList);
	PostProcessor::MakeJSummary(basePlan, req, jWarningList, jDayList, jSummary);
	return 0;
}
int TourCityInfo::MakejDayList (BasePlan* basePlan, Json::Value& jDayList) {
	jDayList = Json::arrayValue;
	Json::Value jDay = Json::Value();
	Json::Value jViewList = Json::arrayValue;
	std::string dayDate = m_arvDate;
	if (m_checkIn != "") dayDate = m_checkIn;
	for (int i = 0; i < m_ViewList.size(); i++) {
		TourViewItem& tourViewItem = m_ViewList[i];
		Json::Value jView = Json::Value();
		if(MakejView(basePlan, tourViewItem, jView)) continue;
		jViewList.append(jView);
		if (tourViewItem.m_funcType == NODE_FUNC_KEY_HOTEL_SLEEP || i == m_ViewList.size()-1) {
			Json::Value nextDayDeptHotel = jView;
			{
				if (tourViewItem.m_funcType == NODE_FUNC_KEY_HOTEL_SLEEP) {
					//休息酒店点不展示自由活动,没有交通
					jViewList[jViewList.size()-1]["idle"] = Json::Value();
					jViewList[jViewList.size()-1]["traffic"] = Json::Value();
				}

				//离开酒店点没有就餐
				nextDayDeptHotel["dining_nearby"] = 0;
			}
			//构造day
			{
				jDay["date"] = dayDate;
				jDay["view"] = jViewList;
				std::string stime = MJ::MyTime::toString(m_arvTime, basePlan->m_TimeZone);
				std::string etime = MJ::MyTime::toString(m_deptTime, basePlan->m_TimeZone);
				PostProcessor::SetDayPlayRange(jDay, stime, etime);
				jDayList.append(jDay);
				_INFO("add day date: %s", jDay["date"].asString().c_str());
			}

			//将酒店离开点放入viewList
			//每过个睡觉点，date +1
			{
				jDay = Json::Value();
				jViewList.resize(0);
				jViewList.append(nextDayDeptHotel);
				dayDate = MJ::MyTime::datePlusOf(dayDate, 1);
			}
		}
	}
	return 0;
}
int TourCityInfo::MakejView(BasePlan* basePlan, const TourViewItem& tourViewItem, Json::Value& jView) {
	jView = Json::Value();
	_INFO("makejView... id: %s, name %s", tourViewItem.m_id.c_str(), tourViewItem.m_name.c_str());
	jView["id"] = basePlan->GetCutId(tourViewItem.m_id);
	if (jView["id"] == "arvNullPlace" || jView["id"] == "deptNullPlace") return 1;
	jView["type"] = tourViewItem.m_type;
	jView["custom"] = tourViewItem.m_custom;
	jView["name"] = tourViewItem.m_name;
	jView["lname"] = tourViewItem.m_lname;
	jView["coord"] = tourViewItem.m_coord;
	jView["stime"] = tourViewItem.m_stime;
	jView["etime"] = tourViewItem.m_etime;
	jView["dur"] = tourViewItem.m_dur;
	jView["traffic"] = tourViewItem.m_traffic;
	jView["do_what"] = tourViewItem.m_doWhat;
	jView["play"] = basePlan->m_poiPlays[tourViewItem.m_id];
	jView["func_type"] = tourViewItem.m_funcType;
	if (!tourViewItem.m_product.isNull()) {
		jView["product"] = tourViewItem.m_product;
	}
	if (!tourViewItem.m_info.isNull()) {
		jView["info"] = tourViewItem.m_info;
	}
	if (!tourViewItem.m_idle.isNull()) {
		jView["idle"] = tourViewItem.m_idle;
	}
	if (tourViewItem.m_close != -1) {
		jView["close"] = tourViewItem.m_close;
	}
	if (tourViewItem.m_diningNearby!=-1) {
		jView["dining_nearby"] = tourViewItem.m_diningNearby;
	}
	return 0;
}
int TourCityInfo::InitTourItem(Json::Value& req, BasePlan* basePlan) {
	_INFO("InitTourItem...");
	if (isFirstCity || m_arvPoi == basePlan->m_arvNullPlace) {
		TourViewItem arvItem(basePlan, m_arvPoi, NODE_FUNC_KEY_ARRIVE, m_arvDate);
		m_ViewList.push_back(arvItem);
		_INFO("add arv %s, date: %s, stime: %s", arvItem.m_id.c_str(), m_arvDate.c_str(), MJ::MyTime::toString(m_arvTime, basePlan->m_TimeZone).c_str());
	}
	//原始tour信息
	Json::Value& jTourView = m_srcRoute["view"];
	Json::Value& jTourDaysList = jTourView["day"];

	std::string date = m_arvDate;
	if (m_checkIn != "") date = m_checkIn;
	for (int i = 0; i < jTourDaysList.size(); i++) {
		Json::Value& jTourDay = jTourDaysList[i];
		Json::Value& jTourDayViewList = jTourDay["view"];
		std::string srcDate = jTourDay["date"].asString();
		for (int j = 0; j < jTourDayViewList.size(); j++) {
			Json::Value& jTourDayView = jTourDayViewList[j];
			if (i != 0 && j == 0 && jTourDayView["type"] == LY_PLACE_TYPE_HOTEL) {
				m_ViewList.back().m_traffic = jTourDayView["traffic"];
				continue;
			}
			//当前点是酒店，酒店的交通等于第二天早上离开的酒店的交通
			bool isSleep = false;
			//当天最后一个点 非最后一天 
			if (j+1 == jTourDayViewList.size() and i+1 < jTourDaysList.size()
				and jTourDaysList[i+1].isMember("view") && jTourDaysList[i+1]["view"].isArray() && jTourDaysList[i+1]["view"].size()>0
				and jTourDayView["type"] == LY_PLACE_TYPE_HOTEL and jTourDaysList[i+1]["view"][0u].isMember("type") and jTourDaysList[i+1]["view"][0u]["type"] == LY_PLACE_TYPE_HOTEL)
			{
				isSleep = true;
			}

			//node_func_type 判断
			int nodeFunc = -1;
			if (jTourDayView.isMember("func_type") and jTourDayView["func_type"].isInt()) {
				nodeFunc = jTourDayView["func_type"].asInt();
			}
			if (isSleep) nodeFunc = NODE_FUNC_KEY_HOTEL_SLEEP;
			else if (jTourDayView["type"] == LY_PLACE_TYPE_HOTEL and i+1 == jTourDaysList.size()) nodeFunc = NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE;

			TourViewItem tourItem(m_srcTourSet, basePlan, jTourDayView, date, srcDate, m_includeWanle);
			if (nodeFunc != -1) tourItem.m_funcType = nodeFunc;

			//读取玩法play信息
			if(jTourDayView.isMember("play") and jTourDayView["play"].isString()) basePlan->m_poiPlays[tourItem.m_id] = jTourDayView["play"].asString();

			m_ViewList.push_back(tourItem);
			_INFO("add %s, date: %s, stime: %s, etime: %s, addInfo: %d", tourItem.m_id.c_str(), date.c_str(), tourItem.m_stime.c_str(), tourItem.m_etime.c_str(), tourItem.m_funcType);
		}
		date = MJ::MyTime::datePlusOf(date,1);
	}
	if (isLastCity || m_deptPoi == basePlan->m_deptNullPlace) {
		TourViewItem deptItem(basePlan, m_deptPoi, NODE_FUNC_KEY_DEPART, m_deptDate);
		m_ViewList.push_back(deptItem);
		_INFO("add dept %s, date: %s", deptItem.m_id.c_str(), m_deptDate.c_str());
	}
	return 0;
}
int TourCityInfo::ChangeHotel (BasePlan* basePlan) {
	if (m_hotelInfoList.size() < 1) return 0;
	std::tr1::unordered_map<int,std::pair<int, std::string>> m_hotelIndex2dIdxAndHotelId;
	int didx = 0;
	for (int i = 0; i < m_ViewList.size(); i++) {
		//包括行李点
		if (m_ViewList[i].m_type == LY_PLACE_TYPE_HOTEL) {
			//不包括arvPoi 和 deptPoi null
			_INFO("didx: %d, oriHotel: %s, index: %d",didx, m_ViewList[i].m_id.c_str(), i);
			if (m_ViewList[i].m_funcType == NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE) didx--; //取行李点 天数减一(前一天入住的酒店)
			m_hotelIndex2dIdxAndHotelId.insert(std::make_pair(i, std::make_pair(didx, m_ViewList[i].m_id)));
		}
		//睡觉点didx才+1
		if(m_ViewList[i].m_funcType == NODE_FUNC_KEY_HOTEL_SLEEP) {
			didx ++;
		}
	}
	didx = 0;
	for (auto it = m_hotelIndex2dIdxAndHotelId.begin(); it != m_hotelIndex2dIdxAndHotelId.end(); it++) {
		didx = it->second.first;
		const LYPlace* hotel = GetHotelbyIndex(didx);
		if (hotel == NULL) {
			std::string date = MJ::MyTime::datePlusOf(m_checkIn,didx);
			_INFO("err date:%s didx:%d, no hotel", date.c_str(), didx);

			// 未规划: 用corehotel表示
			hotel = LYConstData::GetCoreHotel(basePlan->m_City->_ID);
			if (!hotel) {
				MJ::PrintInfo::PrintErr("[%s]TourSet::ChangeHotel, no coreHotel for city %s(%s)", basePlan->m_qParam.log.c_str(), basePlan->m_City->_ID.c_str(), basePlan->m_City->_name.c_str());
				return 1;
			}
		}
		if (hotel && hotel->_ID != it->second.second) {
			std::string date = MJ::MyTime::datePlusOf(m_arvDate,didx);
			TourViewItem hotelItem(basePlan, hotel, NODE_FUNC_KEY_HOTEL, date);
			int index = it->first;
			TourViewItem OriHotel = m_ViewList[index];
			hotelItem.m_stime = OriHotel.m_stime;
			hotelItem.m_etime = OriHotel.m_etime;
			hotelItem.m_idle = OriHotel.m_idle;
			hotelItem.m_diningNearby = OriHotel.m_diningNearby;
			hotelItem.m_funcType = OriHotel.m_funcType;
			hotelItem.m_doWhat = OriHotel.m_doWhat;
			m_ViewList[index] = hotelItem;
			_INFO("change hotel:%s -> %s",it->second.second.c_str(), hotel->_ID.c_str());
			changedHotelIdx.insert(it->first);
		}
	}
	return 0;
}
int TourCityInfo::ExpandView(BasePlan* basePlan) {
	std::tr1::unordered_set<int > WarningedDays;
	for (int i = 0; i+1 < m_ViewList.size(); i++) {
		TourViewItem& tViewItem = m_ViewList[i];
		if (tViewItem.m_traffic.isMember("fixed") && tViewItem.m_traffic["fixed"].isInt() && tViewItem.m_traffic["fixed"].asInt())
		{
			continue;
		}
		std::string fromId = m_ViewList[i].m_id;
		if (!m_ViewList[i].m_product.isNull() && m_ViewList[i].m_product.isMember("pid") && m_ViewList[i].m_product["pid"].isString() && fromId != m_ViewList[i].m_product["pid"].asString()) {
			fromId = m_ViewList[i].m_product["pid"].asString();
		}
		std::string toId = m_ViewList[i+1].m_id;
		if (!m_ViewList[i+1].m_product.isNull() && m_ViewList[i+1].m_product.isMember("pid") && m_ViewList[i+1].m_product["pid"].isString() && toId != m_ViewList[i+1].m_product["pid"].asString()) {
			toId = m_ViewList[i+1].m_product["pid"].asString();
		}
		fromId = basePlan->GetCutId(fromId);
		toId = basePlan->GetCutId(toId);
		std::string date = m_ViewList[i+1].m_stime.substr(0,8);
		const TrafficItem* trafItem = basePlan->GetTraffic(fromId, toId, date);
		//主动规划时，拿不到交通，就使用原来的交通
		if (!trafItem) {
			_INFO("get traffic Item err, from %s, to %s, date %s", fromId.c_str(), toId.c_str(), date.c_str());
			continue;
			//return 1;
		}
		_INFO("get traffic Item success, from %s, to %s, date %s", fromId.c_str(), toId.c_str(), date.c_str());
		TourViewItem& nextItem = m_ViewList[i+1];
		Json::Value jTraffic = Json::Value();
		//修改交通时，非自定义交通不变
		if (basePlan->isChangeTraffic && trafItem->m_custom) {
			MakeTrafficDetail(basePlan, trafItem, jTraffic);
			tViewItem.m_traffic = jTraffic;
		}

		//规划时 到达离开点 酒店点 需要改交通
		if ( !basePlan->isChangeTraffic &&
				(tViewItem.m_type & LY_PLACE_TYPE_ARRIVE
				|| (tViewItem.m_type == LY_PLACE_TYPE_HOTEL && changedHotelIdx.find(i) != changedHotelIdx.end())
				|| (m_ViewList[i+1].m_type == LY_PLACE_TYPE_HOTEL && changedHotelIdx.find(i+1) != changedHotelIdx.end()))) {
			MakeTrafficDetail(basePlan, trafItem, jTraffic);
			tViewItem.m_traffic = jTraffic;
		}

		if (i>=1 && tViewItem.m_funcType == NODE_FUNC_KEY_HOTEL_SLEEP) {
			TourViewItem& lastItem = m_ViewList[i-1];
			time_t stime = MJ::MyTime::toTime(lastItem.m_etime, basePlan->m_TimeZone) + lastItem.m_traffic["dur"].asInt();
			time_t etime = MJ::MyTime::toTime(nextItem.m_stime, basePlan->m_TimeZone) - tViewItem.m_traffic["dur"].asInt();
			std::string stimeStr = MJ::MyTime::toString(stime, basePlan->m_TimeZone);
			std::string etimeStr = MJ::MyTime::toString(etime, basePlan->m_TimeZone);
			if (etime - stime < 0) {
				std::string day0Date = m_arvDate;
				if (m_checkIn != "") day0Date = m_checkIn;
				int didx = (MJ::MyTime::toTime(m_ViewList[i].m_stime.substr(0,8)+"_00:00",basePlan->m_TimeZone) - MJ::MyTime::toTime(day0Date+"_00:00",basePlan->m_TimeZone))/(24*3600);
				const HInfo* hInfo = basePlan->GetHotelByDidx(didx);
				int checkInDate = didx;
				if (hInfo) checkInDate = hInfo->m_dayStart;
				if (WarningedDays.find(checkInDate) != WarningedDays.end()) continue;
				if (basePlan->isChangeTraffic) {
					std::string warning = "无法保证晚上的合理休息时间，请缩短自定义交通时长";
					basePlan->m_tryErrorMap.insert(std::make_pair(checkInDate, std::make_pair(5, warning)));
				} else {
					std::string warning = "当日行程过紧，考虑市内交通后无法保证夜晚合理休息时间，需要减少游玩时长或地点";
					basePlan->m_tryErrorMap.insert(std::make_pair(checkInDate, std::make_pair(5, warning)));
				}
				WarningedDays.insert(checkInDate);
				_INFO("sleep time is less than 4h, date: %s, checkInDidx: %d", date.c_str(), checkInDate);
				continue;
			}
			//酒店点为团游第一个点时，酒店到达时间不变
			if ((isFirstCity && i == 1) || lastItem.m_type == LY_PLACE_TYPE_HOTEL) {
				if (tViewItem.m_stime < stimeStr) tViewItem.m_stime = stimeStr;
			} else {
				tViewItem.m_stime = stimeStr;
			}
			//酒店为团游最后一个点时，酒店离开时间不变
			if ((isLastCity && i+2 == m_ViewList.size()) || nextItem.m_type == LY_PLACE_TYPE_HOTEL) {
				if (tViewItem.m_etime > etimeStr) tViewItem.m_etime = etimeStr;
			} else {
				tViewItem.m_etime = etimeStr;
			}
			_INFO("change hotel time:stime: %s, etime: %s", tViewItem.m_stime.c_str(), tViewItem.m_etime.c_str());
		}
		//add idle
		if ((i == 0 && nextItem.m_type == LY_PLACE_TYPE_HOTEL)
				|| (tViewItem.m_type == LY_PLACE_TYPE_HOTEL && nextItem.m_type == LY_PLACE_TYPE_HOTEL)
				|| (i+2 == m_ViewList.size() && tViewItem.m_type == LY_PLACE_TYPE_HOTEL))
	   	{
			int idle = MJ::MyTime::toTime(nextItem.m_stime,basePlan->m_TimeZone) - MJ::MyTime::toTime(tViewItem.m_etime,basePlan->m_TimeZone) - jTraffic["dur"].asInt();
			if (idle >= 3 * 60 * 60) {
				tViewItem.m_idle["dur"] = idle / 900 * 900;
				tViewItem.m_idle["desc"] = "自由活动";
			} else {
				tViewItem.m_idle = Json::Value();
			}
		}
	}
	if (!basePlan->m_tryErrorMap.empty()) return 1;
	//到达离开点的就餐
	if (isFirstCity || isLastCity) {
		AttachArvDeptPoi(basePlan);
	}
	return 0;
}
int TourCityInfo::GetTraffic(BasePlan* basePlan) {
	std::tr1::unordered_set<std::string> trafKeySet;
	for (int i = 0; i < m_ViewList.size()-1; i++) {
		//需要获取交通
		//玩乐使用pid
		if((isFirstCity && m_ViewList[i].m_type & LY_PLACE_TYPE_ARRIVE)|| m_ViewList[i].m_type == LY_PLACE_TYPE_HOTEL || m_ViewList[i+1].m_type == LY_PLACE_TYPE_HOTEL || (isLastCity && m_ViewList[i+1].m_type & LY_PLACE_TYPE_ARRIVE)) {
			std::string fromId = m_ViewList[i].m_id;
			if (!m_ViewList[i].m_product.isNull() && m_ViewList[i].m_product.isMember("pid") && m_ViewList[i].m_product["pid"].isString() && fromId != m_ViewList[i].m_product["pid"].asString()) {
				fromId = m_ViewList[i].m_product["pid"].asString();
			}
			std::string toId = m_ViewList[i+1].m_id;
			if (!m_ViewList[i+1].m_product.isNull() && m_ViewList[i+1].m_product.isMember("pid") && m_ViewList[i+1].m_product["pid"].isString() && toId != m_ViewList[i+1].m_product["pid"].asString()) {
				toId = m_ViewList[i+1].m_product["pid"].asString();
			}
			fromId = basePlan->GetCutId(fromId);
			toId = basePlan->GetCutId(toId);
			std::string date = m_ViewList[i+1].m_stime.substr(0,8);
			std::string trafKey = fromId + "_" + toId + "_" + date;
			if (trafKeySet.find(trafKey) == trafKeySet.end()) {
				_INFO("trafKeySet add %s", trafKey.c_str());
				trafKeySet.insert(trafKey);
			}
		} else {
			m_ViewList[i].m_traffic["fixed"] = 1;
		}
	}
	if (!trafKeySet.empty()) {
		PathTraffic::GetTrafPair(basePlan, trafKeySet, basePlan->m_realTrafNeed & REAL_TRAF_ADVANCE);
	}
	return 0;
}
//解析请求
int TourCityInfo::GetReqInfo(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	//从basePlan中取城市基本信息
	GetCityInfoFromBasePlan(basePlan);
	//团游信息
	std::string tourId = req["city"]["tour"]["product_id"].asString();
	if (req["product"].isMember("tour") && req["product"]["tour"].isMember(tourId)) {
		m_srcTourSet = req["product"]["tour"][tourId];
	}
	if (m_srcTourSet.isMember("route") && m_srcTourSet["route"].isArray() && req["city"].isMember("ridx")) {
		int i = 0;
		for (i = 0; i < m_srcTourSet["route"].size(); i++) {
			if (m_srcTourSet["route"][i]["ridx"] == req["city"]["ridx"]) {
				m_srcRoute = m_srcTourSet["route"][i];
				break;
			}
		}
		if (i == 0) {
			isFirstCity = true;
		}
		if (i+1 == m_srcTourSet["route"].size()) {
			isLastCity = true;
		}
	}
	//首城市先平移一次
	if (isFirstCity) {
		std::string pstime = "";
		if (m_srcTourSet.isMember("pstime") and m_srcTourSet["pstime"].isString()) {
			pstime = m_srcTourSet["pstime"].asString();
			Json::Value jReq = Json::Value();
			jReq["route"] = m_srcTourSet["route"];
			jReq["pstime"] = pstime;
			jReq["product"] = m_srcTourSet["product"];
			jReq["needView"] = 1;
			QueryParam paramS131 = param;
			paramS131.type = "s131";
			Json::Value resp = Json::Value();
			Planner::DoPlanS131(paramS131, jReq, resp);
			if (resp.isMember("data") and resp["data"].isMember("view") and resp["data"]["view"].isMember("day") and resp["data"]["view"]["day"].isArray() and resp["data"]["view"]["day"].size()>0) {
				m_srcRoute["view"]["day"][0u] = resp["data"]["view"]["day"][0u];
			} else {
				return 1;
			}
			if (resp["data"].isMember("product") and resp["data"]["product"].isMember("wanle")) {
				m_srcTourSet["product"]["wanle"] = resp["data"]["product"]["wanle"];
			}
		}
	}
	if (m_srcTourSet.isMember("includeProduct") && m_srcTourSet["includeProduct"].isArray() && m_srcTourSet["includeProduct"].size()>0) {
		for (int i = 0; i < m_srcTourSet["includeProduct"].size(); i++) {
			if (m_srcTourSet["includeProduct"][i].asInt() == 128) {
				m_includeWanle = true;
				break;
			}
		}
	}
	return 0;
}
int TourCityInfo::GetCityInfoFromBasePlan(BasePlan* basePlan) {
	m_arvPoi = basePlan->m_RouteBlockList.front()->_arrive_place;
	m_arvTime = basePlan->m_ArriveTime;
	m_arvDate = MJ::MyTime::toString(basePlan->m_ArriveTime, basePlan->m_TimeZone).substr(0,8);
	m_deptPoi = basePlan->m_RouteBlockList.back()->_depart_place;
	m_deptTime = basePlan->m_DepartTime;
	m_deptDate = MJ::MyTime::toString(basePlan->m_DepartTime, basePlan->m_TimeZone).substr(0,8);
	m_checkIn = basePlan->m_RouteBlockList.front()->_checkIn;
	m_checkOut = basePlan->m_RouteBlockList.front()->_checkOut;
	m_hotelInfoList.assign(basePlan->m_RouteBlockList.front()->_hInfo_list.begin(), basePlan->m_RouteBlockList.front()->_hInfo_list.end());
	return 0;
}
//获取第i天入住的酒店
const LYPlace* TourCityInfo::GetHotelbyIndex(int didx) {
	if (didx < 0 || didx >= MJ::MyTime::compareDayStr(m_checkIn,m_checkOut)) return NULL;
	std::string checkInDate = MJ::MyTime::datePlusOf(m_checkIn,didx);
	for (int i = 0; i < m_hotelInfoList.size(); i++) {
		HInfo* hInfo = m_hotelInfoList[i];
		const LYPlace* hotel = hInfo->m_hotel;
		std::string hotelDayStart = hInfo->m_checkIn;
		std::string hotelDayEnd = hInfo->m_checkOut;
		if (checkInDate >= hotelDayStart && checkInDate < hotelDayEnd) {
			return hotel;
		}
	}
	return NULL;
}
int TourCityInfo::MakeTrafficDetail (BasePlan* basePlan, const TrafficItem* trafItem, Json::Value& jTrafItem) {
	jTrafItem["id"] = trafItem->_mid;
	jTrafItem["dur"] = trafItem->_time;
	bool isCharterCar = (basePlan->m_travalType == TRAVAL_MODE_CHARTER_CAR) ? true : false;
	jTrafItem["type"] = TrafficItem::InnerTrafMode2UserDefine(trafItem->_mode, isCharterCar, trafItem->m_custom);
	jTrafItem["dist"] = trafItem->m_realDist;
	jTrafItem["custom"] = trafItem->m_custom == true? 1 : 0;
	if (trafItem->_mid.find("Sphere") != std::string::npos) {
		jTrafItem["eval"] = 1;
	} else {
		jTrafItem["eval"] = 0;
	}
	jTrafItem["tips"] = "";
	if (trafItem->m_warnings.size() > 0)
		jTrafItem["tips"] = trafItem->m_warnings[0];
	jTrafItem["transfer"] = Json::arrayValue;
	for (int i = 0; i < trafItem->m_transfers.size(); i++) {
		jTrafItem["transfer"].append(trafItem->m_transfers[i]);
	}
	return 0;
}

int TourCityInfo::AttachArvDeptPoi(BasePlan* basePlan) {
	_INFO("add Attach...");
	if (m_ViewList.size() <= 0) return 0;
	TourViewItem& arvView = m_ViewList[0];
	TourViewItem& deptView = m_ViewList[m_ViewList.size()-1];
	int arvMayNeedAttach = 0;
	int deptMayNeedAttach = 0;
	RestaurantTime arvRest = basePlan->m_RestaurantTimeList[0], deptRest = basePlan->m_RestaurantTimeList[0];
	for (int i = 0; i < basePlan->m_RestaurantTimeList.size(); i ++) {
		const RestaurantTime& restTime = basePlan->m_RestaurantTimeList[i];
		time_t arvRestOpen = MJ::MyTime::toTime(m_arvDate + "_00:00", basePlan->m_TimeZone) + restTime._begin;
		time_t arvRestClose = MJ::MyTime::toTime(m_arvDate + "_00:00", basePlan->m_TimeZone) + restTime._end;
		time_t arvSTime = MJ::MyTime::toTime(arvView.m_stime, basePlan->m_TimeZone);
		time_t arvETime = MJ::MyTime::toTime(arvView.m_etime, basePlan->m_TimeZone);
		if (ToolFunc::HasUnionPeriod(arvRestOpen,arvRestClose,arvSTime,arvETime)) {
			arvMayNeedAttach = 1;
			arvRest = restTime;
		}
	}
	for (int i = 0; i < basePlan->m_RestaurantTimeList.size(); i ++) {
		const RestaurantTime& restTime = basePlan->m_RestaurantTimeList[i];
		time_t deptRestOpen = MJ::MyTime::toTime(m_deptDate + "_00:00", basePlan->m_TimeZone) + restTime._begin;
		time_t deptRestClose = MJ::MyTime::toTime(m_deptDate + "_00:00", basePlan->m_TimeZone) + restTime._end;
		time_t deptSTime = MJ::MyTime::toTime(deptView.m_stime, basePlan->m_TimeZone);
		time_t deptETime = MJ::MyTime::toTime(deptView.m_etime, basePlan->m_TimeZone);
		if (ToolFunc::HasUnionPeriod(deptRestOpen,deptRestClose,deptSTime,deptETime)) {
			deptMayNeedAttach = 1;
			deptRest = restTime;
		}
	}
	if (arvMayNeedAttach == 0 && deptMayNeedAttach == 0) return 0;
	//arv 是否需要richAttch
	{
		time_t arvRestOpen = MJ::MyTime::toTime(m_arvDate + "_00:00", basePlan->m_TimeZone) + arvRest._begin;
		time_t arvRestClose = MJ::MyTime::toTime(m_arvDate + "_00:00", basePlan->m_TimeZone) + arvRest._end;
		int hasRichAttched = 0;
		for (int i = 1; i < m_ViewList.size(); i++) {
			const TourViewItem& tourView = m_ViewList[i];
			time_t viewSTime = MJ::MyTime::toTime(tourView.m_stime, basePlan->m_TimeZone);
			time_t viewETime = MJ::MyTime::toTime(tourView.m_etime, basePlan->m_TimeZone);
			//view与就餐时间有重叠
			if (ToolFunc::HasUnionPeriod(arvRestOpen,arvRestClose,viewSTime,viewETime)) {
				if (tourView.m_diningNearby == 1) {
					hasRichAttched = 1;
					break;
				}
				continue;
			}
			break;
		}
		if (arvMayNeedAttach == 1 && hasRichAttched == 0) arvView.m_diningNearby = 1;
	}
	//dept richAttch
	{
		time_t deptRestOpen = MJ::MyTime::toTime(m_deptDate + "_00:00", basePlan->m_TimeZone) + deptRest._begin;
		time_t deptRestClose = MJ::MyTime::toTime(m_deptDate + "_00:00", basePlan->m_TimeZone) + deptRest._end;
		int hasRichAttched = 0;
		for (int i = m_ViewList.size()-1; i >=0; i--) {
			const TourViewItem& tourView = m_ViewList[i];
			time_t viewSTime = MJ::MyTime::toTime(tourView.m_stime, basePlan->m_TimeZone);
			time_t viewETime = MJ::MyTime::toTime(tourView.m_etime, basePlan->m_TimeZone);
			//view与就餐时间有重叠
			if (ToolFunc::HasUnionPeriod(deptRestOpen,deptRestClose,viewSTime,viewETime)) {
				if (tourView.m_diningNearby == 1) {
					hasRichAttched = 1;
					break;
				}
				continue;
			}
			break;
		}
		if (deptMayNeedAttach == 1 && hasRichAttched == 0) deptView.m_diningNearby = 1;
	}
	return 0;
}

