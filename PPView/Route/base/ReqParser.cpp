#include <math.h>
#include <limits>
#include <cstring>
#include <algorithm>
#include "PathTraffic.h"
#include "BasePlan.h"
#include "ReqChecker.h"
#include "ReqParser.h"
#include "DataChecker.h"
#include <sstream>
// 解析请求
int ReqParser::DoParse(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	bool isListpageReq = false;
	if (param.type == "p104" || param.type == "p105" || param.type == "b116" || param.type == "p201" || param.type == "p202"
			|| param.type == "p101") {
		isListpageReq = true;
	}
	int ret = 0;
	ret = ReqChecker::DoCheck(param, req, basePlan->m_error);
	if (ret) return ret;
	ret = ParseBasicParam(param, basePlan);
	if (ret) return ret;
	//仅规划请求需要parseCity
	if (!isListpageReq) {
		ret = ParseBasicReq(req, basePlan);
		if (ret) return ret;
		ret = ParseCity(param, req, basePlan);
		if (ret) return ret;
		ret = ParseCityPrefer(param, req, basePlan);
		if (ret) return ret;
		ret = ParseCustomPois(param, req, basePlan);
		if (ret) return ret;
		if (param.type != "s128" and param.type != "s205") {
			ret = ParseProduct(param, req, basePlan);
		}
		if (param.type == "ssv006_light" || param.type == "s205" || param.type == "s204" || param.type == "s202" || param.type == "s131" || param.type == "ssv007"
				|| param.type == "s125" || param.type == "s129" || param.type == "s128") {
			ret = ParseCustomTraf(param, req, basePlan);
		}
	}

	if (param.type == "p202"
			|| param.type == "p101") {
		ret = DoParseP202(param, req, basePlan);
	} else if (param.type == "ssv005_rich" || param.type == "ssv005_s130") {
		ret = DoParseSSV005(param, req, basePlan);
	} else if (param.type == "ssv006_light" || param.type == "s204" || param.type == "s202" || param.type == "s131"
			|| param.type == "s125" || param.type == "s129") {
		ret = DoParseSSV006(param, req, basePlan);
	} else if (param.type == "p104") {
		ret = DoParseP104(param, req, basePlan);
	} else if (param.type == "p105") {
		ret = DoParseP105(param, req, basePlan);
	} else if (param.type == "p201") {
		ret = DoParseP201(param, req, basePlan);
	} else if (param.type == "b116") {
		ret = DoParseB116(param, req, basePlan);
	}
	return ret;
}

// 解析跟param，param相关的变量赋值
int ReqParser::ParseBasicParam(const QueryParam& param, BasePlan* basePlan) {
	basePlan->m_qParam = param;

	if (param.dev == 1) {
		basePlan->m_QuerySource = QUERY_SOURCE_IOS;
	} else if (param.dev == 2) {
		basePlan->m_QuerySource = QUERY_SOURCE_ANDORID;
	} else {
		basePlan->m_QuerySource = QUERY_SOURCE_WEB;
	}

	basePlan->m_realTrafNeed = REAL_TRAF_NULL;
	basePlan->m_richPlace = false;
	basePlan->m_notPlanCityPark = false;
	basePlan->m_faultTolerant = false;
	basePlan->m_planAfterZip = false;
	basePlan->m_dayOneLast3hNotPlan = false;

	if (basePlan->m_qParam.type == "ssv005_rich" || basePlan->m_qParam.type == "ssv005_s130") {
		basePlan->m_realTrafNeed = REAL_TRAF_REPLACE;
		if (basePlan->m_qParam.type == "ssv005_rich") {
			basePlan->m_richPlace = true;
			basePlan->m_notPlanCityPark = true;
		}
		basePlan->m_faultTolerant = false;
		basePlan->m_planAfterZip = false;
		basePlan->m_dayOneLast3hNotPlan = true;//day1 和 day last 的3h可用游玩时间不够时 不规划
	} else if (basePlan->m_qParam.type == "s202" || basePlan->m_qParam.type == "s204" || basePlan->m_qParam.type == "ssv006_light" || basePlan->m_qParam.type == "ssv007"
			|| basePlan->m_qParam.type == "s125" || basePlan->m_qParam.type == "s129") {
		basePlan->m_realTrafNeed = REAL_TRAF_ADVANCE;
		basePlan->m_faultTolerant = true;
		basePlan->m_planAfterZip = true;
		basePlan->m_dayOneLast3hNotPlan = false;
	}
	return 0;
}

int ReqParser::ParseBasicReq(Json::Value& req, BasePlan* basePlan) {
	//修改交通操作
	if (req.isMember("action") && req["action"].isInt() && req["action"].asInt()) {
		basePlan->isChangeTraffic = true;
	}
	//内部操作 1.delPoi是否可删点
	if (req.isMember("inner") && req["inner"].isObject()) {
		if (req["inner"].isMember("delPoi") && req["inner"]["delPoi"].isInt() and req["inner"]["delPoi"].asInt()) {
			basePlan->clashDelPoi = true;
		}
	}
	//s125 extra
	if (req.isMember("extra") && req["extra"].isObject()) {
		//realTraffic为0使用静态库交通
		if (req["extra"].isMember("realTraffic") && req["extra"]["realTraffic"].isInt() && req["extra"]["realTraffic"].asInt() == 1) {
			if (req["extra"]["realTraffic"].asInt() == 1) {
				basePlan->m_realTrafNeed = REAL_TRAF_ADVANCE;
			} else {
				basePlan->m_realTrafNeed = REAL_TRAF_NULL;
			}
		}
		//控制是否放行李
		if (req["extra"].isMember("isImmediately") && req["extra"]["isImmediately"].isInt() and req["extra"]["isImmediately"].asInt()) {
			basePlan->m_isImmediately = true;
		}
	}
	//控制是否规划的日期
	if (req.isMember("notPlanDays") && req["notPlanDays"].isArray() && req["notPlanDays"].size()>0) {
		for (int i = 0; i < req["notPlanDays"].size(); i++) {
			std::string date = req["notPlanDays"][i].asString();
			basePlan->m_notPlanDateSet.insert(date);
			_INFO("add date: %s not plan", date.c_str());
		}
	}
	if (!req.isMember("list") || !req["list"].isArray() || !req["list"].size() > 0) {
		return 0;
	}
	if (req["list"][0U].isMember("adults")) {
		basePlan->m_AdultCount = req["list"][0U]["adults"].asInt();
	}
	if (req["list"][0U].isMember("useCBP")) {
		basePlan->m_useCBP = req["list"][0U]["useCBP"].asInt();
	}
	if (req["list"][0U].isMember("useRealTraf")) {
		basePlan->m_useRealTraf = req["list"][0U]["useRealTraf"].asInt();
	}
	if (req["list"][0U].isMember("useTrafShow")) {
		basePlan->m_useTrafShow = req["list"][0U]["useTrafShow"].asInt();
	}
	if (req["list"][0U].isMember("useKpi")) {
		basePlan->m_useKpi = req["list"][0U]["useKpi"].asInt();
	}
	if (req["list"][0U].isMember("useStaticTraf")) {
		basePlan->m_useStaticTraf = req["list"][0U]["useStaticTraf"].asInt();
	}
	if (req["list"][0U].isMember("no_need_urban") && req["list"][0U]["no_need_urban"].isInt()) {
		basePlan->m_isPlan = !(req["list"][0U]["no_need_urban"].asInt());
	}

	return 0;
}

int ReqParser::ParseCid(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	char buff[1000];

	if (req.isMember("cid") && req["cid"].isString() && req["cid"] != "") {
		// 1 根据city_ID获取城市信息
		std::string cityID = req["cid"].asString();
		basePlan->m_qParam.Log(cityID);
		basePlan->m_City = dynamic_cast<const City*>(GetLYPlace(basePlan, cityID, LY_PLACE_TYPE_CITY, param, basePlan->m_error));
		if (basePlan->m_City == NULL) {
			MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCid, city not found %s", basePlan->m_qParam.log.c_str(), cityID.c_str());
			return 1;
		}
		basePlan->m_TimeZone = basePlan->m_City->_time_zone;
		basePlan->m_listCity.push_back(basePlan->m_City->_ID);
	}
	if (req.isMember("cids") && req["cids"].isArray() && req["cids"].size()!=0) {
		for (int i = 0; i < req["cids"].size(); ++i) {
			if (req["cids"][i].isString()) {
				basePlan->m_listCity.push_back(req["cids"][i].asString());
			}
		}
	}
	if(basePlan->m_City == NULL) basePlan->m_City = dynamic_cast<const City*>(GetLYPlace(basePlan, "10001", LY_PLACE_TYPE_CITY, param, basePlan->m_error));

	return 0;
}

