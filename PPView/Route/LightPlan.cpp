#include "Planner.h"
#include "LightPlan.h"


int LightPlan::PlanS126(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error) {
	int ret = 0;
	MJ::PrintInfo::PrintDbg("[%s]LightPlan::PlanS126", param.log.c_str());
	//0 检验请求
	ret = ReqChecker::DoCheck(param, req, error);
	if (ret) return 1;

	ChangeViewAboutCarStore(req);

	//1 获取新的请求与旧响应
	ret = GetReqRespInfo(param, req, error);
	if (ret) return 1;
	//2 检查所做的改变
	ret = CheckChanges(param, req, threadPool);
	if (ret) return 1;

	//3 据新请求更改行程
	for (std::tr1::unordered_map<int, CityInfo>::iterator cIt = m_cityInfoMap.begin(); cIt != m_cityInfoMap.end(); ++cIt) {
		int cIdx = cIt->first;
		CityInfo& cityInfo = cIt->second;
		int changeType = cityInfo.m_changeType;
		Json::Value jCityResp;
		if (changeType) {
			PlanCity(param, req, jCityResp, threadPool, error, cIdx, cityInfo);
		}
		if (jCityResp.isMember("data") && jCityResp["data"].isMember("city") ) {
			resp["data"]["list"][cIdx]["view"] = jCityResp["data"]["city"]["view"];
		}
	}
	//4 对增加的天或空白天进行规划
	Json::FastWriter jw;
	std::cerr<<"hyhy before DoGroupPlan: "<<std::endl<<jw.write(resp)<<std::endl;
	DoGroupPlan(param, req, resp, threadPool, error);

    //5 后续处理，保证expire
    Json::Value newResp = resp;
    BasePlan *plan = new BasePlan;
    plan->m_qParam = param;
    PostProcessor::PostProcess(req, plan, newResp);
    resp = newResp;
	if (plan) {
		delete plan;
		plan = NULL;
	}
	return ret;
}

int LightPlan::PlanCity(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error, int cIdx, CityInfo& cityInfo) {
	MJ::PrintInfo::PrintDbg("[%s]LightPlan::PlanCity", param.log.c_str());

	int changeType = cityInfo.m_changeType;
	/*依改变类型做调整*/
	if (changeType & CHANGE_NEW_CITY) {
		//新需求会在后面分组规划
		//DoGroupPlan
	} else {
		GetReqDateList(cityInfo);
		RefreshRoute(param, req, resp, error, cIdx, cityInfo);
	}

	return 0;
}

int LightPlan::CheckChanges(const QueryParam& param, Json::Value& req, MyThreadPool* threadPool) {
	MJ::PrintInfo::PrintDbg("[%s]LightPlan::CheckChanges", param.log.c_str());
	for (std::tr1::unordered_map<int, CityInfo>::iterator cIt = m_cityInfoMap.begin(); cIt != m_cityInfoMap.end(); ++cIt) {
		int cIdx = cIt->first;
		CityInfo& cityInfo = cIt->second;
		int changeType = CHANGE_NULL;
		//0 判断是否为新增城市
		bool isNewCity = false;
		isNewCity = IsNewCity(param, req, cIdx);
		if (isNewCity) {
			changeType |= CHANGE_NEW_CITY;
		} else {
			//参考行程存在的城市一定平移
			changeType |= CHANGE_NEED_MOVE_ROUTE;
			//1	检查hotel的改变
			GetHotelChangeType(param, cIdx, cityInfo, changeType);
			//1 判断城市间交通时间的变化 以及有否将某个城市的城市内交通由非包车改为包车
			GetCityTrafTimeChangeType(param, req, cIdx, cityInfo, changeType);
		}
		if(HasCarStore(req,cIdx)) changeType |= CHANGE_HAS_CAR_STORE;
		if (changeType == CHANGE_NULL) {
			MJ::PrintInfo::PrintDbg("[%s]LightPlan::CheckChanges, cIdx %d, nothing change", param.log.c_str(), cIdx);
		}
		cityInfo.m_changeType = changeType;
	}
	return 0;
}