int ReqParser::ParseCity(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	if (!req.isMember("list") && !req.isMember("city")) return ret;
	char buff[1000];

	// 1 根据city_ID获取城市信息
	bool multiCity = false;
	if (req.isMember("list") && req["list"].isArray()) {
		multiCity = true;
	}
	// 2 构造RouteBlock
	Json::Value jCityList;
	if (multiCity) { //单城市 多城市 逻辑分割
		for (int i = 0 ; i < req["list"].size(); i ++) {
			jCityList.append(req["list"][i]["city"]);
		}
	}
	else
	{
		jCityList.append(req["city"]);
	}
	std::string cityID = jCityList[0U]["cid"].asString();
	basePlan->m_qParam.Log(cityID);
	basePlan->m_City = dynamic_cast<const City*>(GetLYPlace(basePlan, cityID, LY_PLACE_TYPE_CITY, param, basePlan->m_error));
	if (basePlan->m_City == NULL) return 1;
	basePlan->m_TimeZone = basePlan->m_City->_time_zone;

	for (int i = 0; i < jCityList.size(); i++) {
		if (IsBaoChe(basePlan, jCityList[i])) {// 是否选择包车
			basePlan->m_trafPrefer = TRAF_PREFER_TAXI;
			basePlan->m_travalType = TRAVAL_MODE_CHARTER_CAR;
			basePlan->m_LeaveLuggageTimeCost = 0;//寄存 ， 取行李时间为0
			basePlan->m_ReclaimLuggageTimeCost = 0;
			MJ::PrintInfo::PrintLog("[%s]city, baoche", param.log.c_str());
		}
	}
	if (jCityList[0U].isMember("use17Limit") && jCityList[0U]["use17Limit"].asInt() == 1) {
		basePlan->m_useDay17Limit = true;
	}
	if (jCityList[0U].isMember("ppMode") && jCityList[0U]["ppMode"].asInt() != 0) {
		basePlan->m_isPlan = false;
	}

	for (int i = 0; i < jCityList.size(); ++i) {
		Json::Value& jCity = jCityList[i];

		// 开始构造
		RouteBlock* route_block = new RouteBlock;
		basePlan->m_RouteBlockList.push_back(route_block);
		if (basePlan->m_travalType == TRAVAL_MODE_CHARTER_CAR || basePlan->m_isImmediately) {
			route_block->_delete_luggage_feasible = true;
		}

		route_block->_arrive_time = MJ::MyTime::toTime(jCity["arv_time"].asString(), basePlan->m_TimeZone);
		route_block->_depart_time = MJ::MyTime::toTime(jCity["dept_tTime"].asString(), basePlan->m_TimeZone);
		if (route_block->_arrive_time > route_block->_depart_time) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCity, arrive > depart", basePlan->m_qParam.log.c_str());
			basePlan->m_error.Set(53110,  "arrive_time > depart_time");
			return 1;
		}
		Json::FastWriter jw;
		if (jCity.isMember("checkin") && jCity.isMember("checkout")) {
			route_block->_checkIn = jCity["checkin"].asString();
			route_block->_checkOut = jCity["checkout"].asString();
		} else {
			basePlan->m_error.Set(53110,  "no checkin or no checkout");
			return 1;
		}
		// hotel部分
		if (!jCity.isMember("hotel") || jCity["hotel"].isNull() || (jCity["hotel"].isArray() and jCity["hotel"].size()<=0 )) {
			// 未规划: 用corehotel表示
			const LYPlace* hotel = LYConstData::GetCoreHotel(cityID);
			if (!hotel) {
				MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCity, no coreHotel for city %s(%s)", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
				return 1;
			}
			route_block->_hotel_list.push_back(hotel);
			//hInfo
			HInfo* hInfo = new HInfo();
			hInfo->m_checkIn = route_block->_checkIn;
			hInfo->m_checkOut = route_block->_checkOut;
			hInfo->m_hotel = route_block->_hotel_list[0];
			hInfo->m_isCoreHotel = true;
			route_block->_hInfo_list.push_back(hInfo);
		} else {
			// 有指定酒店
			Json::Value& jHotelList = jCity["hotel"];//jHotelList 只存储了product_id
			for (int i = 0; i < jHotelList.size(); i++) {
				if(not jHotelList[i]["product_id"].isString()) continue;
				std::string hotel_id = jHotelList[i]["product_id"].asString();
				if (req["product"]["hotel"].isMember(hotel_id) and req["product"]["hotel"][hotel_id].isObject()) {
					Json::Value jHotel = req["product"]["hotel"][hotel_id];
					std::string hid = jHotel["id"].asString();
					ret = ParseCustomHotel(param, jHotel, basePlan, hid);
					if (ret) {
						return 1;
					}
					const LYPlace* hotel = GetLYPlace(basePlan, hid, LY_PLACE_TYPE_HOTEL, param, basePlan->m_error);
					if (!hotel) {
						MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCity, Hotel null for city %s(%s)", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
						return 1;
					}
					route_block->_hotel_list.push_back(hotel);
					//hInfo
					HInfo* hInfo = new HInfo();
					hInfo->m_checkIn = jHotel["checkin"].isNull() ? jCity["arv_time"].asString().substr(0, 8) : jHotel["checkin"].asString().substr(0, 8);
					hInfo->m_checkOut = jHotel["checkout"].isNull() ? jCity["dept_time"].asString().substr(0, 8) : jHotel["checkout"].asString().substr(0, 8);
					hInfo->m_hotel = hotel;
					hInfo->m_userCommand = true;
					route_block->_hInfo_list.push_back(hInfo);
				}
			}
		}
		//补全酒店
		ret = CompleteHotel(param, req, basePlan, i);
		if (ret) return 1;
		std::sort(route_block->_hInfo_list.begin(), route_block->_hInfo_list.end(), hInfoCmp());
		route_block->_hotel_list.clear();
		for (int i = 0; i < route_block->_hInfo_list.size(); ++i) {
			route_block->_hotel_list.push_back(route_block->_hInfo_list[i]->m_hotel);
		}
		if (route_block->_hotel_list.empty()) {
			std::cerr<<"hyhy should not happen, _hotel_list empty, check hotels checkIn and checkOut\n";
			return 1;
		}
		// arrive_place 与dept_place
		if (!jCity["arv_poi"].isNull() && jCity.isMember("arvCustom") && !jCity["arvCustom"].isNull()
				&& jCity["arvCustom"].isMember("custom") && jCity["arvCustom"]["custom"].isInt()
				&& jCity["arvCustom"]["custom"].asInt()) {//自定义 到达离开地点
			std::string id = jCity["arv_poi"].asString();
			std::string coord = LYConstData::GetCoreHotelCoord(cityID);
			std::string name = jCity["arvCustom"]["name"].asString();
			std::string lname = jCity["arvCustom"]["lname"].asString();
			//add by shyy
			std::string mode = jCity["arvCustom"]["mode"].asString();
			int arvType = LY_PLACE_TYPE_STATION;
			//自定义到达交通
			if (mode == "flight") {
				arvType = LY_PLACE_TYPE_AIRPORT;
			}
			else if (mode == "ship") {
				arvType = LY_PLACE_TYPE_SAIL_STATION;
			}
			else if (mode == "bus") {
				arvType = LY_PLACE_TYPE_BUS_STATION;
			}
			//shyy end
			MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCity, Make custom arvPoi id %s(%s), coord (%s)", basePlan->m_qParam.log.c_str(), id.c_str(), name.c_str(), coord.c_str());
			const LYPlace* place = basePlan->SetCustomPlace(arvType, id, name, lname, coord, POI_CUSTOM_MODE_CUSTOM);
			if (!place) {
				MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCity, Make custom arvPoi id %s failed", basePlan->m_qParam.log.c_str(), id.c_str());
			}
		}
		if (!jCity["dept_poi"].isNull() && jCity.isMember("deptCustom") && !jCity["deptCustom"].isNull() && jCity["deptCustom"].isMember("custom") && jCity["deptCustom"]["custom"].isInt() && jCity["deptCustom"]["custom"].asInt()) {//自定义 到达离开地点
			std::string id = jCity["dept_poi"].asString();
			std::string coord = LYConstData::GetCoreHotelCoord(cityID);
			std::string name = jCity["deptCustom"]["name"].asString();
			std::string lname = jCity["deptCustom"]["lname"].asString();
			//add by shyy
			std::string mode = jCity["deptCustom"]["mode"].asString();
			int deptType = LY_PLACE_TYPE_STATION;
			//自定义离开交通
			if (mode == "flight") {
				deptType = LY_PLACE_TYPE_AIRPORT;
			}
			else if (mode == "ship") {
				deptType = LY_PLACE_TYPE_SAIL_STATION;
			}
			else if (mode == "bus") {
				deptType = LY_PLACE_TYPE_BUS_STATION;
			}
			//shyy end
			MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCity, Make custom deptPoi id %s(%s), coord (%s)", basePlan->m_qParam.log.c_str(), id.c_str(), name.c_str(), coord.c_str());
			const LYPlace* place = basePlan->SetCustomPlace(deptType, id, name, lname, coord, POI_CUSTOM_MODE_CUSTOM);
			if (!place) {
				MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCity, Make custom deptPoi id %s failed", basePlan->m_qParam.log.c_str(), id.c_str());
			}
		}
		route_block->_arrive_place = GetLYPlace(basePlan, jCity["arv_poi"].asString(), LY_PLACE_TYPE_ALL, param, basePlan->m_error);
		//如果到达和离开点不存在,用酒店代替
		if (route_block->_arrive_place == NULL) {
			basePlan->m_arvNullPlace = basePlan->SetCustomPlace(LY_PLACE_TYPE_NULL, "arvNullPlace", "空到达站点", "", basePlan->m_City->_poi, POI_CUSTOM_MODE_CUSTOM);
			route_block->_arrive_place = basePlan->m_arvNullPlace;
		}
		route_block->_depart_place = GetLYPlace(basePlan, jCity["dept_poi"].asString(), LY_PLACE_TYPE_ALL, param, basePlan->m_error);
		if (route_block->_depart_place == NULL) {
			basePlan->m_deptNullPlace = basePlan->SetCustomPlace(LY_PLACE_TYPE_NULL, "deptNullPlace", "空离开站点", "", basePlan->m_City->_poi, POI_CUSTOM_MODE_CUSTOM);
			route_block->_depart_place = basePlan->m_deptNullPlace;
		}

		//此处获取进出站时间,在构造keyNode设置
		//下边读到用户配置时间是会做修改
		route_block->_arrive_dur = basePlan->GetEntryTime(route_block->_arrive_place);
		route_block->_depart_dur = basePlan->GetExitTime(route_block->_depart_place);
	}

	// 3 检验m_RouteBlockList构造成功
	if (basePlan->m_RouteBlockList.size() <= 0) {
		MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCity, build route_block failed", basePlan->m_qParam.log.c_str());
		snprintf(buff, sizeof(buff), "构造route_block失败");
		basePlan->m_error.Set(55101, std::string(buff));
		return 1;
	}
	basePlan->m_ArriveTime = basePlan->m_RouteBlockList.front()->_arrive_time;
	basePlan->m_DepartTime = basePlan->m_RouteBlockList.back()->_depart_time;

	// 4 完善route_block信息
	for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
		RouteBlock* route_block = basePlan->m_RouteBlockList[i];
		std::string firstDate = MJ::MyTime::toString(route_block->_arrive_time, basePlan->m_TimeZone).substr(0,8);
		if (route_block->_checkIn != "") firstDate = route_block->_checkIn;
		snprintf(buff, sizeof(buff), "%s_00:00", firstDate.c_str());
		std::string endDate = MJ::MyTime::toString(route_block->_depart_time, basePlan->m_TimeZone).substr(0,8);
		//不过夜只有一个日期
		if (route_block->_checkOut !="") {
			endDate = route_block->_checkOut;
		} else {
			endDate = firstDate;
		}
		time_t day0_time = MJ::MyTime::toTime(buff, basePlan->m_TimeZone);
		while (firstDate <= endDate) { //凌晨00:00:00 开启新的一天
			route_block->_day0_times.push_back(day0_time);
			route_block->_dates.push_back(firstDate);
			day0_time += 24 * 3600;
			route_block->_day_limit++;
			firstDate = MJ::MyTime::datePlusOf(firstDate, 1);
		}

		route_block->_need_hotel = false;
		if(route_block->_checkOut>route_block->_checkIn) route_block->_need_hotel = true;

		if (!route_block->_need_hotel) {
			route_block->_hotel_list.clear();
			for (int i = 0; i < route_block->_hInfo_list.size(); ++i) {
				const HInfo* hInfo = route_block->_hInfo_list[i];
				if (hInfo) {
					delete hInfo;
					hInfo = NULL;
				}
			}
			route_block->_hInfo_list.clear();
		}
		// 确定每个酒店的天数范围
		if (route_block->_need_hotel) {
			snprintf(buff, sizeof(buff), "%d_00:00", MJ::MyTime::get(route_block->_arrive_time, "YMD", basePlan->m_TimeZone));
			if (route_block->_checkIn != "") {
				snprintf(buff, sizeof(buff), "%s_00:00", route_block->_checkIn.c_str());
			}
			time_t arv_day0_time = MJ::MyTime::toTime(buff, basePlan->m_TimeZone);
			for (int j = 0; j < route_block->_hInfo_list.size(); ++j) {
				HInfo* hInfo = route_block->_hInfo_list[j];

				snprintf(buff, sizeof(buff), "%s_00:00", hInfo->m_checkIn.c_str());
				time_t start_day0_time = MJ::MyTime::toTime(buff, basePlan->m_TimeZone);
				//凌晨到时,checkin即是昨天
				int hotelDayStart = std::max(0, static_cast<int>((start_day0_time - arv_day0_time) / (24 * 3600))) ;
				hInfo->m_dayStart = hotelDayStart;

				snprintf(buff, sizeof(buff), "%s_00:00", hInfo->m_checkOut.c_str());
				time_t end_day0_time = MJ::MyTime::toTime(buff, basePlan->m_TimeZone);
				int hotelDayEnd = std::max(0, static_cast<int>((end_day0_time - arv_day0_time) / (24 * 3600)));
				hInfo->m_dayEnd = hotelDayEnd;
				if (hInfo->m_dayEnd > route_block->_day_limit - 1) hInfo->m_dayEnd = route_block->_day_limit - 1;
			}
		}
	}

	return 0;
}

int ReqParser::ParseCityPrefer(const QueryParam & param,Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	if (!req.isMember("cityPreferCommon") || req["cityPreferCommon"].isNull()) return ret;
	Json::Value& jCityPrefer = req["cityPreferCommon"];
	if(jCityPrefer.isMember("keepTime") and jCityPrefer["keepTime"].isInt() and jCityPrefer["keepTime"].asInt() == 0)
	{
		basePlan->m_keepTime = false;
	}
	//游玩强度
	if (jCityPrefer.isMember("intensity")) {
		int userIntensity = jCityPrefer["intensity"].asInt();
		if (userIntensity == 0) {
			basePlan->m_scalePrefer = 1;
		} else if (userIntensity == 1) {
			basePlan->m_scalePrefer = 0.7;
		} else if (userIntensity == 2) {
			basePlan->m_scalePrefer = 1.5;
		}
	}
	// 时间范围
	if (jCityPrefer.isMember("timeRange")) {
		if (jCityPrefer["timeRange"].isMember("from") && jCityPrefer["timeRange"]["from"].isString()) {
			std::string fromTime = jCityPrefer["timeRange"]["from"].asString();
			if (fromTime == "24:00") {
				basePlan->m_HotelCloseTime = 24*3600;
			} else {
				basePlan->m_HotelCloseTime = MJ::MyTime::getOffsetHM(fromTime);
			}
		}
		if (jCityPrefer["timeRange"].isMember("to")) {
			std::string toTime = jCityPrefer["timeRange"]["to"].asString();
			if (toTime == "24:00") {
				basePlan->m_HotelOpenTime = 24*3600;
			} else {
				basePlan->m_HotelOpenTime = MJ::MyTime::getOffsetHM(toTime);
			}
		}
	}
	if(jCityPrefer.isMember("keepPlayTimeRange") and jCityPrefer["keepPlayTimeRange"].isInt() and jCityPrefer["keepPlayTimeRange"].asInt() == 0) {
		basePlan->m_keepPlayRange = false;
		//智能优化放开时间范围限制时
		basePlan->m_HotelCloseTime = 7*3600;
		basePlan->m_HotelOpenTime = 23*3600;
	}
	if (param.type=="ssv005_rich" and jCityPrefer.isMember("mustGo") && jCityPrefer["mustGo"].isArray() && jCityPrefer["mustGo"].size() > 0) {
		std::tr1::unordered_set<const LYPlace*> addSet;
		for (auto place: basePlan->m_userMustPlaceSet) {
			addSet.insert(place);
		}
		for (int i = 0; i < jCityPrefer["mustGo"].size(); ++i) {
			Json::Value & oneCityMustGo  = jCityPrefer["mustGo"][i];
			if (not oneCityMustGo.isObject()) continue;
			Json::Value & oneCityMustGoPois  = oneCityMustGo["pois"];
			if (not( oneCityMustGoPois.isArray() and oneCityMustGoPois.size()>0)) continue;
			for (int j = 0; j < oneCityMustGoPois.size(); ++j) {
				if ( not (oneCityMustGoPois[j].isObject() and oneCityMustGoPois[j].isMember("id") and oneCityMustGoPois[j]["id"].isString())) continue;
				std::string id = oneCityMustGoPois[j]["id"].asString();
				const LYPlace* place = basePlan->GetLYPlace(id);
				const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
				if (vPlace == NULL) {
					MJ::PrintInfo::PrintErr("ReqParser::ParsePrefer, can`t get varplace %s", id.c_str());
					basePlan->m_invalidPois.push_back(id);
					continue;
				}
				if (basePlan->m_City == NULL) {
					return 1;
				}
				int flag = 0;
				for (int j = 0; j < vPlace->_cid_list.size(); ++j) {
					if (basePlan->m_City->_ID ==  vPlace->_cid_list[j]) {
						flag = 1;
						break;
					}
				}
				if (flag == 0) {
					continue;
				}
				if (addSet.find(vPlace) == addSet.end()) {
					basePlan->m_userMustPlaceSet.insert(vPlace);
					addSet.insert(vPlace);
				}
				const Json::Value & jPoi = oneCityMustGoPois[j];

				if (vPlace->_t & LY_PLACE_TYPE_VIEWSHOP) {
					if(jPoi["date"].isString())//内部使用,产品接口并无此字段
					{
						std::string date=jPoi["date"].asString();
						if(date>"20160101" and date<"20500101") basePlan->m_PlaceDateMap[vPlace->_ID].insert(date);
					}
					int dur = 0;
					if (vPlace && vPlace->_rcmd_intensity) dur = vPlace->_rcmd_intensity->_dur;
					if (jPoi.isMember("dur") && jPoi["dur"].isInt()) {
						int rDur = jPoi["dur"].asInt();
						int tDur = rDur;
						if (rDur == -1) {
							tDur = 4*3600;
						}
						else if (rDur == -2) {
							tDur = 8*3600;
						}
						else if (rDur == -3) {
							tDur = 2*3600;
						}
						if ( tDur > 0 )
						{
							dur = tDur;
							basePlan->m_UserDurMap[place->_ID] = dur;
						}
					}
					if (jPoi.isMember("stime") && jPoi["stime"].isString() && jPoi["stime"].asString()!="") {
						std::string stimeS = jPoi["stime"].asString();
						int startOffset = 0;
						ToolFunc::toIntOffset(stimeS, startOffset);
						int endOffset = startOffset + dur;
						basePlan->m_vPlaceOpenMap[place->_ID] = std::pair<int, int>(startOffset, endOffset);
					}
				}
				//add by yangshu  一开始任意确定场次
				else if (vPlace->_t & LY_PLACE_TYPE_TOURALL) {
					const Tour *tour = dynamic_cast<const Tour*>(vPlace);
					if (tour) {
						std::string arvDate = MJ::MyTime::toString(basePlan->m_ArriveTime, basePlan->m_TimeZone).substr(0, 8);
						std::string deptDate = MJ::MyTime::toString(basePlan->m_DepartTime, basePlan->m_TimeZone).substr(0, 8);
						for (int i = 0; ; ++i) {
							std::string now = MJ::MyTime::datePlusOf(arvDate, i);
							if (basePlan->IsTourAvailable(tour, now)) {
								basePlan->m_PlaceDateMap[vPlace->_ID].insert(now);
								_INFO("wanle id:%s,date:%s",vPlace->_ID.c_str(),now.c_str());
							}
							if (now > deptDate) {
								break;
							}
						}
						if (basePlan->m_PlaceDateMap[vPlace->_ID].empty()) continue;
					}

					//add by shyy
					if (vPlace->_t & LY_PLACE_TYPE_VIEWTICKET) {
						const ViewTicket* viewTicket = dynamic_cast<const ViewTicket*>(vPlace);
						if(viewTicket != NULL) {
							const LYPlace* place = LYConstData::GetViewByViewTicket(const_cast<ViewTicket*>(viewTicket), basePlan->m_qParam.ptid);
							if (place != NULL) {
								std::string viewId = place->_ID;
								if (basePlan->m_notPlanSet.find(viewId) == basePlan->m_notPlanSet.end()) {
									basePlan->m_notPlanSet.insert(viewId);
								}
							}
						}
					}
					//add end
				}
				//add end
			}
		}
	}
	//智能优化选项
	if (req.isMember("option") && req["option"].isObject()) {
		if (req["option"].isMember("playDur") && req["option"]["playDur"].isInt() && req["option"]["playDur"].asInt() == 1) {
			basePlan->m_OptionMode.set(0);
		}
		if (req["option"].isMember("playOrder") && req["option"]["playOrder"].isInt() && req["option"]["playOrder"].asInt() == 1) {
			basePlan->m_OptionMode.set(1);
		}
		if (req["option"].isMember("playTimeRange") && req["option"]["playTimeRange"].isInt() && req["option"]["playTimeRange"].asInt() == 1) {
			basePlan->m_OptionMode.set(2);
		}
	}
	return 0;
}

const LYPlace* ReqParser::GetLYPlace(BasePlan* basePlan, const std::string& id, int pType, const QueryParam& param, ErrorInfo& errorInfo) {
	char buff[100];
	const LYPlace* place = basePlan->GetLYPlace(id);
	if (place && (place->_t & pType)) {
		return place;
	} else {
		if (id != "" ) {
			MJ::PrintInfo::PrintErr("%s ReqParser::GetLYPlace, 未收录(%s)信息", param.log.c_str(), id.c_str());
			if (id.substr(0, 2) == "ht") {
				snprintf(buff, sizeof(buff), "由于此行程的酒店在数据库中已下线，请先返回行程概览页更改酒店");
				errorInfo.Set(53102, std::string(buff), std::string(buff));
			} else {
				snprintf(buff, sizeof(buff), "未收录(%s)信息", id.c_str());
				errorInfo.Set(53102, std::string(buff));
			}
		} else {
			MJ::PrintInfo::PrintErr("%s ReqParser::GetLYPlace, id is a null string", param.log.c_str());
		}
		return NULL;
	}
}

const LYPlace* ReqParser::GetLYPlace(const std::string& id, int pType, const QueryParam& param, ErrorInfo& errorInfo) {
	char buff[100];
	const LYPlace* place = LYConstData::GetLYPlace(id, param.ptid);
	if (place && (place->_t & pType)) {
		return place;
	} else {
		if (id != "" ) {
			MJ::PrintInfo::PrintErr("%s ReqParser::GetLYPlace, 未收录(%s)信息", param.log.c_str(), id.c_str());
			snprintf(buff, sizeof(buff), "未收录(%s)信息", id.c_str());
			errorInfo.Set(53102, std::string(buff));
		} else {
			MJ::PrintInfo::PrintErr("%s ReqParser::GetLYPlace, id is a null string", param.log.c_str());
		}
		return NULL;
	}
}

int ReqParser::ParseProduct(const QueryParam& param, Json::Value& req, BasePlan* basePlan)
{
	ParseCarRental(param,req,basePlan);
	return 0;
}

int ReqParser::ParseCarRental(const QueryParam& param, Json::Value& req, BasePlan* basePlan)
{
	if(req.isMember("product") and req["product"].isObject() and req["product"].isMember("zuche") and req["product"]["zuche"].isObject())
	{
		std::string cityID = basePlan->m_City->_ID;
		Json::Value carRentals = req["product"]["zuche"];
		Json::Value::Members mem = carRentals.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++)
		{
			Json::Value oneCityCarRental = carRentals[*iter];
			//应该在检查请求时把不符合格式的租车产品去掉,否则分城市规划时可能会出现有借无还或没借却还的怪象
			if(not (oneCityCarRental.isObject()
						and oneCityCarRental["cars"].isArray()
						and oneCityCarRental["cars"].size()>0
						and oneCityCarRental["cars"][0].isObject()
						and oneCityCarRental["cars"][0]["source"].isObject())
			  ) continue;

			std::map<std::string,int> _map;
			int _getCar = 0;
			int _returnCar = 1;
			_map.insert(std::make_pair("start",_getCar));
			_map.insert(std::make_pair("end",_returnCar));
			for(auto it = _map.begin(); it!= _map.end(); it++)
			{
				if(oneCityCarRental[it->first].isObject()
						and oneCityCarRental[it->first]["cid"].isString()
						and cityID == oneCityCarRental[it->first]["cid"].asString()
				  )
				{
					Json::Value zuche = oneCityCarRental["cars"][0]["source"];
					if(zuche.isObject())
					{
						if(zuche[it->first].isObject()
								and zuche[it->first]["date"].isString()
								and zuche[it->first]["time"].isString()
								and zuche[it->first]["coord"].isString()
								and zuche[it->first]["name"].isString()
						  )
						{
							std::string name = zuche[it->first]["name"].asString();
							std::string lname = name;
							std::string date= zuche[it->first]["date"].asString();
							int iid=0;
							std::string zucheId = it->first + "#" + zuche[it->first]["coord"].asString() + "#" + zuche[it->first]["name"].asString();
							MJ::md5Last4(zucheId,iid);
							//MJ::md5Last4(zuche["unionkey"].asString(),iid);
							std::string id = "cs"+std::to_string(iid+it->second);
							std::string coord = zuche[it->first]["coord"].asString();
							std::string pstimeS = zuche[it->first]["time"].asString();
							std::string _beginTime = date+"_"+ pstimeS;
							time_t _tbeginTime = MJ::MyTime::toTime(_beginTime, basePlan->m_TimeZone);
							time_t _day0_time =  MJ::MyTime::toTime(date+"_00:00", basePlan->m_TimeZone);
							bool inTimeRange = false;
							for(int i=0; i<basePlan->m_RouteBlockList.size(); i++)
							{
								RouteBlock* routeBlock = basePlan->m_RouteBlockList[i];
								int arvTime = routeBlock->_arrive_time;
								int deptTime = routeBlock->_depart_time;
								if(_tbeginTime >= arvTime and _tbeginTime <= deptTime)
								{
									inTimeRange = true;
									break;
								}
							}
							if(not inTimeRange and param.type != "s131") continue;

							int _leaveHotel = _tbeginTime -1.0*3600 - _day0_time; //提前半个小时出发;粗略
							if(_leaveHotel < basePlan->m_HotelCloseTime and _leaveHotel >= basePlan->m_HotelCloseTime-4*3600)
							{
								basePlan->m_dayRangeMap[date]=std::make_pair(_tbeginTime-1.0*3600, basePlan->GetDayRange(date).second);
							}
							const LYPlace* cPlace = basePlan->SetCustomPlace(LY_PLACE_TYPE_CAR_STORE, id, name, lname, coord, POI_CUSTOM_MODE_MAKE);
							if (!cPlace) {
								MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCarStore, Make fail id = %s", param.log.c_str(), id.c_str());
								continue;
							}

							LYPlace * tcPlace = const_cast<LYPlace *>(cPlace);
							tcPlace->setMode(it->second);
							if(zuche["corp"].isObject() and zuche["corp"]["name"].isString()) tcPlace->_corp= zuche["corp"]["name"].asString();
							if(oneCityCarRental["product_id"].isString()) tcPlace->_pid = oneCityCarRental["product_id"].asString();
							if(zuche["unionkey"].isString()) tcPlace->_unionkey = zuche["unionkey"].asString();
							_INFO("parse carstore :%s, stime:%s",id.c_str(),_beginTime.c_str());
							//如果是取车点或者还车点,则将其伪装成景点;
							const LYPlace * place = basePlan->SetCustomPlace(LY_PLACE_TYPE_VIEW, tcPlace->_ID+"#faker", tcPlace->_name, tcPlace->_lname, tcPlace->_poi, POI_CUSTOM_MODE_CUSTOM);
							std::string placeId = place->_ID;
							std::string newId = basePlan->Insert2MultiMap(placeId);
							if (newId != "") place = basePlan->GetLYPlace(newId);
							if (place == NULL) continue;
							LYPlace * tPlace = const_cast<LYPlace *>(place);
							tPlace->_rawPlace = cPlace;
							VarPlace *tvPlace = dynamic_cast<VarPlace*>(tPlace);
							tvPlace->_level = VIEW_LEVEL_SYSOPT;
							tvPlace->SetHotLevel(200);//给所伪装的景点设置为一个极大值
							basePlan->m_userMustPlaceSet.insert(tvPlace);

							int startOffset = 0;
							ToolFunc::toIntOffset(pstimeS, startOffset);
							//没有考虑取车跨天
							basePlan->m_vPlaceOpenMap[place->_ID] = std::pair<int, int>(startOffset,startOffset+3600);
							basePlan->m_vPlaceOpenMap[tcPlace->_ID] = std::pair<int, int>(startOffset,startOffset+3600);
							basePlan->m_UserDurMap[place->_ID] = 3600;
							basePlan->m_UserDurMap[tcPlace->_ID] = 3600;
							basePlan->m_PlaceDateMap[place->_ID].insert(date);
						}
					}
				}
			}
		}
	}
	return 0;
}