int LightPlan::GetReqRespInfo(const QueryParam& param, Json::Value& req, ErrorInfo& error) {
	int ret = 0;
	char buf[1024];
	if (req.isMember("cityPreferCommon") && req["cityPreferCommon"].isMember("timeRange") && req["cityPreferCommon"]["timeRange"].isObject()) m_preferTimeRange = req["cityPreferCommon"]["timeRange"];
	for (int i = 0; i < req["list"].size(); ++i) {
		CityInfo cityInfo;
		//1	解析请求中的相关字段
		//timeZone
		Json::Value route = req["list"][i];
		if (route.isMember("ridx") && route["ridx"].isInt()) {
			int ridx = route["ridx"].asInt();
			if (m_cityDaysTranslate.find(ridx) != m_cityDaysTranslate.end()) {
				cityInfo.m_translate = m_cityDaysTranslate[ridx];
			}
		}
		ErrorInfo errInfo;
		std::string cid = route["cid"].asString();
		const City* city = dynamic_cast<const City*>(ReqParser::GetLYPlace(cid, LY_PLACE_TYPE_CITY, param, errInfo));
		if (city == NULL) {
			snprintf(buf, sizeof(buf), "请求格式异常：route[%d] 中cid error", i);
			MJ::PrintInfo::PrintDbg("[%s]LightPlan::GetReqRespInfo, cid error in route[%d]", param.log.c_str(), i);
			error.m_errID = 51001;
			error.m_errReason = buf;
			return 1;
		}
		cityInfo.m_cid = city->_ID;
		cityInfo.m_timeZone = city->_time_zone;
		//checkin checkout
		if (!route["checkin"].isNull()) cityInfo.m_qCheckInDate = route["checkin"].asString();
		if (!route["checkout"].isNull()) cityInfo.m_qCheckOutDate = route["checkout"].asString();
		//ArvPoi
		if (!route["arv_poi"].isNull()) cityInfo.m_qArvPoiInfo.m_id = route["arv_poi"].asString();
		cityInfo.m_qArvPoiInfo.m_arvTime = route["arv_time"].asString();
		//DeptPoi
		if (!route["dept_poi"].isNull()) cityInfo.m_qDeptPoiInfo.m_id = route["dept_poi"].asString();
		cityInfo.m_qDeptPoiInfo.m_deptTime = route["dept_time"].asString();
		//Hotels
		if (route.isMember("hotel") && !route["hotel"].empty()) {
			for (int j = 0; j < route["hotel"].size(); ++j) {
				if (route["hotel"][j].isMember("product_id") && route["hotel"][j]["product_id"].isString()) {
					std::string hotel_id = route["hotel"][j]["product_id"].asString();
					if (req.isMember("product") && req["product"].isObject() &&
						req["product"].isMember("hotel") && req["product"]["hotel"].isObject() &&
						req["product"]["hotel"].isMember(hotel_id) && req["product"]["hotel"][hotel_id].isObject()) {
						Json::Value jHotel = req["product"]["hotel"][hotel_id];
						HInfo hInfo;
						hInfo.m_id= jHotel["id"].asString();
						hInfo.m_checkIn = jHotel["checkin"].asString().substr(0, 8);
						hInfo.m_checkOut = jHotel["checkout"].asString().substr(0, 8);
						cityInfo.m_qHInfoList.push_back(hInfo);
					}
				}
			}
		}
		//traffic_in
		if (route.isMember("traffic_in")) {
			cityInfo.m_qTrafficIn = route["traffic_in"];
		}
		cityInfo.m_ridx = -1;
		if(route.isMember("ridx") and route["ridx"].isInt()) cityInfo.m_ridx = route["ridx"].asInt();
		//2	解析原response中的相应字段
		//126 允许view为null
		if (route["originView"].isNull() || route["originView"]["day"].size() < 1) {
			m_cityInfoMap.insert(std::make_pair(i, cityInfo));
			continue;
		}
		//ArvPoi
		int firstDayViewNum = route["originView"]["day"][0u]["view"].size();
		if (firstDayViewNum > 0) {
			const Json::Value& oriFirstPoi = route["originView"]["day"][0u]["view"][0u];
				if (route["originView"].isMember("summary") && route["originView"]["summary"].isMember("arv_time")) {
					cityInfo.m_rArvPoiInfo.m_arvTime = route["originView"]["summary"]["arv_time"].asString();
				} else {
					cityInfo.m_rArvPoiInfo.m_arvTime = oriFirstPoi["stime"].asString();
				}
				bool oriFirstPoiIsArv = false;
				if (oriFirstPoi["stime"].asString() == cityInfo.m_rArvPoiInfo.m_arvTime) {
					oriFirstPoiIsArv = true;
				}
				if (oriFirstPoi.isMember("func_type") and oriFirstPoi["func_type"].isInt() and oriFirstPoi["func_type"].asInt() == NODE_FUNC_KEY_ARRIVE) {
					oriFirstPoiIsArv = true;
				}
			int rArvType = oriFirstPoi["type"].asInt();
			if (rArvType & LY_PLACE_TYPE_ARRIVE) {
				cityInfo.m_rArvPoiInfo.m_id = oriFirstPoi["id"].asString();
				if (cityInfo.m_qArvPoiInfo.m_id == cityInfo.m_rArvPoiInfo.m_id and oriFirstPoiIsArv) {
					const Json::Value& firstPoi = route["originView"]["day"][0u]["view"][0u];
					if (firstPoi.isMember("dur") && firstPoi["dur"].isInt()) {
						cityInfo.m_qArvPoiInfo.m_dur = firstPoi["dur"].asInt();
					}
					if (firstPoi.isMember("play") && firstPoi["play"].isString()) {
						cityInfo.m_qArvPoiInfo.m_play = firstPoi["play"].asString();
					}
				}
			}
		}
		//DeptPoi
		int dayNum = route["originView"]["day"].size();
		int lastDayViewNum = route["originView"]["day"][dayNum - 1]["view"].size();
		if (lastDayViewNum > 0) {
			const Json::Value& oriLastPoi = route["originView"]["day"][dayNum - 1]["view"][lastDayViewNum - 1];
			if (route["originView"].isMember("summary") && route["originView"]["summary"].isMember("dept_time")) {
				cityInfo.m_rDeptPoiInfo.m_deptTime = route["originView"]["summary"]["dept_time"].asString();
			} else {
				cityInfo.m_rDeptPoiInfo.m_deptTime = oriLastPoi["etime"].asString();
			}
			bool oriLastPoiIsDept = false;
			if (oriLastPoi["etime"] == cityInfo.m_rDeptPoiInfo.m_deptTime) {
				oriLastPoiIsDept = true;
			}
			if (oriLastPoi.isMember("func_type") and oriLastPoi["func_type"].isInt() and oriLastPoi["func_type"].asInt() == NODE_FUNC_KEY_DEPART) {
				oriLastPoiIsDept = true;
			}
			int rDeptType = oriLastPoi["type"].asInt();
			if (rDeptType & LY_PLACE_TYPE_ARRIVE) {
				cityInfo.m_rDeptPoiInfo.m_id = oriLastPoi["id"].asString();
				//保留原行程中自定义的玩法和时长
				if (cityInfo.m_rDeptPoiInfo.m_id == cityInfo.m_qDeptPoiInfo.m_id and oriLastPoiIsDept) {
					const Json::Value& lastPoi = route["originView"]["day"][dayNum - 1]["view"][lastDayViewNum - 1];
					if (lastPoi.isMember("dur") && lastPoi["dur"].isInt()) {
						cityInfo.m_qDeptPoiInfo.m_dur = lastPoi["dur"].asInt();
					}
					if (lastPoi.isMember("play") && lastPoi["play"].isString()) {
						cityInfo.m_qDeptPoiInfo.m_play = lastPoi["play"].asString();
					}
				}
			}
		}
		//checkin checkout
		if (route["originView"].isMember("checkin") && route["originView"]["checkin"].isString()) {
			cityInfo.m_rCheckInDate = route["originView"]["checkin"].asString();
		} else {
			cityInfo.m_rCheckInDate = cityInfo.m_rArvPoiInfo.m_arvTime.substr(0,8);
		}
		if (route["originView"].isMember("checkout") && route["originView"]["checkout"].isString()) {
			cityInfo.m_rCheckOutDate = route["originView"]["checkout"].asString();
		} else {
			cityInfo.m_rCheckOutDate = cityInfo.m_rDeptPoiInfo.m_deptTime.substr(0,8);
		}
		//Hotels
		if (route["originView"].isMember("summary") && route["originView"]["summary"].isMember("hotel") &&
				!route["originView"]["summary"]["hotel"].empty()) {
			Json::Value& hotels = route["originView"]["summary"]["hotel"];
			for (int j = 0; j < hotels.size(); ++j) {
				HInfo hInfo;
				hInfo.m_id= hotels[j]["id"].asString();
				hInfo.m_checkIn = hotels[j]["checkin"].asString().substr(0, 8);
				hInfo.m_checkOut = hotels[j]["checkout"].asString().substr(0, 8);
				cityInfo.m_rHInfoList.push_back(hInfo);
			}
		}
		else if (route["originView"]["summary"]["hotel"].empty()) {
			int dayViewNum = route["originView"]["day"][0u]["view"].size();
			if (dayViewNum > 0) {
				Json::Value jHotel = route["originView"]["day"][0u]["view"][dayViewNum-1];
				cityInfo.m_rHotelId = "";
				if (jHotel.isMember("id") and jHotel["id"].isString() and jHotel.isMember("type") and jHotel["type"].isInt() and jHotel["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
					cityInfo.m_rHotelId = jHotel["id"].asString();
				}
			}
		}
		//traffic_in
		if (route["originView"].isMember("summary") && route["originView"]["summary"].isMember("traffic_in")) {
			cityInfo.m_rTrafficIn = route["originView"]["summary"]["traffic_in"];
		}
		m_cityInfoMap.insert(std::make_pair(i, cityInfo));
	}
	return ret;
}

int LightPlan::MakeReqSSV006(const Json::Value& req, Json::Value& jReq, QueryParam& queryParam, int cIdx, CityInfo& cityInfo) {
	MJ::PrintInfo::PrintDbg("[%s]LightPlan::MakeReqSSV006", queryParam.log.c_str());
	int ret = 0;
	//1	构造queryParam
	queryParam.type = "ssv006_light";
	//2 构造req的基本成员
	jReq["product"]=req["product"];
	jReq["cityPreferCommon"]=req["cityPreferCommon"];
	jReq["ridx"] = 0;
	if (!req.isMember("list") || req["list"].size() <= cIdx) {
		MJ::PrintInfo::PrintErr("[%s]LightPlan::MakeReqSSV006, cIdx %d is bigger than list's size", queryParam.log.c_str(), cIdx);
		return 1;
	}
	jReq["city"] = req["list"][cIdx];

	if (jReq.isMember("cityPreferCommon")) jReq["cityPreferCommon"] = jReq["cityPreferCommon"];
	//3 构造days，不在请求给出的日期范围的天，其景点数组大小置0
	ret = MakeJDays(jReq, cityInfo);
	if (ret) return 1;
	//4 jCustomPoiList
	//自定义点通过days传入
	Json::FastWriter jw;
	std::cerr<<"hyhy 006 jw\n"<<jw.write(jReq)<<std::endl;
	return 0;
}

//依据请求的起始日期得到dateList
int LightPlan::GetReqDateList(CityInfo& cityInfo) {
	cityInfo.m_qDateList.clear();
	std::string startDate = cityInfo.m_qArvPoiInfo.m_arvTime.substr(0, 8);
	std::string endDate = cityInfo.m_qDeptPoiInfo.m_deptTime.substr(0, 8);
	if(cityInfo.m_qCheckInDate != "" && cityInfo.m_qCheckOutDate != "") {
		startDate = cityInfo.m_qCheckInDate;
		endDate = cityInfo.m_qCheckOutDate;
	} else {
		//不住酒店时只有一天，用arvDate
		endDate = startDate;
	}
	for (std::string date = startDate; date <= endDate; date = MJ::MyTime::datePlusOf(date,1)) {
		cityInfo.m_qDateList.push_back(date);
	}

	return 0;
}

int LightPlan::GetHotelChangeType(const QueryParam& param, int cIdx, const CityInfo& cityInfo, int& changeType) {
	bool isHotelChange = false;
	const std::vector<HInfo>& qHotelInfoList = cityInfo.m_qHInfoList;
	const std::vector<HInfo>& rHotelInfoList = cityInfo.m_rHInfoList;
	if (qHotelInfoList.empty() && !rHotelInfoList.empty()) {
		//删除酒店
		changeType |= CHANGE_HOTEL_NULL;
		isHotelChange = true;
	} else if (qHotelInfoList.size() != rHotelInfoList.size()) {
		//酒店数目变化
		changeType |= CHANGE_HOTEL_NUM;
		isHotelChange = true;
	} else if (qHotelInfoList.size() == rHotelInfoList.size()) {
		if (rHotelInfoList.empty()) {
			if (cityInfo.m_rHotelId.find("coreHotel") == std::string::npos) {
				changeType |= CHANGE_HOTEL_COREHOTEL_ID;
				isHotelChange =true;
			}
		}
		for (int qH = 0; qH < qHotelInfoList.size(); qH++) {
			for (int rH = 0; rH < rHotelInfoList.size(); rH++) {
				HInfo qHInfo = qHotelInfoList[qH];
				HInfo rHInfo = rHotelInfoList[rH];
				if (qHInfo.m_id != rHInfo.m_id)  { //酒店顺序变化 或者是换酒店
					changeType |= CHANGE_HOTEL_REPLACE;
					isHotelChange = true;
					break;
				} else if (qHInfo.m_checkIn.substr(0, 8) != rHInfo.m_checkIn.substr(0, 8) || qHInfo.m_checkOut.substr(0, 8) != rHInfo.m_checkOut.substr(0, 8)) {
					changeType |= CHANGE_HOTEL_DATE;
					isHotelChange = true;
					break;
				}
			}
			if (isHotelChange) break;
		}
	}
	if (isHotelChange) {
		MJ::PrintInfo::PrintDbg("[%s]LightPlan::GetHotelChangeType, cIdx %d, Hotel Change %x", param.log.c_str(), cIdx, changeType);
	}
	return 0;
}

int LightPlan::getNewHotelId(const CityInfo& cityInfo, std::string checkInDate, std::string& hotelId) {
	const std::vector<HInfo>& qHotelInfoList = cityInfo.m_qHInfoList;
	for (int i = 0; i < qHotelInfoList.size(); i++) {
		const HInfo& qHotel = qHotelInfoList[i];
		if (i==0 && checkInDate < qHotel.m_checkIn) {
			hotelId = qHotel.m_id;
			return 0;
		}
		if (checkInDate >= qHotel.m_checkIn && checkInDate < qHotel.m_checkOut) {
			hotelId = qHotel.m_id;
			return 0;
		}
	}
	//拿不到酒店时，传市中心酒店
	const LYPlace* hotel = LYConstData::GetCoreHotel(cityInfo.m_cid);
	hotelId = hotel->_ID;
	return 0;
}

int LightPlan::GetCityTrafTimeChangeType(const QueryParam& param, Json::Value& req, int cIdx, const CityInfo& cityInfo, int& changeType) {
	const PoiInfo& qArvPoiInfo = cityInfo.m_qArvPoiInfo;
	const PoiInfo& rArvPoiInfo = cityInfo.m_rArvPoiInfo;
	const PoiInfo& qDeptPoiInfo = cityInfo.m_qDeptPoiInfo;
	const PoiInfo& rDeptPoiInfo = cityInfo.m_rDeptPoiInfo;

	double timeZone = cityInfo.m_timeZone;
	//day0Time
	time_t qArvDay0Time = MJ::MyTime::toTime(qArvPoiInfo.m_arvTime.substr(0, 8), timeZone);
	time_t rArvDay0Time = MJ::MyTime::toTime(rArvPoiInfo.m_arvTime.substr(0, 8), timeZone);
	time_t qDeptDay0Time = MJ::MyTime::toTime(qDeptPoiInfo.m_deptTime.substr(0, 8), timeZone);
	time_t rDeptDay0Time = MJ::MyTime::toTime(rDeptPoiInfo.m_deptTime.substr(0, 8), timeZone);
	if (cityInfo.m_qCheckInDate.length()==8 && cityInfo.m_qCheckOutDate.length()==8
			&& cityInfo.m_rCheckInDate.length()==8 && cityInfo.m_rCheckOutDate.length()==8)
	{
		qArvDay0Time = MJ::MyTime::toTime(cityInfo.m_qCheckInDate, timeZone);
		rArvDay0Time = MJ::MyTime::toTime(cityInfo.m_rCheckInDate, timeZone);
		qDeptDay0Time = MJ::MyTime::toTime(cityInfo.m_qCheckOutDate, timeZone);
		rDeptDay0Time = MJ::MyTime::toTime(cityInfo.m_rCheckOutDate, timeZone);
	}
	//天数变化 仅天数增加需要被记录
	if ((qDeptDay0Time - qArvDay0Time) - (rDeptDay0Time - rArvDay0Time) >= 24 * 3600) {
		changeType |= CHANGE_ADD_DAYS;
		MJ::PrintInfo::PrintDbg("[%s]LightPlan::GetCityTrafTimeChangeType, cIdx %d, ADD_DAYS, CHANGE_NEED_MOVE_ROUTE", param.log.c_str(), cIdx);
	}
	return 0;
}

int LightPlan::ChangeViewAboutCarStore(Json::Value& req)
{
	//存储请求中每个城市和orginView中起始日期的偏移量;
	//如果一个city的偏移量并没有被填充过,则根据key取出的偏移值为0,恰恰合意
	//暂不考虑一个城市出现多次
	for (int i = 0; i < req["list"].size(); ++i) {
		if(not req["list"][i]["ridx"].isInt()) continue;
		long long llridx  = req["list"][i]["ridx"].asInt();
		//std::string ridx = std::to_string(llridx);
		Json::Value originView = req["list"][i]["originView"];
		if(not (originView.isObject()
					and originView["summary"].isObject()
					and originView["summary"]["days"].isArray()
					and originView["summary"]["days"].size()>0
					and originView["summary"]["days"][0]["date"].isString()
					and originView["summary"]["days"][0]["date"].asString().length()==8)
		  ) continue;
		if(not (req["list"][i]["arv_time"].isString()
					and req["list"][i]["arv_time"].asString().length() == 14)
		  ) continue;
		std::string oldDate = originView["summary"]["arv_time"].asString().substr(0,8);
		std::string newDate = req["list"][i]["arv_time"].asString().substr(0,8);
		if (originView["summary"]["days"].size() > 0 && originView["summary"]["days"][0u]["date"].isString()) oldDate = originView["summary"]["days"][0u]["date"].asString();
		if (req["list"][i]["checkin"].asString() != "") newDate = req["list"][i]["checkin"].asString();
		//将新旧城市都当做零时区,算相差天数没有问题
		m_cityDaysTranslate[llridx]= (MJ::MyTime::toTime(oldDate)-MJ::MyTime::toTime(newDate))/(24*3600);
	}
	for(auto it= m_cityDaysTranslate.begin(); it!= m_cityDaysTranslate.end();it++)
	{
		_INFO("daysTranslate : %d->%d",it->first,it->second);
	}

	int _getCar = 0;
	int _returnCar = 1;

	std::tr1::unordered_map<std::string, std::pair<int, int>> hotelSleepTimeRange;
	//获取每日酒店睡觉范围 date为入住日期
	{
		for (int i = 0; i < req["list"].size(); ++i) {
			Json::Value& originView = req["list"][i]["originView"];
			if(not originView.isObject()) continue;
			for(int j=0; j< originView["day"].size()-1; j++)
			{
				if(not (originView["day"][j]["date"].isString())) continue;

				Json::Value& jReqDayView = originView["day"][j]["view"];
				Json::Value tReqDayView = Json::arrayValue;
				std::string date = originView["day"][j]["date"].asString();
				std::string hotelKey = req["list"][i]["cid"].asString()+"|"+date;
				if (jReqDayView.size()>0)
				{
					int k = jReqDayView.size() - 1;
					if(jReqDayView[k]["type"].asInt() & LY_PLACE_TYPE_HOTEL)
					{
						if(not jReqDayView[k]["id"].isString()) continue;
						int stime = MJ::MyTime::toTime(jReqDayView[k]["stime"].asString());
						int etime = MJ::MyTime::toTime(jReqDayView[k]["etime"].asString());
						hotelSleepTimeRange[hotelKey] = std::make_pair(stime, etime);
						_INFO("%s: hotelSleepRange: %s - %s",hotelKey.c_str(), jReqDayView[k]["stime"].asString().c_str(), jReqDayView[k]["etime"].asString().c_str());
					}
				}
			}
		}
	}
	//解析product中的租车数据
	std::tr1::unordered_map<std::string,std::tr1::unordered_map<std::string,Json::Value> > carStoreInfos;//存储解析到的租车数据
	if(req.isMember("product") and req["product"].isObject() and req["product"].isMember("zuche") and req["product"]["zuche"].isObject())
	{
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
			_map.insert(std::make_pair("start",_getCar));
			_map.insert(std::make_pair("end",_returnCar));
			for(auto it = _map.begin(); it!= _map.end(); it++)
			{
				if(oneCityCarRental[it->first].isObject()
						and oneCityCarRental[it->first]["ridx"].isInt())
				{
					long long llridx = oneCityCarRental[it->first]["ridx"].asInt();
					std::string ridx = std::to_string(llridx);
					std::string cid = oneCityCarRental[it->first]["cid"].asString();
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
							std::string date= zuche[it->first]["date"].asString();
							int iid=0;
							std::string zucheId = it->first + "#" + zuche[it->first]["coord"].asString() + "#" + zuche[it->first]["name"].asString();
							MJ::md5Last4(zucheId,iid);
							//MJ::md5Last4(zuche["unionkey"].asString(),iid);
							std::string id = "cs"+std::to_string(iid+it->second);
							std::string coord = zuche[it->first]["coord"].asString();

							//根据originView将date平移;
							//假如originview中游玩日期从a开始,而新请求中游玩日期从a1开始,则date_new = date+a-a1
							std::string originDate = date;
							date = MJ::MyTime::datePlusOf(date,m_cityDaysTranslate[llridx]);

							Json::Value daysPoi;
							daysPoi["id"] = id;
							daysPoi["type"] = LY_PLACE_TYPE_CAR_STORE;
							daysPoi["stime"] = date+"_"+zuche[it->first]["time"].asString();
							daysPoi["pdur"] = 3600;

							Json::Value dayView;
							dayView["id"] = id;
							dayView["originDate"]=originDate;
							dayView["custom"] = POI_CUSTOM_MODE_MAKE;
							dayView["type"] = LY_PLACE_TYPE_CAR_STORE;
							dayView["name"] = name;
							dayView["lname"] = name;
							dayView["coord"] = coord;
							dayView["do_what"] = it->second == _getCar?"取车":"还车";
							Json::Value info;
							info["mode"] = it->second;
							if(zuche["corp"].isObject() and zuche["corp"]["name"].isString())
							{
								info["corp"] = zuche["corp"]["name"].asString();
							}
							dayView["info"] = info;
							dayView["close"]=0;
							dayView["dur"] = 3600;
							dayView["stime"] = date+"_"+zuche[it->first]["time"].asString();
							//时区不影响加一个小时的计算
							time_t tTime = MJ::MyTime::toTime(dayView["stime"].asString());
							dayView["etime"] = MJ::MyTime::toString(tTime+3600);
							Json::Value product;
							if(oneCityCarRental["product_id"].isString())
							{
								product["product_id"]= oneCityCarRental["product_id"].asString();
							}
							if(zuche["unionkey"].isString())
							{
								product["unionkey"] =zuche["unionkey"].asString();
							}
							dayView["product"]=product;

							//前端要求必须传
							dayView["idle"]=Json::nullValue;
							dayView["traffic"]=Json::nullValue;

							Json::Value carStoreInfo;
							carStoreInfo["daysPoi"] = daysPoi;
							carStoreInfo["dayView"] = dayView;
							carStoreInfo["mode"]=it->second;

							//特殊逻辑  根据租车时间判断租车点应放入的day序
							{
								std::string stime = dayView["stime"].asString();
								//< 6:00 的租车点，按照离 入住/离开时间点 更近的原则放入
								if (stime.substr(9,5) >= "00:00" && stime.substr(9,5) <= "06:00") {
									std::string checkInDate = MJ::MyTime::datePlusOf(date, -1);
									std::string hotelKey = cid + "|" + checkInDate;
									auto it = hotelSleepTimeRange.find(hotelKey);
									if (it != hotelSleepTimeRange.end()) {
										int sleepStime = it->second.first;
										int sleepEtime = it->second.second;
										if ((sleepStime < sleepEtime and tTime-sleepStime < sleepEtime-tTime)
											   or (sleepStime > sleepEtime) and tTime < sleepEtime){
											date = checkInDate;
										}
									}
								}
							}

							std::string key=ridx+"|"+date;
							std::string idKey = id + "|" + zuche["unionkey"].asString();
							carStoreInfos[key][idKey] = carStoreInfo;
							_INFO("get carstore,mode:%s->%s:%d",key.c_str(),id.c_str(),it->second);
						}
					}
				}
			}
		}
	}

	std::tr1::unordered_set<std::string> originZucheKey;
	//删除原view中的租车数据
	{
		for (int i = 0; i < req["list"].size(); ++i) {
			Json::Value& originView = req["list"][i]["originView"];
			if(not originView.isObject()) continue;
			if(not req["list"][i]["ridx"].isInt()) continue;
			Json::Value& summary = originView["summary"];
			if(not (summary.isObject() and summary["days"].isArray())) continue;
			long long llridx = req["list"][i]["ridx"].asInt();
			std::string ridx= std::to_string(llridx);
			for(int j=0; j< summary["days"].size(); j++)
			{
				if(not (summary["days"][j]["date"].isString())) continue;

				Json::Value& jReqDayView = originView["day"][j]["view"];
				Json::Value& jReqDayPois = summary["days"][j]["pois"];
				Json::Value tReqDayView = Json::arrayValue;
				Json::Value tReqDayPois = Json::arrayValue;
				std::string zucheKey = ridx+"|"+summary["days"][j]["date"].asString();
				for(int k=0; k<jReqDayView.size(); k++)
				{
					if(jReqDayView[k]["type"].asInt() & LY_PLACE_TYPE_CAR_STORE)
					{
						if(not jReqDayView[k]["id"].isString()) continue;
						if(not jReqDayView[k].isMember("product") and not jReqDayView[k]["product"].isMember("unionkey") and not jReqDayView[k]["product"]["unionkey"].isString()) continue;
						std::string stime = jReqDayView[k]["stime"].asString();
						std::string idKey = jReqDayView[k]["id"].asString() + "|" + jReqDayView[k]["product"]["unionkey"].asString();
						std::string oriZuche = zucheKey + "|" + idKey;
						if(carStoreInfos.find(zucheKey) != carStoreInfos.end()
								and carStoreInfos[zucheKey].find(idKey) != carStoreInfos[zucheKey].end())
						{
							carStoreInfos[zucheKey][idKey]["daysPoi"]["pdur"] = jReqDayView[k]["dur"];
							carStoreInfos[zucheKey][idKey]["dayView"]["dur"] = jReqDayView[k]["dur"];
							originZucheKey.insert(oriZuche);
						}
						_INFO("delete carstore,id:%s",idKey.c_str());
					}
					else
					{
						tReqDayView.append(jReqDayView[k]);
						tReqDayPois.append(jReqDayPois[k]);
					}
				}
				jReqDayView = tReqDayView;
				jReqDayPois = tReqDayPois;
			}
		}
	}

	//添加新的租车数据进入view
	//按照景点的时间确定租车点插入的位置,租车门店左右的点有冲突时直接删掉
	{
		for (int i = 0; i < req["list"].size(); ++i) {
			Json::Value & originView = req["list"][i]["originView"];
			if(not originView.isObject()) continue;
			Json::Value & summary = originView["summary"];
			if(not (summary.isObject() and summary["days"].isArray())) continue;
			long long llridx = req["list"][i]["ridx"].asInt();
			std::string ridx= std::to_string(llridx);
			std::string arvTime = summary["arv_time"].asString();
			std::string deptTime = summary["dept_time"].asString();
			for(int j=0; j< summary["days"].size(); j++)
			{
				if(not (summary["days"][j]["date"].isString())) continue;
				std::string zucheKey = ridx+"|"+summary["days"][j]["date"].asString();
				if(carStoreInfos.find(zucheKey) == carStoreInfos.end()) continue;
				Json::Value& jReqDayView = originView["day"][j]["view"];
				Json::Value& jReqDayPois = summary["days"][j]["pois"];
				if(jReqDayView.size()!=jReqDayPois.size())
				{
					_INFO("originView format wrong,view and pois size not equal");
					continue;
				}
				bool isFirstPoiArv=false, isLastPoiDept = false;
				if (jReqDayView.size()>0) {
					Json::Value& jFirstPoi = jReqDayView[0u];
					if (jFirstPoi["stime"].asString() == arvTime and jFirstPoi["type"].asInt() & LY_PLACE_TYPE_ARRIVE) {
						isFirstPoiArv = true;
					}
					Json::Value& jLastPoi = jReqDayView[jReqDayView.size()-1];
					if (jLastPoi["etime"].asString() == deptTime and jLastPoi["type"].asInt() & LY_PLACE_TYPE_ARRIVE) {
						isLastPoiDept = true;
					}
				}

				for(auto it=carStoreInfos[zucheKey].begin(); it != carStoreInfos[zucheKey].end(); )
				{
					std::string stime=it->second["dayView"]["stime"].asString();
					std::string etime=it->second["dayView"]["etime"].asString();
					if(stime <=arvTime or stime>=deptTime)
					{
						it++;
						continue;
					}

					{
						//确定租车点添加的位置
						int vPlace = 0;
						for(int k=0; k<jReqDayView.size(); k++)
						{
							if(jReqDayView[k]["stime"].asString() < stime) vPlace=k+1;
							else break;
						}

						if(vPlace == 0 and vPlace < jReqDayView.size())
						{
							if (isFirstPoiArv) vPlace = 1;
							//非第一天，租车都应该在酒店离开点之后
							if (jReqDayView[0u]["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
								if (j != 0) vPlace = 1;
							}
						}
						else if(jReqDayView.size() > 0 and vPlace == jReqDayView.size())
						{
							int endIdx = jReqDayView.size()-1;
							//离开天租车应该放在站点之前
							if (isLastPoiDept) vPlace = endIdx;
							//非最后一天，租车都应该放在休息酒店之前
							if(jReqDayView[endIdx]["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
								if (j != summary["days"].size()-1) vPlace=endIdx;
							}
						}

						Json::Value tReqDayView=Json::arrayValue;
						Json::Value tReqDayPois=Json::arrayValue;
						for(int p=0; p<vPlace and p < jReqDayView.size(); p++)
						{
							tReqDayView.append(jReqDayView[p]);
							tReqDayPois.append(jReqDayPois[p]);
						}
						//原行程中没有的租车点，可能需要前推酒店
						std::string stime = it->second["daysPoi"]["stime"].asString();
						std::string id = it->second["daysPoi"]["id"].asString();
						std::string oriZuche = zucheKey + "|" + id + "|" + it->second["dayView"]["product"]["unionkey"].asString();
						if (originZucheKey.find(oriZuche) == originZucheKey.end() && j != 0 && vPlace == 1) {
							it->second["dayView"]["needAdjustHotel"] = 1;
						}
						tReqDayView.append(it->second["dayView"]);
						tReqDayPois.append(it->second["daysPoi"]);
						it=carStoreInfos[zucheKey].erase(it);
						_INFO("add carstore, %s, day view idx: %d", zucheKey.c_str(), vPlace);
						for(int p=vPlace; p<jReqDayView.size(); p++)
						{
							tReqDayView.append(jReqDayView[p]);
							tReqDayPois.append(jReqDayPois[p]);
						}
						jReqDayView = tReqDayView;
						jReqDayPois = tReqDayPois;
					}
				}
			}
		}
		m_missCarStores = carStoreInfos;
	}

	//修改适配新版平移
	{
		for (int i = 0; i < req["list"].size(); ++i) {
			Json::Value & originView = req["list"][i]["originView"];
			if(not originView.isObject()) continue;
			Json::Value & summary = originView["summary"];
             summary["days"] = Json::arrayValue;
             PostProcessor::MakejDays(req, originView["day"], summary["days"]);
		}
	}

	return 0;
}

int LightPlan::RefreshRoute(const QueryParam& param, Json::Value& req, Json::Value& resp, ErrorInfo& error, int cIdx, CityInfo& cityInfo) {
	MJ::PrintInfo::PrintDbg("[%s]LightPlan::RefreshRoute, cIdx %d", param.log.c_str(), cIdx);
	int ret = 0;
	Json::Value jReq;
	QueryParam queryParam = param;
	Json::FastWriter jw;
	/*打006*/
	ret = MakeReqSSV006(req, jReq, queryParam, cIdx, cityInfo);
	if (ret) {
		MJ::PrintInfo::PrintDbg("[%s]LightPlan::RefreshRoute, failed to makequery Idx %d", param.log.c_str(), cIdx);
		return 1;
	}
	ret = Planner::DoPlanSSV006(queryParam, jReq, resp);
	if (ret || !resp.isMember("data") || !resp["data"].isMember("city") || !resp["data"]["city"].isMember("view")) {
		MJ::PrintInfo::PrintDbg("[%s]LightPlan::RefreshRoute, failed to plan cIdx %d", param.log.c_str(), cIdx);
		return 1;
	}
	std::cerr<<"hyhy res 006  final cIdx "<<cIdx<<"\n"<<jw.write(resp)<<std::endl;
	return 0;
}

int LightPlan::MakeJDays(Json::Value& jReq, CityInfo& cityInfo) {
	//填充days[]-pois[]
	Json::Value jDaysList = Json::arrayValue;
	const Json::Value& jOriginView = jReq["city"]["originView"];
	const Json::Value& jOriginDays = jOriginView["summary"]["days"];
	std::string startDate = cityInfo.m_qDateList.front();
	std::string endDate = cityInfo.m_qDateList.back();

	std::string hotelStime = "19:00";
	std::string hotelEtime = "09:00";
	if (m_preferTimeRange.isMember("to") && m_preferTimeRange["to"].isString()) hotelStime = m_preferTimeRange["to"].asString();
	if (m_preferTimeRange.isMember("from") && m_preferTimeRange["from"].isString()) hotelEtime = m_preferTimeRange["from"].asString();
	int hotelStimeInt = 0, hotelEtimeInt = 0;
	ToolFunc::toIntOffset(hotelEtime, hotelEtimeInt);
	ToolFunc::toIntOffset(hotelStime, hotelStimeInt);
	int hotelpdur = hotelEtimeInt - hotelStimeInt + 24*3600;

	bool hasReclaimLuggage = false;
	int didx = 0;
	for (std::string date = startDate; date <= endDate;date = MJ::MyTime::datePlusOf(date, 1)){
		/*
		 * 通过startIdx 和 endIdx 控制删除原days中的arv dept poi
		 * 如果酒店变化，在构造days过程中换酒店id
		 * date 用来控制新行程的日期范围
		 * didx为原行程的天数范围
		 * 通过m_translate修改stime 避免跨天日期给错
		 */
		//判断酒店是否变化的flag
		bool isHotelChange = false;
		Json::Value jPoiList = Json::arrayValue;
		if (didx < jOriginDays.size()) {
			int startIdx = 0;
			if (didx == 0) {
				if (cityInfo.m_qArvPoiInfo.m_id != "") {
					Json::Value poi;
					poi["id"] = cityInfo.m_qArvPoiInfo.m_id;
					poi["pdur"] = cityInfo.m_qArvPoiInfo.m_dur;
					poi["stime"] = cityInfo.m_qArvPoiInfo.m_arvTime;
					poi["play"] = cityInfo.m_qArvPoiInfo.m_play;
					poi["type"] = LY_PLACE_TYPE_ARRIVE;
					poi["func_type"] = NODE_FUNC_KEY_ARRIVE;
					jPoiList.append(poi);
				}
				if (cityInfo.m_rArvPoiInfo.m_id != "") {
					startIdx = 1;
				}
			}

			int endIdx = -1;
			if (jOriginDays[didx]["pois"].size()) endIdx = jOriginDays[didx]["pois"].size()-1;
			if (didx+1 == jOriginDays.size()) {
				//原行程最后一天删除站点
				if (cityInfo.m_rDeptPoiInfo.m_id != "" && jOriginDays[didx]["pois"].size()>0) {
					endIdx --;
				}
				//原行程最后一天如果有行李点(紧挨站点的非睡觉酒店endIdx>0) 删除
				if (endIdx > 0 && endIdx < jOriginDays[didx]["pois"].size() && jOriginDays[didx]["pois"][endIdx]["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
					hasReclaimLuggage = true;
					endIdx --;
				}
			}
			else if(didx+1 < jOriginDays.size()) {
				//新行程最后一天删除原行程晚上的酒店 (该天必须非原行程的最后一天)
				if (date == endDate and endIdx >= 0) {
					const Json::Value& jLastPoi = jOriginDays[didx]["pois"][endIdx];
					if (jLastPoi["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
						endIdx --;
					}
				}
			}

			// 原行程当天离开酒店 oriDeptHotelId
			std::string oriDeptHotelId = "";
			for (int i = startIdx; i <= endIdx ; i++) {
				Json::Value jPoi = jOriginDays[didx]["pois"][i];
				if (jPoi.isMember("stime") && jPoi["stime"].asString().length() == 14) {
					std::string oriDate = jPoi["stime"].asString().substr(0,8);
					std::string newDate = MJ::MyTime::datePlusOf(oriDate, 0-cityInfo.m_translate);
					jPoi["stime"] = newDate + jPoi["stime"].asString().substr(8);
				}
				if (jPoi.isMember("etime") && jPoi["etime"].asString().length() == 14) {
					std::string oriDate = jPoi["etime"].asString().substr(0,8);
					std::string newDate = MJ::MyTime::datePlusOf(oriDate, 0-cityInfo.m_translate);
					jPoi["etime"] = newDate + jPoi["etime"].asString().substr(8);
				}
				if (cityInfo.m_changeType & CHANGE_HOTEL && jPoi["type"].isInt() && jPoi["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
					// 出发酒店checkindate为前一天
					// 首天无出发酒店,所有酒店均为day0入住酒店
					std::string hotelId = "";
					std::string checkInDate = date;
					if (date != startDate and i==0) {
						checkInDate = MJ::MyTime::datePlusOf(date, -1);
						oriDeptHotelId = jPoi["id"].asString();
					}
					getNewHotelId(cityInfo, checkInDate, hotelId);
					if (hotelId != "") {
						/*
						 * 非原行程最后一天，修改酒店，使用原行程酒店时间
						 * 原行程最后一天为新行程中间天时，需新增一个酒店 ***
						 */
						Json::Value newHotel = Json::Value();
						newHotel["id"] = hotelId;
						newHotel["stime"] = jPoi["stime"];
						newHotel["etime"] = jPoi["etime"];
						newHotel["pdur"] = jPoi["pdur"];
						newHotel["type"] = jPoi["type"];
						newHotel["func_type"] = NODE_FUNC_KEY_HOTEL_SLEEP;
						jPoi = newHotel;
					}
					if (hotelId != oriDeptHotelId) isHotelChange = true;
				}
				if (jPoi.isMember("needAdjustHotel") and isHotelChange) jPoi.removeMember("needAdjustHotel");
				jPoiList.append(jPoi);
			}
			//原行程最后一天增加一个酒店 ***
			if (didx+1 == jOriginDays.size() && date!=endDate) {
				std::string hotelId = "";
				getNewHotelId(cityInfo, date, hotelId);
				Json::Value jHotel;
				jHotel["id"] = hotelId;
				jHotel["pdur"] = hotelpdur;
				jHotel["stime"] = date + "_" + hotelStime;
				jHotel["etime"] = MJ::MyTime::datePlusOf(date,1) + "_" + hotelEtime;
				jHotel["type"] = LY_PLACE_TYPE_HOTEL;
				jHotel["func_type"] = NODE_FUNC_KEY_HOTEL_SLEEP;
				jPoiList.append(jHotel);
			}
		}
		//兼容城市内规划失败 传入空数组
		if (startDate != endDate
				and ((date == startDate and cityInfo.m_qArvPoiInfo.m_id != "" and jPoiList.size() == 1) //首天到达点非null size 为1
				or jPoiList.size() == 0)) {
			cityInfo.m_addDateSet.insert(date);
			//每天加早晚酒店
			for (int i = 0; i < 2; i++) {
				Json::Value jHotel = Json::Value();
				std::string hotelId = "";
				std::string checkInDate = "";
				if (date != startDate and i == 0) {
					checkInDate = MJ::MyTime::datePlusOf(date, -1);
				}else{
					checkInDate = date;
				}
				getNewHotelId(cityInfo, checkInDate, hotelId);
				jHotel["id"] = hotelId;
				jHotel["pdur"] = hotelpdur;
				jHotel["stime"] = checkInDate + "_" + hotelStime;
				jHotel["etime"] = MJ::MyTime::datePlusOf(checkInDate,1)+ "_" + hotelEtime;
				jHotel["type"] = LY_PLACE_TYPE_HOTEL;
				jHotel["func_type"] = NODE_FUNC_KEY_HOTEL_SLEEP;
				jPoiList.append(jHotel);
				//首尾天只加一个酒店
				if (date == startDate or date == endDate) break;
			}
		}

		if (date == endDate) {
			if (hasReclaimLuggage && didx+1 == jOriginDays.size()) {
				std::string hotelId = "";
				std::string checkInDate = MJ::MyTime::datePlusOf(date, -1);
				getNewHotelId(cityInfo, checkInDate, hotelId);
				Json::Value jHotel;
				jHotel["id"] = hotelId;
				jHotel["pdur"] = 1800;
				jHotel["type"] = LY_PLACE_TYPE_HOTEL;
				jHotel["func_type"] = NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE;
				jPoiList.append(jHotel);
			}
			if (cityInfo.m_qDeptPoiInfo.m_id != "") {
				Json::Value poi;
				poi["id"] = cityInfo.m_qDeptPoiInfo.m_id;
				poi["pdur"] = cityInfo.m_qDeptPoiInfo.m_dur;
				poi["play"] = cityInfo.m_qDeptPoiInfo.m_play;
				poi["func_type"] = NODE_FUNC_KEY_DEPART;
				int etime = MJ::MyTime::toTime(cityInfo.m_qDeptPoiInfo.m_deptTime);
				std::string stime = MJ::MyTime::toString(etime - poi["pdur"].asInt());
				poi["stime"] = stime;
				//交通站点可能前推酒店
				//如果仅修改酒店，不往前推酒店时间
				if (!isHotelChange) poi["needAdjustHotel"] = 1;
				jPoiList.append(poi);
			}
		}
		Json::Value jDay;
		jDay["date"] = date;
		jDay["pois"] = jPoiList;
		jDaysList.append(jDay);

		didx ++;
	}
	Json::FastWriter jw;
	std::cerr<<"lidw jDaysList: "<<jw.write(jDaysList)<<std::endl;
	jReq["days"] = jDaysList;
	return 0;
}

int LightPlan::DoGroupPlan(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error) {
	using namespace std;
	using namespace std::tr1;
	unordered_map<string, vector<int> > cidsMap;
	unordered_map<int, Json::Value> ridx2JsonMap;

	Json::Value jTrafficPass = Json::arrayValue;
	if (req.isMember("list")) {
		for (int i = 0; i < req["list"].size(); ++ i) {
			if (req["list"][i].isMember("traffic_pass") && req["list"][i]["traffic_pass"].isMember("summary") &&
					req["list"][i]["traffic_pass"]["summary"].isMember("days")) {
				for (int j = 0; j < req["list"][i]["traffic_pass"]["summary"]["days"].size(); ++ j) {
					jTrafficPass.append(req["list"][i]["traffic_pass"]["summary"]["days"][j]);
				}
			}
		}
	}
	Json::Value jTourCity = Json::arrayValue;
	if (req["product"].isMember("tour")) {
		Json::Value& jtourProduct = req["product"]["tour"];
		Json::Value& tourCity = req["tourCity"];
		Json::Value::Members jMemList = tourCity.getMemberNames();
		for (Json::Value::Members::iterator it = jMemList.begin(); it != jMemList.end(); it++) {
			std::string productId = *it;
			if (!jtourProduct.isMember(productId)) {
				_INFO("err productId: %s", productId.c_str());
			}
			Json::Value& jRoute = jtourProduct[productId]["route"];
			Json::Value& jIdxList = tourCity[*it];
			for (int i = 0; i < jIdxList.size(); i++) {
				int idx = jIdxList[i].asInt();
				if (idx < jRoute.size()) {
					Json::Value& jCity = jRoute[idx];
					Json::Value& jDays = jCity["view"]["summary"]["days"];
					for (int j = 0; j < jDays.size(); j++) {
						jTourCity.append(jDays[j]);
					}
				}
			}
		}
	}


	/*分组*/
	for (int i = 0; i < req["list"].size(); ++i) {
		int changeType = CHANGE_NULL;
		unordered_map<int, CityInfo>::iterator cIt = m_cityInfoMap.find(i);
		if (cIt != m_cityInfoMap.end()) {
			changeType = cIt->second.m_changeType;
		} else {
			MJ::PrintInfo::PrintErr("[%s]LightPlan::DoGroupPlan, cann't find cityInfo for cIdx %d", param.log.c_str(), i);
			return 1;
		}
		string cid = req["list"][i]["cid"].asString();
		cidsMap[cid].push_back(i);
	}
	/*拼请求*/
	for (unordered_map<string, vector<int> >::iterator it = cidsMap.begin(); it != cidsMap.end(); ++it) {
		vector<int>& cIdxList = it->second;
		Json::Value jReq;
		jReq["product"]=req["product"];
		jReq["cityPreferCommon"]=req["cityPreferCommon"];
		jReq["list"].resize(0);
		bool needPlan = false;	//该组城市是否需要走规划
		for (int i = 0; i < cIdxList.size(); ++i) {
			int cIdx = cIdxList[i];
			unordered_map<int, CityInfo>::iterator cIt = m_cityInfoMap.find(cIdx);
			if (cIt == m_cityInfoMap.end()) {
				std::cerr<<"hyhy cann't find cIdx "<<cIdx<<std::endl;
				return 1;
			}
			jReq["list"].append(req["list"][cIdx]);
			jReq["list"][jReq["list"].size() - 1]["product"]=req["product"];
			ReqParser::NewCity2OldCity(param , jReq["list"][jReq["list"].size() - 1]);//new 和 old 是指请求格式方面
			jReq["list"][jReq["list"].size() - 1]["ridx"] = cIdx;
			//	jReq["list"][jReq["list"].size() - 1]["use17Limit"] = 1; //17点逻辑~
			jReq["list"][jReq["list"].size() - 1]["notPlanDays"] = Json::nullValue;
			if (resp.isMember("data") && resp["data"].isMember("list") && resp["data"]["list"].size() > cIdx &&
					resp["data"]["list"][cIdx].isMember("view") && resp["data"]["list"][cIdx]["view"].isMember("summary")) {
				unordered_set<string>& addDatesSet = cIt->second.m_addDateSet;
				jReq["list"][jReq["list"].size() - 1]["notPlanDays"] = Json::Value(Json::arrayValue);
				jReq["list"][jReq["list"].size() - 1]["view"] = resp["data"]["list"][cIdx]["view"];
				for (int j = 0; j < resp["data"]["list"][cIdx]["view"]["summary"]["days"].size(); ++j) {
					string date = resp["data"]["list"][cIdx]["view"]["summary"]["days"][j]["date"].asString();
					//计算该天景点数
					int dayVCnt = 0;
					Json::Value& jPois = resp["data"]["list"][cIdx]["view"]["summary"]["days"][j]["pois"];
					for (int v = 0; v < jPois.size(); ++v) {
						if (jPois[v].isMember("id")) {
							//不算首尾站点及酒店
							if (jPois[v].isMember("type")
									&& jPois[v]["type"].asInt() & (LY_PLACE_TYPE_ARRIVE | LY_PLACE_TYPE_HOTEL |LY_PLACE_TYPE_CAR_STORE)) {
								continue;
							}
							else dayVCnt++;
						}
					}
					//增加的天一定要规划 非修改行程的空白天也规划
					if ((addDatesSet.find(date) != addDatesSet.end()) || (!dayVCnt && !req.isMember("isChangeTrip"))) {
						cIt->second.m_needPlanDateSet.insert(date);
					}
					else {
						jReq["notPlanDays"].append(date);
					}
				}
			}

			//是新增城市或增加天
			if ((cIt->second.m_changeType & CHANGE_NEW_CITY)
					|| (cIt->second.m_changeType & CHANGE_ADD_DAYS)
					|| not cIt->second.m_needPlanDateSet.empty()) {
				needPlan = true;
			}
		}
		for (int i = 0; i < jReq["list"].size(); ++ i) {
			if (jReq["list"][i].isMember("notPlanDays")) {
				//途经点中已有点不再规划
				for (int j = 0; j < jTrafficPass.size(); ++ j) {
					if(jTrafficPass[j]["pois"].size() > 0) {
						jTrafficPass[j]["date"] = "";
						jReq["list"][i]["view"]["summary"]["days"].append(jTrafficPass[j]);
					}
				}
				//团游中已规划点不再规划
				for (int j = 0; j < jTourCity.size(); j ++) {
					if (jTourCity[j]["pois"].size() > 0) {
						jTourCity[j]["date"] = "";
						jReq["list"][i]["view"]["summary"]["days"].append(jTourCity[j]);
					}
				}
			}
		}
		if (!needPlan) continue;
		/*打请求*/
		Json::Value jResp;
		QueryParam queryParam = param;
		queryParam.type = "ssv005_rich";
		Json::FastWriter jw;
		std::cerr<<"hyhy cid: "<<(it->first)<<" ssv005_rich req "<<std::endl<<jw.write(jReq)<<std::endl;
		Planner::DoPlanSSV005(queryParam, jReq, jResp, threadPool);
		std::cerr<<"hyhy cid: "<<(it->first)<<" ssv005_rich resp "<<std::endl<<jw.write(jResp)<<std::endl;
		/*拼返回结果*/
		for (int i = 0; i < jResp["data"]["list"].size() && i < cIdxList.size(); i++) {
			int cIdx = cIdxList[i];
			resp["data"]["list"][cIdx] = jResp["data"]["list"][i];
		}
	}
	return 0;
}