int ReqParser::ParseFilterP104(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	basePlan->m_sortMode = 0;
	basePlan->m_reqMode = LY_PLACE_TYPE_TOURALL;
	ParseFilter(param, req, basePlan);
	if (req["filter"].isMember("mode") && req["filter"]["mode"].isArray() && req["filter"]["mode"].size()>0) {
		basePlan->m_reqMode = LY_PLACE_TYPE_NULL;
		for (int i = 0; i < req["filter"]["mode"].size(); ++i) {
			if (req["filter"]["mode"][i].isInt()) {
				basePlan->m_reqMode = req["filter"]["mode"][i].asInt();
			}
		}
	}
	return 0;
}

int ReqParser::ParseFilter(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	basePlan->m_sortMode = 0;
	if (!req.isMember("filter"))
		return 0;
	if (req["filter"].isMember("sort")) {
		basePlan->m_sortMode = req["filter"]["sort"].asInt();
	}
	if (req["filter"].isMember("key")) {
		basePlan->m_key = req["filter"]["key"].asString();
	}
	if (req["filter"].isMember("dist")) {
		basePlan->m_maxDist = req["filter"]["dist"].asInt();
	}
	if (req["filter"].isMember("utime")) {
		basePlan->m_utime = req["filter"]["utime"].asInt();
	}
	if (req["filter"].isMember("private")) {
		basePlan->m_privateFilter = req["filter"]["private"].asInt();
	}
	if (!req["filter"].isMember("tagIds"))
		return 0;
	if (req["filter"].isMember("tagIds")) {
		Json::Value& jTag = req["filter"]["tagIds"];
		std::vector<std::string> tags;
		for (int i = 0;i < jTag.size(); ++i) {
			tags.push_back(jTag[i].asString());
		}
		int type = req["mode"].asInt();
		if (tags.size() > 0) {//有才添加 没有直接跳过
			basePlan->m_filterTags = tags;
		}
	}
	if (req["filter"].isMember("centerId") && req["filter"]["centerId"].isString()) {            //GEM
				std::string cid = req["filter"]["centerId"].asString();
				basePlan->m_listCenter.push_back(cid);
	}
	if(basePlan->m_listCenter.empty())                                     //GEM
	{
		basePlan->m_listCenter.insert(basePlan->m_listCenter.end(), basePlan->m_listCity.begin(), basePlan->m_listCity.end());
	}
	return 0;
}

int ReqParser::DoParseSSV005(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	if (req.isMember("list")) {
		for (int i = 0; i < req["list"].size(); i ++) {
			if (ret == 0) {
				ret = ParseDaysNotPlan(param, req["list"][i], basePlan);
			}
		}
	} else {
		ret = ParseDaysNotPlan(param, req["city"], basePlan);
	}

	if (ret == 0) {
		ret = RemoveTogoPoi(basePlan);
	}

	if (ret == 0 and param.type == "ssv005_s130") {
		ret = ParseDaysProd(param, req, basePlan);
	}
	return ret;
}

int ReqParser::DoParseSSV006(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = ParseDaysPois(param, req, basePlan);
	return ret;
}

int ReqParser::ParseCustomPois(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	if (!req.isMember("days") || !req["days"].isArray()) return 0;
	for (int i = 0; i < req["days"].size(); ++i) {
		for (int j = 0; j < req["days"][i]["pois"].size(); ++j) {
			if (req["days"][i]["pois"][j].isMember("custom") && req["days"][i]["pois"][j]["custom"].isInt() && req["days"][i]["pois"][j]["custom"].asInt()) {
				std::string id = req["days"][i]["pois"][j]["id"].asString();
				std::string name = req["days"][i]["pois"][j]["name"].asString();
				std::string lname = req["days"][i]["pois"][j]["lname"].asString();
				std::string coord = req["days"][i]["pois"][j]["coord"].asString();
				int type = req["days"][i]["pois"][j]["type"].asInt();
				if(type == LY_PLACE_TYPE_CAR_STORE) continue; //租还车站点,始终只从product中取
				const LYPlace* place = basePlan->SetCustomPlace(type, id, name, lname, coord, POI_CUSTOM_MODE_CUSTOM);
				if (!place) {
					MJ::PrintInfo::PrintDbg("[%s]ReqParser::ParseCustomPois, Make fail id = %s", param.log.c_str(), id.c_str());
					continue;
				}
			}
		}
	}
	return 0;
}

int ReqParser::ParseDaysPois (const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	Json::Value& jDays = req["days"];
	/*const Json::Value& jViewDay = req["city"]["view"]["day"];
	if (jDays.size() != jViewDay.size()) {
		_INFO("ReqParser::ParseDaysPois, req is err, days.size != city.view.day.size");
		return 1;
	}*/

	std::tr1::unordered_set<std::string> trafKeySet;
	for(int i = 0; i < jDays.size(); i ++) {
		Json::Value& jDaysPois = jDays[i]["pois"];
		std::string date = jDays[i]["date"].asString();
		if (basePlan->m_notPlanDateSet.count(date)) continue;
		Json::Value jPoisList = Json::arrayValue;
		{
			/* days需满足以下格式
			 * 保证有站点情况下首天第一个点为到达站点 尾天最后一个点为离开站点
			 * 无站点时首尾补酒店 同时补stime pdur
			 * 保证days[i].poi.size()>=2
			 */
			/*
			 * 修改多重游玩id
			 * 解析玩乐票
			 */
			Json::Value jPoi = Json::Value();
			int day0time = MJ::MyTime::toTime(date+"_00:00", basePlan->m_TimeZone);
			//addArvPoi
			if (i == 0 && basePlan->m_arvNullPlace) {
				jPoi = Json::Value();
				const LYPlace* place = basePlan->m_arvNullPlace;
				jPoi["id"] = place->_ID;
				jPoi["stime"] = req["city"]["arv_time"].asString();
				jPoi["func_type"] = NODE_FUNC_KEY_ARRIVE;
				jPoi["pdur"] = 0;
				jPoisList.append(jPoi);
				_INFO("init poi list add arvPoi: %s", jPoi["id"].asString().c_str());
			}
			for (int j = 0; j < jDaysPois.size(); j ++) {
				jPoi = jDaysPois[j];
				std::string id = basePlan->GetCutId(jPoi["id"].asString());
				//直接根据func_type判断到达离开点
				if (jPoi.isMember("func_type") and jPoi["func_type"].isInt()) {
					if (jPoi["func_type"].asInt() == NODE_FUNC_KEY_ARRIVE) {
						std::string stime = MJ::MyTime::toString(basePlan->m_RouteBlockList[0]->_arrive_time,basePlan->m_TimeZone);
						jPoi["stime"] = stime;
					} else if (jPoi["func_type"].asInt() == NODE_FUNC_KEY_DEPART) {
						int etime = basePlan->m_RouteBlockList[0]->_depart_time;
						int pdur = 0;
						if (jPoi.isMember("pdur") && jPoi["pdur"].isInt()) pdur = jPoi["pdur"].asInt();
						std::string stime = MJ::MyTime::toString(etime - pdur,basePlan->m_TimeZone);
						jPoi["stime"] = stime;
					}
				}
				//兼容老行程 无func_type
				{
					//加到达站点stime
					if (!jPoi.isMember("func_type") && i==0 && j==0 && id != "arvNullPlace") {
						if (id == basePlan->m_RouteBlockList[0]->_arrive_place->_ID) {
							std::string stime = MJ::MyTime::toString(basePlan->m_RouteBlockList[0]->_arrive_time,basePlan->m_TimeZone);
							jPoi["stime"] = stime;
							jPoi["func_type"] = NODE_FUNC_KEY_ARRIVE;
						}
					}
					//加离开站点etime
					if (!jPoi.isMember("func_type") && i==jDays.size()-1 && j==jDaysPois.size()-1 && id != "deptNullPlace"){
						if (id == basePlan->m_RouteBlockList[0]->_depart_place->_ID) {
							int etime = basePlan->m_RouteBlockList[0]->_depart_time;
							int pdur = 0;
							if (jPoi.isMember("pdur") && jPoi["pdur"].isInt()) pdur = jPoi["pdur"].asInt();
							std::string stime = MJ::MyTime::toString(etime - pdur,basePlan->m_TimeZone);
							jPoi["stime"] = stime;
							jPoi["func_type"] = NODE_FUNC_KEY_DEPART;
						}
					}
				}

				//玩乐特殊处理id为pid
				//解析票的信息
				Json::Value jTickets = Json::arrayValue;
				{
					if (jPoi.isMember("product") && jPoi["product"].isObject()) {
						if (jPoi["product"].isMember("product_id") && jPoi["product"]["product_id"].isString()
								&& req["product"]["wanle"].isMember(jPoi["product"]["product_id"].asString())) {
							std::string productId = jPoi["product"]["product_id"].asString();
							if (req["product"]["wanle"][productId].isMember("pid") && req["product"]["wanle"][productId]["pid"].isString()) {
								id = req["product"]["wanle"][productId]["pid"].asString();
							}
							if(req["product"]["wanle"][productId].isMember("tickets") and req["product"]["wanle"][productId]["tickets"].isArray()) {
								jTickets = req["product"]["wanle"][productId]["tickets"];
							}
							if (req["product"]["wanle"][productId].isMember("stime") and req["product"]["wanle"][productId]["stime"].isString() and req["product"]["wanle"][productId]["stime"].asString().length() == 5) {
								jPoi["stime"] = req["product"]["wanle"][productId]["stime"].asString();
							}
						} else {
							if (jPoi["product"].isMember("pid") && jPoi["product"]["pid"].isString()) {
								id = jPoi["product"]["pid"].asString();
							}
							if(jPoi["product"].isMember("tickets") && jPoi["product"]["tickets"].isArray()) {
								jTickets = jPoi["product"]["tickets"];
							}
						}
					}
				}
				const LYPlace* place = basePlan->GetLYPlace(id);
				if (id == "") {
					MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, id is null", param.log.c_str());
					return 1;
				}
				if (place == NULL) {
					_INFO("ReqParse::ParseDaysPois::Get Place err, id is: %s", id.c_str());
					//换正确酒店
					if (jPoi.isMember("type") && jPoi["type"].isInt() && jPoi["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
						if (j == 0) place = GetHotelByDidx(basePlan, i-1);
						else place = GetHotelByDidx(basePlan, i);
					}
					if (place == NULL) continue;
					id = place->_ID;
				}
				//增加判断行李点逻辑，兼容未输出func_type行程
				if (not jPoi.isMember("func_type") and place->_t == LY_PLACE_TYPE_HOTEL) {
					if (i == 0 and j != jDaysPois.size()-1) {
						jPoi["func_type"] = NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE;
					} else if (i == jDays.size()-1 and j != 0) {
						jPoi["func_type"] = NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE;
					} else {
						jPoi["func_type"] = NODE_FUNC_KEY_HOTEL_SLEEP;
					}
				}

				std::string newId = basePlan->Insert2MultiMap(id);
				if (newId == "") newId = id;
				const LYPlace* newPlace = basePlan->GetLYPlace(newId);
				if (newPlace->_t == LY_PLACE_TYPE_VIEWTICKET) {
					const ViewTicket *viewTicket = dynamic_cast<const ViewTicket*>(newPlace);
					if (viewTicket != NULL) {
						//确保景点门票关联的景点存在
						if (LYConstData::GetViewByViewTicket(const_cast<ViewTicket*>(viewTicket), basePlan->m_qParam.ptid) == NULL) {
							MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, yangshu viewTicket view is null, id = %s", param.log.c_str(), id.c_str());
							continue;
						}
					}
				}

				jPoi["id"] = newId;
				//读取玩法play信息
				if(jPoi.isMember("play") and jPoi["play"].isString()) basePlan->m_poiPlays[newId] = jPoi["play"].asString();
				//读取功能信息
				if(jPoi.isMember("func_type") and jPoi["func_type"].isInt() and jPoi["func_type"].asInt()) basePlan->m_poiFunc[newId] = jPoi["func_type"].asInt();

				//读取票的信息,增加多重游玩后再加票信息,保证对应
				{
					for (int j = 0; j < jTickets.size(); ++j) {
						if (jTickets[j].isMember("source") && jTickets[j]["source"].isObject() &&
								jTickets[j]["source"].isMember("ticketId") && jTickets[j]["source"]["ticketId"].isInt() &&
								jTickets[j].isMember("num") && jTickets[j]["num"].isInt()) {
							basePlan->m_pid2ticketIdAndNum[newPlace->_ID].insert(std::make_pair(jTickets[j]["source"]["ticketId"].asInt(), jTickets[j]["num"].asInt()));
						}
					}
				}

				//修改行程传入pdur 为 -1 需要修正
				if (newPlace->_t & LY_PLACE_TYPE_ARRIVE && jPoi["pdur"].asInt() == -1) {
					if (i == 0 && j == 0 && jPoi["id"] != "arvNullPlace") {					
						jPoi["pdur"] = basePlan->GetEntryTime(newPlace);
					} else {
						std::string stime = jPoi["stime"].asString();
						jPoi["pdur"] = basePlan->GetExitTime(newPlace);
						int pdur = jPoi["pdur"].asInt();
						jPoi["stime"] = MJ::MyTime::toString(MJ::MyTime::toTime(stime) - pdur);
					}
				}

				if (!basePlan->m_keepPlayRange && (j == 0 || j == jDaysPois.size()-1)) {
					//此处调整酒店为偏好时间
					//主要是在智能优化突破酒店范围时起作用
					if (jPoi.isMember("type") && jPoi["type"].isInt() && jPoi["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
						std::string hotelStime = jPoi["stime"].asString();
						std::string hotelEtime = "";
						if (jPoi.isMember("etime"))	hotelEtime = jPoi["etime"].asString();
						else hotelEtime = MJ::MyTime::toString(MJ::MyTime::toTime(hotelStime)+jPoi["pdur"].asInt());
						std::string hotelOpenTime = "", hotelCloseTime = "";
						ToolFunc::toStrTime(basePlan->m_HotelOpenTime, hotelOpenTime);
						ToolFunc::toStrTime(basePlan->m_HotelCloseTime, hotelCloseTime);
						if (j==0) {
							hotelOpenTime = MJ::MyTime::datePlusOf(date,-1) + "_" + hotelOpenTime;
							hotelCloseTime = date + "_" + hotelCloseTime;
						} else {
							hotelOpenTime = date + "_" + hotelOpenTime;
							hotelCloseTime = MJ::MyTime::datePlusOf(date,1) + "_" + hotelCloseTime;
						}
						if (hotelStime < hotelOpenTime) hotelStime = hotelOpenTime;
						if (hotelEtime > hotelCloseTime) hotelEtime = hotelCloseTime;
						int pdur = MJ::MyTime::toTime(hotelEtime) - MJ::MyTime::toTime(hotelStime);
						jPoi["pdur"] = pdur;
						jPoi["stime"] = hotelStime;
						jPoi["etime"] = hotelEtime;
						_INFO("break hotel Range, change stime - etime: %s - %s, pdur: %d", hotelStime.c_str(), hotelEtime.c_str(), pdur);
					}
				}
				//确保stime为日期+时刻
				if (newPlace->_t == LY_PLACE_TYPE_CAR_STORE
						&& (!jPoi.isMember("stime") || jPoi["stime"].asString().length()!=14)) {
					if (basePlan->m_vPlaceOpenMap.find(id) != basePlan->m_vPlaceOpenMap.end()) {
						int stimeHour = basePlan->m_vPlaceOpenMap[id].first;
						std::string stimeStr = "";
						ToolFunc::toStrTime(stimeHour,stimeStr);
						if (stimeStr != "") {
							std::string stime = date + "_" + stimeStr;
							jPoi["stime"] = stime;
						}
					}
				}
				if (newPlace->_t & LY_PLACE_TYPE_TOURALL && jPoi.isMember("stime") && jPoi["stime"].asString().length()==5) {
					std::string stime = date + "_" + jPoi["stime"].asString();
					jPoi["stime"] = stime;
				}

				jPoisList.append(jPoi);
				_INFO("init poi list add didx: %d  poi: %s,index: %d",i,jPoi["id"].asString().c_str(), j);
			}
			//加deptPoi 
			if (i+1 == jDays.size() && basePlan->m_deptNullPlace) {
				jPoi = Json::Value();
				const LYPlace* place = basePlan->m_deptNullPlace;
				jPoi["id"] = place->_ID;
				jPoi["stime"] = req["city"]["dept_time"].asString();
				jPoi["pdur"] = 0;
				jPoi["func_type"] = NODE_FUNC_KEY_DEPART;
				if (basePlan->m_qParam.type == "ssv006_light") jPoi["needAdjustHotel"] = 1;  //平移时酒店可以前推
				jPoisList.append(jPoi);
				_INFO("init poi list add deptPoi:%s", jPoi["id"].asString().c_str());
			}
		}
		if (jPoisList.size() < 2) {
			_INFO("ParseDaysPois error, didx: %d, pois size < 2", i);
			return 1;
		}
		Json::FastWriter fw;
		std::cerr << "date: " << date << "  jpoislist:" << std::endl << fw.write(jPoisList) << std::endl;
		//获取交通
		for (int j = 0; j+1 < jPoisList.size(); j++) {
			std::string fromId = basePlan->GetCutId(jPoisList[j]["id"].asString());
			std::string toId = basePlan->GetCutId(jPoisList[j+1]["id"].asString());
			std::string trafKey = fromId + "_" + toId;
			if (fromId == "arvNullPlace" || toId == "deptNullPlace") {
				TrafficItem* traf = new TrafficItem(*TrafficData::_blank_traffic_item);
				if (basePlan->m_customTrafMap.find(trafKey) == basePlan->m_customTrafMap.end()) {
					basePlan->m_customTrafMap[trafKey] = traf;
				}
			}
			if (fromId == toId) continue;
			trafKey = trafKey + "_" + date;
			_INFO("add trafKey: %s", trafKey.c_str());
			trafKeySet.insert(trafKey);
		}
		jDaysPois = jPoisList;
	}
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	if (param.type != "s131") {
		ret = PathTraffic::GetTrafPair(basePlan, trafKeySet, basePlan->m_realTrafNeed & REAL_TRAF_ADVANCE);
	} else {
		//s131 不打交通服务  对于新增的trafKey(主要是酒店出发- 第一个点和最后一个点到酒店离开点) 交通时间置0
		for (auto it = trafKeySet.begin(); it != trafKeySet.end(); it++) {
			std::string trafKey = *it;
			std::vector<std::string> itemList;
			ToolFunc::sepString(trafKey, "_", itemList);
			if (itemList.size() != 3) {
				MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysPois, bad traf key: %s", basePlan->m_qParam.log.c_str(), trafKey.c_str());
				return 1;
			}
			std::string start = itemList[0];
			std::string end = itemList[1];
			std::string date = itemList[2];
			const TrafficItem* trafItem = basePlan->GetTraffic(start, end, date);
			if (trafItem == NULL) {
				TrafficItem* traf = new TrafficItem(*TrafficData::_blank_traffic_item);
				std::string trafKey = start + "_" + end;
				if (basePlan->m_lastTripTrafMap.find(trafKey) == basePlan->m_lastTripTrafMap.end()) {
					basePlan->m_lastTripTrafMap[trafKey] = traf;
				}
			}
		}
	}
	basePlan->m_cost.m_traffic8003 = t.cost();
	if (ret != 0) {
		_INFO("ReqParser::ParseDaysPois, get traffic error");
		return 1;
	}
	return 0;
}

const LYPlace* ReqParser::GetHotelByDidx (BasePlan* basePlan, int didx) {
	const LYPlace* hotel = NULL;
	if (didx < 0) didx = 0;
	const HInfo* hInfo = basePlan->GetHotelByDidx(didx);
	if (hInfo) hotel = hInfo->m_hotel;
	else {
		hotel = LYConstData::GetCoreHotel(basePlan->m_City->_ID);
	}
	if (hotel == NULL) {
		_INFO("get core hotel error, city:%s",basePlan->m_City->_ID.c_str());
	}
	return hotel;
}

int ReqParser::ParseDaysProd(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	Json::Value& jDayList = req["days"];
	std::vector<std::pair<Json::Value, std::string> > sJDayList;
	std::vector<PlaceOrder> tmpPlaceOrderList;  //存放到达离开点,租车点,玩乐
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		std::string date = jDay["date"].asString();
		Json::Value& jCity = req["city"];
		Json::Value arvPoi;
		if (i == 0) {
			Json::Value new_jPois = Json::arrayValue;
			arvPoi["id"] = basePlan->m_RouteBlockList[0]->_arrive_place->_ID;
			std::string stime = MJ::MyTime::toString(basePlan->m_RouteBlockList[0]->_arrive_time,basePlan->m_TimeZone);
			arvPoi["stime"] = stime.substr(9);
			arvPoi["date"] = stime.substr(0,8);
			arvPoi["func_type"] = NODE_FUNC_KEY_ARRIVE;
			//如果传进来pois[0]==arv 不考虑date
			if (jDay["pois"].isArray() && jDay["pois"].size() > 0 && jDay["pois"][0U]["id"] == arvPoi["id"]
					and not (basePlan->m_RouteBlockList[0]->_arrive_place->_t & LY_PLACE_TYPE_HOTEL)) {
				arvPoi["pdur"] = jDay["pois"][0U]["pdur"];
				basePlan->m_RouteBlockList[0]->_arrive_dur = arvPoi["pdur"].asInt();
				new_jPois = jDay["pois"];
				new_jPois[0U] = arvPoi;
				jDay["pois"] = new_jPois;
			} else {
				const LYPlace* place = basePlan->m_RouteBlockList[0]->_arrive_place;
				if (place) {
					arvPoi["pdur"] = basePlan->GetEntryTime(place);
				}
				new_jPois.append(arvPoi);
				for (int j = 0; j < jDay["pois"].size(); j ++) {
					new_jPois.append(jDay["pois"][j]);
				}
				jDay["pois"] = new_jPois;
			}
		}
		if (i == jDayList.size()-1) {
			Json::Value deptPoi;
			deptPoi["id"] = basePlan->m_RouteBlockList[0]->_depart_place->_ID;
			std::string etime = MJ::MyTime::toString(basePlan->m_RouteBlockList[0]->_depart_time,basePlan->m_TimeZone);
			deptPoi["etime"] = etime.substr(9);
			if (((jDay["date"].asString() != arvPoi["date"].asString() && jDay["pois"].isArray() && jDay["pois"].size() > 0)
					||(jDay["date"].asString() == arvPoi["date"].asString() && jDay["pois"].isArray() && jDay["pois"].size() > 1))
					&& (jDay["pois"][jDay["pois"].size()-1]["id"] == deptPoi["id"])
					and not (basePlan->m_RouteBlockList[0]->_depart_place->_t & LY_PLACE_TYPE_HOTEL)) {
				deptPoi["pdur"] = jDay["pois"][jDay["pois"].size()-1]["pdur"];
				std::string stime = MJ::MyTime::toString(basePlan->m_RouteBlockList[0]->_depart_time - jDay["pois"][jDay["pois"].size()-1]["pdur"].asInt(), basePlan->m_TimeZone);
				deptPoi["stime"] = stime;
				deptPoi["func_type"] = NODE_FUNC_KEY_DEPART;
				jDay["pois"][jDay["pois"].size()-1] = deptPoi;
				basePlan->m_RouteBlockList[0]->_depart_dur = deptPoi["pdur"].asInt();
			}
			else if (jDay["date"].asString() == etime.substr(0,8)) {
				const LYPlace* place = basePlan->m_RouteBlockList[0]->_depart_place;
				if (place) {
					deptPoi["pdur"] = basePlan->GetExitTime(place);
				}
				jDay["pois"].append(deptPoi);
			}
			else {
				//防止离开点的时间>days的最后一天
				Json::Value newDay = Json::Value();
				newDay["date"] = etime.substr(0,8);
				const LYPlace* place = basePlan->m_RouteBlockList[0]->_depart_place;
				if (place) {
					deptPoi["pdur"] = basePlan->GetExitTime(place);
				}
				newDay["pois"].append(deptPoi);
				if (!newDay.isNull()) sJDayList.push_back(std::pair<Json::Value, std::string>(newDay, newDay["date"].asString()));
			}
		}
		sJDayList.push_back(std::pair<Json::Value, std::string>(jDay, date));
	}
	std::sort(sJDayList.begin(), sJDayList.end(), sort_pair_second<Json::Value, std::string, std::less<std::string> >());

	std::map<std::string ,std::string> varPlaceDate;
	for (int i = 0; i < sJDayList.size(); ++i) {
		Json::Value& jDay = sJDayList[i].first;
		std::string date = jDay["date"].asString();
		if (basePlan->m_notPlanDateSet.count(date)) continue;
		Json::Value& jPoiList = jDay["pois"];
		for (int j = 0; j < jPoiList.size(); ++j) {
			Json::Value& jPoi = jPoiList[j];
			std::string id = jPoi["id"].asString();
			std::string stime;
			//兼容老格式
			if(jPoi.isMember("pstime") and jPoi["pstime"].isString()) stime = jPoi["pstime"].asString();
			if(jPoi.isMember("stime") and jPoi["stime"].isString()) stime = jPoi["stime"].asString();

			//从product中取stime，tickets信息
			Json::Value jTickets = Json::arrayValue;
			if(jPoi.isMember("product") && jPoi["product"].isObject())
			{
				//不修改从product中拿所需stime
				if(jPoi["product"].isMember("product_id") && jPoi["product"]["product_id"].isString())
				{
					std::string productId = jPoi["product"]["product_id"].asString();
					if(req.isMember("product") and req["product"].isObject()
							and req["product"].isMember("wanle") and req["product"]["wanle"].isObject()
							and req["product"]["wanle"].isMember(productId) and req["product"]["wanle"][productId].isObject())
					{
						if (req["product"]["wanle"][productId].isMember("pid") and req["product"]["wanle"][productId]["pid"].isString()) {
							id = req["product"]["wanle"][productId]["pid"].asString();
						}
						if(req["product"]["wanle"][productId].isMember("stime") and req["product"]["wanle"][productId]["stime"].isString())
						{
							stime = req["product"]["wanle"][productId]["stime"].asString();
						}
						if(req["product"]["wanle"][productId].isMember("tickets") and req["product"]["wanle"][productId]["tickets"].isArray()) {
							jTickets = req["product"]["wanle"][productId]["tickets"];
						}
					}
				} else {
				//修改的情况下直接根据新传入信息
					if(jPoi["product"].isMember("stime") and jPoi["product"]["stime"].isString()) stime = jPoi["product"]["stime"].asString();
					if(jPoi["product"].isMember("tickets") && jPoi["product"]["tickets"].isArray()) {
						jTickets = jPoi["product"]["tickets"];
					}
					if(jPoi["product"].isMember("pid") && jPoi["product"]["pid"].isString()) {
						id = jPoi["product"]["pid"].asString(); //只有景点门票的pid 和 id不一样
					}
				}
			}

			const LYPlace* place = NULL;
			//去除请求中不正确的景点信息
			{
				if (id == "") {
					MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, id is null", param.log.c_str());
					return 1;
				}
				//如果租车产品在新的行程中已经不存在,则place为空,该点被合理的忽略
				place = basePlan->GetLYPlace(id);
				if (place == NULL) {
					MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, yangshu place is null, id = %s", param.log.c_str(), id.c_str());
					continue;
				}
				const ViewTicket *viewTicket = dynamic_cast<const ViewTicket*>(place);
				if (viewTicket != NULL) {
					//确保景点门票关联的景点存在
					if (LYConstData::GetViewByViewTicket(const_cast<ViewTicket*>(viewTicket), basePlan->m_qParam.ptid) == NULL) {
						MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, yangshu viewTicket view is null, id = %s", param.log.c_str(), id.c_str());
						continue;
					}
				}
			}

			if( place->_t & LY_PLACE_TYPE_CAR_STORE )
			{
				//"加#号 可以使最后view的traffic 的 id字段正确"
				place = basePlan->GetLYPlace(place->_ID+"#faker");
			}
			else if (place->_t & LY_PLACE_TYPE_VAR_PLACE || place->_t & LY_PLACE_TYPE_ARRIVE || place->_t & LY_PLACE_TYPE_HOTEL)
			{
				//统一进行 加多重游玩景点
				//车站亦需要加多重游玩
				std::string newId = basePlan->Insert2MultiMap(id);
				if (newId == "") {
					MJ::PrintInfo::PrintLog("[%s] %s make Multi failed", param.log.c_str(), id.c_str());
					continue;
				}
				place = basePlan->GetLYPlace(newId);
			}

			//读取票的信息,增加多重游玩后再加票信息,保证对应
			{
				for (int j = 0; j < jTickets.size(); ++j) {
					if (jTickets[j].isMember("source") && jTickets[j]["source"].isObject() &&
							jTickets[j]["source"].isMember("ticketId") && jTickets[j]["source"]["ticketId"].isInt() &&
							jTickets[j].isMember("num") && jTickets[j]["num"].isInt()) {
						basePlan->m_pid2ticketIdAndNum[place->_ID].insert(std::make_pair(jTickets[j]["source"]["ticketId"].asInt(), jTickets[j]["num"].asInt()));
					}
				}
			}
			//读取玩法play信息
			if(jPoi.isMember("play") and jPoi["play"].isString()) basePlan->m_poiPlays[place->_ID] = jPoi["play"].asString();
			//读取功能信息
			if(jPoi.isMember("func_type") and jPoi["func_type"].isInt() and jPoi["func_type"].asInt()) basePlan->m_poiFunc[place->_ID] = jPoi["func_type"].asInt();

			if ((place->_t & LY_PLACE_TYPE_VAR_PLACE) && LYConstData::IsRealID(place->_ID)) {
				const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
				if(place->getRawType() != LY_PLACE_TYPE_CAR_STORE) //租车在解析product时加入集合
				{
					basePlan->m_userMustPlaceSet.insert(place);
				}
				//在玩乐产品结构分离前,玩乐的游玩日期从此处读
				if(place->_t & LY_PLACE_TYPE_TOURALL) basePlan->m_PlaceDateMap[place->_ID].insert(date);
				if(place->_t & LY_PLACE_TYPE_VAR_PLACE) varPlaceDate[place->_ID] = date;

				if ((place->_t & LY_PLACE_TYPE_TOURALL) || place->getRawType() == LY_PLACE_TYPE_CAR_STORE) {
					std::cerr << "add tmp order place ID : " << place->_ID << std::endl;
					tmpPlaceOrderList.push_back(PlaceOrder(place, date, tmpPlaceOrderList.size(), NODE_FUNC_PLACE_VIEW_SHOP));  //保证tmpPlaceOrder index 与userpalceOrder index 一致
				}
			}
			else if ((place->_t & LY_PLACE_TYPE_ARRIVE|| place->_t & LY_PLACE_TYPE_HOTEL) && LYConstData::IsRealID(place->_ID)) {
				unsigned char nodeType = NODE_FUNC_NULL;
				tmpPlaceOrderList.push_back(PlaceOrder(place, date, tmpPlaceOrderList.size(), nodeType));
			}

			if (jPoi.isMember("pdur") && jPoi["pdur"].isInt()) {
				int dur = jPoi["pdur"].asInt();
				int pdur = dur;
				if (pdur == -1) {
					pdur = 4*3600;
				}
				if (pdur == -2) {
					pdur = 8*3600;
				}
				if (pdur == -3) {
					pdur = 2*3600;
				}
				LYPlace * tplace = const_cast<LYPlace *> (place);
				if(basePlan->m_keepTime || place->_t &LY_PLACE_TYPE_TOURALL)
				{
					basePlan->m_UserDurMap[place->_ID] = pdur;
				}
				//普通景点已经无pstime,只有玩乐会有
				if (stime.length()==14) stime = stime.substr(9);
				if (stime.length()==5 and (place->_t & LY_PLACE_TYPE_ARRIVE || place->_t & LY_PLACE_TYPE_TOURALL || place->_t & LY_PLACE_TYPE_HOTEL || place->_rawPlace != NULL)){
					int startOffset = 0;
					ToolFunc::toIntOffset(stime, startOffset);
					int endOffset = startOffset + pdur;
					basePlan->m_vPlaceOpenMap[place->_ID] = std::pair<int, int>(startOffset, endOffset);
				}
				if (place->getRawType() == LY_PLACE_TYPE_CAR_STORE && basePlan->m_vPlaceOpenMap.find(place->_ID) != basePlan->m_vPlaceOpenMap.end() && basePlan->m_UserDurMap.find(place->_ID) != basePlan->m_UserDurMap.end()) {
					int startOffset = basePlan->m_vPlaceOpenMap[place->_ID].first;
					int dur = basePlan->m_UserDurMap[place->_ID];
					basePlan->m_vPlaceOpenMap[place->_ID] = std::make_pair(startOffset, startOffset+dur);
				}
			}
		}  // for
	}  // for

	//一个景点多次游玩时,在重新规划时仍保持规划的日期不变
	{
		std::map<std::string,int> playCounter;
		for(auto it=varPlaceDate.begin();it!=varPlaceDate.end();it++)
		{
			playCounter[basePlan->GetCutId(it->first)]++;
		}
		for(auto it=varPlaceDate.begin();it!=varPlaceDate.end();it++)
		{
			if(playCounter[basePlan->GetCutId(it->first)]>1) basePlan->m_PlaceDateMap[it->first].insert(it->second);
		}
	}

	if (basePlan->clashDelPoi) {
		std::tr1::unordered_set<const LYPlace*> delPoisSet;
		int ret = DealClashOfSpecialPois(basePlan, tmpPlaceOrderList, delPoisSet);
		if (ret) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysProd, deal arrive/dept pois, carstore, tour clash error", basePlan->m_qParam.log.c_str());
			//return ret;
		}
		MJ::PrintInfo::PrintLog("[%s]ReqParser::ParseDaysProd, deal Pois: ", basePlan->m_qParam.log.c_str());
		for(auto it = delPoisSet.begin(); it != delPoisSet.end(); it++ ) {
			std::cerr << (*it)->_ID << "  " << (*it)->_name << std::endl;
		}
	}
	return 0;
}


int ReqParser::ParseDaysNotPlan(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	if (req.isMember("view") && req["view"].isMember("summary")) {
		for (int i = 0; i < req["view"]["summary"]["days"].size(); i++) {
			std::string date = req["view"]["summary"]["days"][i]["date"].asString();
			if (date == "" || basePlan->m_notPlanDateSet.count(date)) {//1.date为空：途经点中已有点或团游中已规划点不规划；2.notPlanDate
				Json::Value day = req["view"]["summary"]["days"][i];
				for(int j = 0; j < day["pois"].size(); j++) {
					Json::Value& jPoi = day["pois"][j];
					std::string id = jPoi["id"].asString();
					if (id == "") {
						MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseDaysNotPlan, id is null", param.log.c_str());
						return 1;
					}
					std::string cutId = basePlan->GetCutId(id);
					if (basePlan->m_notPlanSet.find(cutId) == basePlan->m_notPlanSet.end()) {//不规划的点集
						basePlan->m_notPlanSet.insert(cutId);
						_INFO("add %s not Plan", cutId.c_str());
					}
				}
			}
		}
	}
	return 0;
}

int ReqParser::ParseTrafFromView(Json::Value& jView, Json::Value& jCustomTrafList, Json::Value& jLastTripTrafList) {
	_INFO("get custom traffic from originView...");
	if (jView.isMember("day") && jView["day"].isArray()) {
		for (int i = 0; i < jView["day"].size(); i++) {
			Json::Value& jDay = jView["day"][i];
			if (jDay.isMember("view") && jDay["view"].isArray()) {
				Json::Value& jViewList = jDay["view"];
				for (int j = 0; j+1 < jViewList.size(); j ++) {
					Json::Value& jNowView = jViewList[j];
					if (jNowView.isMember("traffic") && jNowView["traffic"].isObject()
							&& jNowView["traffic"].isMember("id") && jNowView["traffic"]["id"].isString()
							//目前所有交通均保留
							&& jNowView["traffic"].isMember("custom") && jNowView["traffic"]["custom"].isInt()) {
						std::vector<std::string> itemList;
						ToolFunc::sepString(jNowView["traffic"]["id"].asString(), "_", itemList);
						if (itemList.size() < 2) {
							_INFO("ReqParser::ParseCustomTraf, trafKey split error: %s", jNowView["traffic"]["id"].asString().c_str());
							continue;
						}
						std::string trafKey = itemList[0] + "_" + itemList[1];
						if (jNowView["traffic"]["custom"].asInt()) {
							jCustomTrafList[trafKey] = jNowView["traffic"];
							_INFO("%s, is custom traffic", jNowView["traffic"]["id"].asString().c_str());
						} else {
							jLastTripTrafList[trafKey] = jNowView["traffic"];
							_INFO("%s, is last trip traffic", trafKey.c_str());
						}
					}
				}
			}
		}
	}
	return 0;
}

int ReqParser::ParseCustomTraf(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	_INFO("ReqParse::parse custom traffic ...");
	char buff[1000];
	Json::Value jCustomTrafficFromOriView = Json::Value();
	Json::Value jLastTripTrafficFromOriView = Json::Value();
	if (req.isMember("city") && req["city"].isObject() && req["city"].isMember("originView") && req["city"]["originView"].isObject()) {
		ParseTrafFromView(req["city"]["originView"], jCustomTrafficFromOriView, jLastTripTrafficFromOriView);
	}
	Json::Value jCustomTrafficFromView = Json::Value();
	Json::Value jLastTripTrafficFromView = Json::Value();
	if (req.isMember("city") && req["city"].isObject() && req["city"].isMember("view") && req["city"]["view"].isObject()) {
		ParseTrafFromView(req["city"]["view"], jCustomTrafficFromView, jLastTripTrafficFromView);
	}
	//自定义交通
	Json::Value jCustomTraffic = req["customTraffic"];
	if ( param.type == "s205"
		||	param.type == "s128")
		jCustomTraffic = req["city"]["customTraffic"];
	Json::Value::Members jMemList = jCustomTrafficFromView.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		if(!jCustomTraffic.isMember(*it)) jCustomTraffic[*it] = jCustomTrafficFromView[*it];
	}
	jMemList = jCustomTrafficFromOriView.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		if(!jCustomTraffic.isMember(*it)) jCustomTraffic[*it] = jCustomTrafficFromOriView[*it];
	}
	jMemList = jCustomTraffic.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		std::string trafKey = *it;
		Json::Value jTraffic = jCustomTraffic[trafKey];
		TrafficItem* trafItem = ParseTraffic2TrafItem (param, basePlan, trafKey, jTraffic);
		if (!trafItem) continue;

		std::string start2end = trafItem->m_startP + "_" + trafItem->m_stopP;
		if (basePlan->m_customTrafMap.find(start2end) == basePlan->m_customTrafMap.end()) {
			basePlan->m_customTrafMap[start2end] = trafItem;
		} else {
			//snprintf(buff, sizeof(buff), "请求格式异常: customTraffic error, trafKey: %s repeated", trafKey.c_str());
			//MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCustomTraf, customTraffic error, trafKey : %s repeated", param.log.c_str(), trafKey.c_str());
			//basePlan->m_error.Set(50000, buff);
			delete trafItem;
			trafItem = NULL;
			continue;
		}
	}
	// 上一次规划的交通
	Json::Value jLastTripTrafList = Json::Value();
	jMemList = jLastTripTrafficFromOriView.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		if(!jLastTripTrafList.isMember(*it)) jLastTripTrafList[*it] = jLastTripTrafficFromOriView[*it];
	}
	jMemList = jLastTripTrafficFromView.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		if(!jLastTripTrafList.isMember(*it)) jLastTripTrafList[*it] = jLastTripTrafficFromView[*it];
	}
	jMemList = jLastTripTrafList.getMemberNames();
	for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); ++it) {
		std::string trafKey = *it;
		Json::Value jTraffic = jLastTripTrafList[trafKey];
		TrafficItem* trafItem = ParseTraffic2TrafItem (param, basePlan, trafKey, jTraffic);
		if (!trafItem) continue;

		std::string trafS2P = trafItem->m_startP + "_" + trafItem->m_stopP;
		if (basePlan->m_lastTripTrafMap.find(trafS2P) == basePlan->m_lastTripTrafMap.end()) {
			basePlan->m_lastTripTrafMap[trafS2P] = trafItem;
		} else {
			delete trafItem;
			trafItem = NULL;
			continue;
		}
	}

	return 0;
}
TrafficItem* ReqParser::ParseTraffic2TrafItem (const QueryParam& param, BasePlan* basePlan, std::string trafKey, Json::Value& jTraffic) {
	std::vector<std::string> itemList;
	ToolFunc::sepString(trafKey, "_", itemList);
	if (itemList.size() < 2) {
		MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCustomTraf, trafKey split error: %s", param.log.c_str(), trafKey.c_str());
		return NULL;
	}
	TrafficItem* trafItem = new TrafficItem();

	if (jTraffic.isMember("custom") and jTraffic["custom"].isInt() and jTraffic["custom"].asInt() == 0) {
		trafItem->m_custom = false;
	} else {
		trafItem->m_custom = true;
	}
	trafItem->m_startP = itemList[0];
	trafItem->m_stopP = itemList[1];
	trafItem->m_order = 0;
	trafItem->_mid = trafKey;
	if (jTraffic.isMember("id") and jTraffic["id"].isString()) trafItem->_mid = jTraffic["id"].asString();
	trafItem->_time = jTraffic["dur"].asInt();
	trafItem->_dist = jTraffic["dist"].asInt();
	const LYPlace* placeA = basePlan->GetLYPlace(trafItem->m_startP);
	const LYPlace* placeB = basePlan->GetLYPlace(trafItem->m_stopP);
	if (!placeA or !placeB) {
		delete trafItem;
		trafItem = NULL;
		return trafItem;
	}
	//不修改交通距离
	//if (trafItem->_dist == 0) {
	//	trafItem->_dist = LYConstData::CaluateSphereDist(placeA, placeB);
	//}
	trafItem->m_realDist = trafItem->_dist;
	if (DataChecker::IsBadTraf(trafItem)) {
		char buff[1000];
		snprintf(buff, sizeof(buff), "请求格式异常: parseTraffic error, trafKey: %s ", trafKey.c_str());
		MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCustomTraf, parseTraffic error, trafKey: %s", param.log.c_str(), trafKey.c_str());
		basePlan->m_error.Set(50000, buff);
		delete trafItem;
		trafItem = NULL;
		return NULL;
	}

	trafItem->_mode = TrafficItem::UserTrafMode2InnerDefine(jTraffic["type"].asInt());
	if (param.type == "s205"
		||	param.type == "s128")
		trafItem->_mode = jTraffic["type"].asInt();

	return trafItem;
}
int ReqParser::DoParseP202(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	{
		basePlan->m_reqMode = req["mode"].asInt();
		basePlan->m_pageIndex = req["page"].asInt();
		if (req.isMember("pageCnt")) {
			basePlan->m_numEachPage = req["pageCnt"].asInt();
		} else {
			basePlan->m_numEachPage = 20;
		}
	}

	if (ret == 0) {
		ret = ParseCid(param, req, basePlan);
	}

	if (ret == 0) {
		ret = ParseFilter(param, req, basePlan);
	}

	return ret;
}
int ReqParser::DoParseP201(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;
	ret = ParseCid(param, req, basePlan);
	return ret;
}

int ReqParser::DoParseP104(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = 0;

	if (ret == 0) {
		basePlan->m_reqMode = LY_PLACE_TYPE_TOURALL;
		basePlan->m_pageIndex = req["page"].asInt();
		if (req.isMember("pageCnt")) {
			basePlan->m_numEachPage = req["pageCnt"].asInt();
		} else {
			basePlan->m_numEachPage = 20;
		}
		basePlan->m_listDate = req["date"].asString();
	}
	if (ret) return ret;
	ret = ParseCid(param, req, basePlan);
	if (ret) return ret;

	ret = ParseFilterP104(param, req, basePlan);
	return ret;
}

int ReqParser::DoParseP105(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	int ret = ParseDetailList(param, req, basePlan);
	//默认添加一个城市,保持数据的一致性,无实际意义
	basePlan->m_City = dynamic_cast<const City*>(GetLYPlace(basePlan, "10001", LY_PLACE_TYPE_CITY, param, basePlan->m_error));

	return ret;
}

int ReqParser::ParseDetailList(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	for (int i = 0; i < req["list"].size(); ++i) {
		int mode = req["list"][i]["mode"].asInt();
		POIDetail poi;
		std::string id = req["list"][i]["id"].asString();
		poi.m_sDate = req["list"][i]["date"]["from"].asString();
		poi.m_eDate = req["list"][i]["date"]["to"].asString();
		poi.m_place = GetLYPlace(basePlan, id, LY_PLACE_TYPE_TOURALL, param, basePlan->m_error);
		basePlan->m_POIList.push_back(poi);
	}
	return 0;
}

int ReqParser::DoParseB116(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	ParseProductIDList(param, req, basePlan);
	//默认添加一个城市,保持数据的一致性,无实际意义
	basePlan->m_City = dynamic_cast<const City*>(GetLYPlace(basePlan, "10001", LY_PLACE_TYPE_CITY, param, basePlan->m_error));

	return 0;
}

int ReqParser::ParseProductIDList(const QueryParam& param, Json::Value& req, BasePlan* basePlan) {
	for (int i = 0; i < req["wanle"].size(); ++i) {
		ProductTicket productTicket;
		if (req["wanle"][i].isMember("source") && req["wanle"][i]["source"].isObject()
				&& req["wanle"][i]["source"].isMember("ticketId") && req["wanle"][i]["source"]["ticketId"].isInt()) {
			productTicket.ticketId = std::to_string(req["wanle"][i]["source"]["ticketId"].asInt());
		}
		if (req["wanle"][i].isMember("date") && req["wanle"][i]["date"].isString()) {
			productTicket.date = req["wanle"][i]["date"].asString();
		}
		productTicket.productId = req["wanle"][i]["product_id"].asString();
		basePlan->m_productList.push_back(productTicket);
	}
	return 0;
}

int ReqParser::NewCity2OldCity(const QueryParam& param, Json::Value& req) {
	Json::Value jCity;
	jCity["cid"] = req["cid"];
	jCity["arv_poi"] = req["arv_poi"];
	jCity["dept_poi"] = req["dept_poi"];
	jCity["arv_time"] = req["arv_time"];
	jCity["dept_time"] = req["dept_time"];
	jCity["use17Limit"] = req["use17Limit"];
	jCity["ppMode"] = req["ppMode"];
	if (req.isMember("checkin")) {
		jCity["checkin"] = req["checkin"];
	}
	if (req.isMember("checkout")) {
		jCity["checkout"] = req["checkout"];
	}
	if (req.isMember("arvCustom")) {
		jCity["arvCustom"] = req["arvCustom"];
	}
	if (req.isMember("deptCustom")) {
		jCity["deptCustom"] = req["deptCustom"];
	}
	jCity["hotel"] = req["hotel"];
	if (req.isMember("traffic_in")) {
		jCity["traffic_in"] = req["traffic_in"];
	}
	req["city"] = jCity;

	return 0;
}

int ReqParser::CompleteHotel(const QueryParam& param, Json::Value& req, BasePlan* basePlan, int routeIdx) {
	if (basePlan->m_RouteBlockList.empty() || !basePlan->m_City) return 1;

	char buff[1024];
	std::string cityID = basePlan->m_City->_ID;
	RouteBlock* route_block = basePlan->m_RouteBlockList[routeIdx];
	std::sort(route_block->_hInfo_list.begin(), route_block->_hInfo_list.end(), hInfoCmp());

	std::string dayStart = route_block->_checkIn;
	std::string dayEnd = route_block->_checkOut;
	std::cerr<<"hanyong dayStart "<<dayStart<<" "<<dayEnd<<std::endl;
	if (dayStart == "" || dayEnd == "") {
		//城市间指出不需要酒店
		MJ::PrintInfo::PrintLog("[%s]ReqParser::CompleteHotel, city %s(%s) don't need hotel", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
		return 0;
	}
	//检查Hotel的checkIn和checkOut，不合理者删除
	std::string lastCheckOut = dayStart;
	for (std::vector<HInfo*>::iterator it = route_block->_hInfo_list.begin(); it != route_block->_hInfo_list.end();) {
		HInfo* hInfo = *it;
		if (hInfo->m_checkOut <  hInfo->m_checkIn) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, city %s(%s) del hotel %s cause checkOut < checkIn", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str(), hInfo->m_hotel->_name.c_str());
			it = route_block->_hInfo_list.erase(it);
			delete hInfo;
			hInfo = NULL;
			continue;
		}
		if (hInfo->m_checkOut < dayStart || dayEnd < hInfo->m_checkIn) { //删除不在日期范围内的酒店
			MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, city %s(%s) del hotel %s cause not in date range", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str(), hInfo->m_hotel->_name.c_str());
			it = route_block->_hInfo_list.erase(it);
			delete hInfo;
			hInfo = NULL;
			continue;
		}
		if (hInfo->m_checkIn < dayStart) {
			hInfo->m_checkIn = dayStart;
		} else if (hInfo->m_checkOut > dayEnd) {
			hInfo->m_checkOut = dayEnd;
		}
		if (hInfo->m_checkIn < lastCheckOut) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, city %s(%s) del hotel %s cause overlap with its former hotel", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str(), hInfo->m_hotel->_name.c_str());
			it = route_block->_hInfo_list.erase(it);
			delete hInfo;
			hInfo = NULL;
			continue;
		}

		lastCheckOut = hInfo->m_checkOut;
		++it;
	}
	//补coreHotel
	if (route_block->_hInfo_list.empty()) {
		const LYPlace* hotel = LYConstData::GetCoreHotel(cityID);
		if (!hotel) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, no coreHotel for city %s(%s)\n", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
			return 1;
		}
		//hInfo
		HInfo* hInfo = new HInfo();
		hInfo->m_checkIn = dayStart;
		hInfo->m_checkOut = dayEnd;
		hInfo->m_hotel = hotel;
		hInfo->m_isCoreHotel = true;
		route_block->_hInfo_list.push_back(hInfo);
		return 0;
	}
	//中间是否需要补全
	std::vector<HInfo*> hInfo_list = route_block->_hInfo_list;
	for (int j = 0; j < hInfo_list.size(); ++j) {
		HInfo* hInfo = hInfo_list[j];
		if (hInfo->m_checkIn > dayStart) {
			//补coreHotel
			const LYPlace* hotel = LYConstData::GetCoreHotel(cityID);
			if (!hotel) {
				MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, no coreHotel for city %s(%s)\n", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
				return 1;
			}
			//cHInfo
			HInfo* cHInfo = new HInfo();
			cHInfo->m_checkIn = dayStart;
			cHInfo->m_checkOut = hInfo->m_checkIn;
			cHInfo->m_hotel = hotel;
			cHInfo->m_isCoreHotel = true;
			route_block->_hInfo_list.push_back(cHInfo);
			dayStart = cHInfo->m_checkIn;
		}
		dayStart = hInfo->m_checkOut;
	}
	//最后是否需要补全
	if (dayStart < dayEnd) {
		//补coreHotel
		const LYPlace* hotel = LYConstData::GetCoreHotel(cityID);
		if (!hotel) {
			MJ::PrintInfo::PrintErr("[%s]ReqParser::CompleteHotel, no coreHotel for city %s(%s)\n", basePlan->m_qParam.log.c_str(), cityID.c_str(), basePlan->m_City->_name.c_str());
			return 1;
		}
		route_block->_hotel_list.push_back(hotel);
		//hInfo
		HInfo* hInfo = new HInfo();
		hInfo->m_checkIn = dayStart;
		hInfo->m_checkOut = dayEnd;
		hInfo->m_hotel = hotel;
		hInfo->m_isCoreHotel = true;
		route_block->_hInfo_list.push_back(hInfo);
		dayStart = dayEnd;
	}
	return 0;
}

int ReqParser::ParseCustomHotel(const QueryParam& qParam, Json::Value& jHotel, BasePlan* basePlan, const std::string& hid) {
	if (jHotel.isMember("custom") && jHotel["custom"].isInt() && jHotel["custom"].asInt() == POI_CUSTOM_MODE_CUSTOM) {
		//常规 coreHotel 补坐标
		std::string coord = LYConstData::GetCoreHotelCoord(basePlan->m_City->_ID);
		//有就用传过来的
		if (jHotel.isMember("coord") && jHotel["coord"].isString()) {
			coord = jHotel["coord"].asString();
		}
		if (coord == "") {
			MJ::PrintInfo::PrintLog("[%s]ReqParser::ParseCity, lost coreHotel and coord is Null  for city %s(%s)", basePlan->m_qParam.log.c_str(), basePlan->m_City->_ID.c_str(), basePlan->m_City->_name.c_str());
		}
		std::string name = jHotel["name"].asString();
		std::string lname = jHotel["lname"].asString();
		if(name.empty()) name = lname;
		if(lname.empty()) lname = name;
		const LYPlace* hotel = basePlan->SetCustomPlace(LY_PLACE_TYPE_HOTEL, hid, name, lname, coord, POI_CUSTOM_MODE_CUSTOM);
		if (!hotel) {
			char buff[1000];
			snprintf(buff, sizeof(buff), "make hotel %s(%s) failed for city %s(%s)", hid.c_str(), name.c_str(), basePlan->m_City->_ID.c_str(), basePlan->m_City->_name.c_str());
			MJ::PrintInfo::PrintErr("[%s]ReqParser::ParseCity, %s", basePlan->m_qParam.log.c_str(), buff);
			basePlan->m_error.Set(53111, "构造自定义酒店失败");
			return 1;
		}
	}
	return 0;
}

int ReqParser::RemoveTogoPoi(BasePlan *basePlan) {
	if (basePlan->m_isPlan == 0) {
		basePlan->m_userMustPlaceSet.clear();
	}
	return 0;
}

//解决出发到达点,租车点,玩乐的客观冲突
int ReqParser::DealClashOfSpecialPois(BasePlan* basePlan, std::vector<PlaceOrder>& orderList, std::tr1::unordered_set<const LYPlace*>& delPoisSet) {
	if (orderList.size() == 1) return 0;

	for (int i = 0; i < orderList.size(); i++) {
		const LYPlace* place = orderList[i]._place;
		MJ::PrintInfo::PrintLog("[%s]ReqParser::DealClashOfSpecialPois, check order %d : %s(%s)",basePlan->m_qParam.log.c_str(), i, orderList[i]._place->_ID.c_str(), orderList[i]._place->_name.c_str());
		std::string date = "";
		if (i > 0) {
			date = orderList[i-1]._date;
		} else {
			date = orderList[0]._date;
		}
		if (date != orderList[i]._date) {//开始新一天的景点
			date = orderList[i]._date;
		}
		const int len = 1024;
		char buf[len] = {0};
		snprintf(buf, len, "%s_00:00", date.c_str());
		time_t dayTime = MJ::MyTime::toTime(buf, basePlan->m_TimeZone);

		std::tr1::unordered_map<std::string, int>::iterator durIt = basePlan->m_UserDurMap.find(place->_ID);
		std::tr1::unordered_map<std::string, std::pair<int, int> >::iterator timeIt = basePlan->m_vPlaceOpenMap.find(place->_ID);
		int dur = 0;
		if (timeIt != basePlan->m_vPlaceOpenMap.end()) {
			std::pair<int, int> timePair = timeIt->second;
			time_t start = dayTime + timePair.first;
			time_t end = dayTime + timePair.second;
			//逆序
			std::vector<PlaceOrder> inversePlaceOrderList;
			//重叠
			std::vector<PlaceOrder> overlapPlaceOrderList;
			for (int j = 0; j < i; j++) {
				if (date != orderList[j]._date) continue;
				PlaceOrder lastOrder = orderList[j];
				if (delPoisSet.find(lastOrder._place) != delPoisSet.end()) continue;
				std::tr1::unordered_map<std::string, std::pair<int, int> >::iterator lastTimeIt = basePlan->m_vPlaceOpenMap.find(lastOrder._place->_ID);
				if (lastTimeIt != basePlan->m_vPlaceOpenMap.end()) {
					std::pair<int, int> lastTimePair = lastTimeIt->second;
					time_t lastStart = dayTime + lastTimePair.first;
					time_t lastEnd = dayTime + lastTimePair.second;
					if (lastStart >= end) {
						inversePlaceOrderList.push_back(lastOrder);
					}
					if (lastStart < end && start < lastEnd) {
						overlapPlaceOrderList.push_back(lastOrder);
					}
				}
			}
			if (!inversePlaceOrderList.empty()) {
				if (place->_t & LY_PLACE_TYPE_TOURALL) {
					DelClashPois(basePlan, orderList[i], delPoisSet);
				} else {
					for (int k = inversePlaceOrderList.size() - 1; k >= 0; k--) {
						PlaceOrder placeOrder = inversePlaceOrderList[k];
						const LYPlace* iPlace = inversePlaceOrderList[k]._place;
						std::vector<PlaceOrder> tmpPlaceOrder;
						if (iPlace->_t & LY_PLACE_TYPE_TOURALL) {
							DelClashPois(basePlan, placeOrder, delPoisSet);
							for (int j = 0; j <= i; j ++) {
								if (orderList[j] != inversePlaceOrderList[k]) tmpPlaceOrder.push_back(orderList[j]);
							}
							DealClashOfSpecialPois(basePlan, tmpPlaceOrder, delPoisSet);
						} else {
							if (place->_rawPlace != NULL) {
								DelClashPois(basePlan, orderList[i], delPoisSet);
							}
							if (iPlace->_rawPlace != NULL && place->_t & LY_PLACE_TYPE_ARRIVE) {
								DelClashPois(basePlan, placeOrder, delPoisSet);
							}
						}
					}
				}
			}
			//重叠
			if (!overlapPlaceOrderList.empty()) {
				if (place->_t & LY_PLACE_TYPE_TOURALL) {
					DelClashPois(basePlan, orderList[i], delPoisSet);
				} else {
					for (int k = overlapPlaceOrderList.size() - 1; k >= 0; k-- ) {
						PlaceOrder placeOrder = overlapPlaceOrderList[k];
						const LYPlace* iPlace = overlapPlaceOrderList[k]._place;
						std::vector<PlaceOrder> tmpPlaceOrder;
						if (iPlace->_t & LY_PLACE_TYPE_TOURALL) {
							DelClashPois(basePlan, placeOrder, delPoisSet);
							for (int j = 0; j <= i; j ++) {
								if (orderList[j] != overlapPlaceOrderList[k]) tmpPlaceOrder.push_back(orderList[j]);
							}
							DealClashOfSpecialPois(basePlan, tmpPlaceOrder, delPoisSet);
						} else {
							if (place->_rawPlace != NULL) {
								DelClashPois(basePlan, orderList[i], delPoisSet);
							}
							if (iPlace->_rawPlace != NULL && place->_t & LY_PLACE_TYPE_ARRIVE) {
								DelClashPois(basePlan, placeOrder, delPoisSet);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int ReqParser::DelClashPois(BasePlan* basePlan, const PlaceOrder placeOrder, std::tr1::unordered_set<const LYPlace*>& delPoisSet) {
	const LYPlace* place = placeOrder._place;
	if (basePlan->m_userMustPlaceSet.count(place)) {
		basePlan->m_userMustPlaceSet.erase(place);
		basePlan->m_PlaceDateMap.erase(place->_ID);
	}
	basePlan->m_UserDurMap.erase(place->_ID);
	basePlan->m_vPlaceOpenMap.erase(place->_ID);
	if (delPoisSet.find(place) == delPoisSet.end()) {
		delPoisSet.insert(place);
		basePlan->m_userDelSet.insert(place);
	}
	return 0;
}
bool ReqParser::IsBaoChe(BasePlan* basePlan, Json::Value& req) {
	if (req.isMember("traffic_in") && req["traffic_in"].isMember("detail") &&
			req["traffic_in"]["detail"].isArray() && !req["traffic_in"]["detail"].empty() &&
			req["traffic_in"]["detail"][0].isMember("mode") && req["traffic_in"]["detail"][0]["mode"].isString() &&
			(req["traffic_in"]["detail"][0]["mode"].asString() == "baoche" || req["traffic_in"]["detail"][0]["mode"].asString() == "zuche")) {
		for (int i = 0; i < req["traffic_in"]["detail"].size();i++) {
			Json::Value& jDetail = req["traffic_in"]["detail"][i];
			if (jDetail.isMember("days") && jDetail["days"].isArray()) {
				for (int j = 0; j < jDetail["days"].size(); j++) {
					if (jDetail["days"][j].isMember("date") && jDetail["days"][j]["date"].isString() && jDetail["days"][j]["date"].asString().length()==8) {
						std::string date = jDetail["days"][j]["date"].asString();
						basePlan->m_date2trafPrefer[date] = TRAF_PREFER_TAXI;
					}
				}
			}
		}
		return true;
	}
	return false;
}


