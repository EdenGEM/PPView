#include "DaysPlan.h"
#include "Route/PostProcessor.h"
#include "Route/base/PathPerfect.h"

const bool debugInfo=false;
//就餐时间
const std::string RESTAURANT_LUNCH_TIME = "12:00";
const std::string RESTAURANT_SUPPER_TIME = "18:00";

//不舒适时间段
const std::string UNCOMFORTABLE_BACK_HOTELTIME = "24:00";
const std::string UNCOMFORTABLE_DEPT_HOTELTIME = "06:00";

bool DaysPlan::GetPlanResp (BasePlan* basePlan, const Json::Value& req, Json::Value& resp) {
	bool ret = true;
	ret = InitDayPlan(basePlan, req);
	if (ret) ret = InitRouteJ(basePlan);
	if (ret) ret = MakeResp(basePlan, req, resp);
	return ret;
}
bool DaysPlan::PerfectWarningList (Json::Value& jWarningList) {
	Json::Value tmpWarningList = Json::arrayValue;
	int didx = 0;
	while (didx < m_dayPlan.size()) {
		for (int i = 0; i < jWarningList.size(); i++) {
			Json::Value jWarning = jWarningList[i];
			if (jWarning.isMember("didx") && jWarning["didx"].isInt() && jWarning["didx"].asInt() == didx) {
				tmpWarningList.append(jWarning);
			}
		}
		didx ++;
	}
	jWarningList = tmpWarningList;
}
bool DaysPlan::MakeResp (BasePlan* basePlan, const Json::Value& req, Json::Value& resp) {
	//jSummary  view.summary
	//jViewDayList   view.day
	Json::Value jErrorList = Json::arrayValue;
	Json::Value jViewDayList = Json::arrayValue;
	Json::Value jSummary = Json::Value();

	Json::Value jWarningList = Json::arrayValue;
	for (int i = 0; i < m_dayPlan.size(); i++) {
		_INFO("make resp...");
		Json::Value jError = Json::Value();
		Json::Value jDay = Json::Value();
		Json::Value jDayWarningList = Json::arrayValue;
		DayRoute* dayRoute = m_dayPlan[i];
		dayRoute->GetDayView(basePlan, jError, jDay, jDayWarningList);
		
		if (!jError.isNull()) {
			jErrorList.append(jError);
		}
		if (!jDay.isNull()) {
			jViewDayList.append(jDay);
		}
		if (jDayWarningList.size()>0) {
			for (int j = 0; j < jDayWarningList.size(); j++) {
				jWarningList.append(jDayWarningList[j]);
			}
		}
	}
	
	if (jErrorList.size() > 0) {
		if (basePlan->m_qParam.type == "s202" || basePlan->m_qParam.type == "s125") {
			resp["data"]["error"] = jErrorList;
			return true;
		} else if (basePlan->m_qParam.type == "s204" || basePlan->m_qParam.type == "s129") {
			resp["data"]["error"] = jErrorList;
			resp["data"]["stat"] = 1;
			return true;
		}
	} else if (basePlan->m_qParam.type == "s204" || basePlan->m_qParam.type == "s129") {
		resp["data"]["stat"] = 0;
		return true;
	}
	//智能优化时拼结果
	Json::Value jReqDayList = req["city"]["view"]["day"];
	std::tr1::unordered_set<std::string> dateSet;
	for (auto it = m_date2pois.begin(); it != m_date2pois.end(); it++) {
		std::string date = it->first;
		if(!basePlan->m_notPlanDateSet.count(date)) {
			dateSet.insert(date);
		}
	}
	jViewDayList = PostProcessor::AddResult(jReqDayList,jViewDayList,dateSet);//智能优化时拼结果(notPlanDates.size != 0时)，同时FixHotelTime
	Json::Value jView = Json::Value();
	if (jViewDayList.size() > 0) {
		PostProcessor::AddTrafficWarning(basePlan, jViewDayList, jWarningList);
		PerfectWarningList(jWarningList);
		MakejSummary(basePlan, req, jWarningList, jViewDayList, jSummary);

		PostProcessor::AddDoWhat(jViewDayList, basePlan);
		PathPerfect::RichAttach(basePlan, jViewDayList);

		jView["day"] = jViewDayList;
		jView["summary"] = jSummary;
		jView["expire"] = hasExpire(m_dayPlan.size());
		if (basePlan->m_qParam.type == "s202" || basePlan->m_qParam.type == "s125") {
			resp["data"]["view"] = jView;
		} else if (basePlan->m_qParam.type == "ssv006_light") {
			resp["data"]["city"] = req["city"];
			resp["data"]["city"]["view"] = jView;
		}
	}
	if (basePlan->m_qParam.type == "s131") {
		resp = Json::Value();
		bool hasError = false;
		if (jErrorList.size()>0) {
			Json::Value& jErr = jErrorList[0u];
			if (jErr["didx"] == 0) {
				if (jErr.isMember("tips") and jErr["tips"].isArray()) {
					for (int i = 0; i < jErr["tips"].size(); i++) {
						if (jErr["tips"][i].isMember("type") and jErr["tips"][i]["type"] != 6) {
							hasError = true;
						}
					}
				}
			}
		}
		if (hasError) {
			ChangeConflict2TourErr(req, jWarningList, resp);
			return true;
		}
		if (req.isMember("needView")) resp["data"]["view"] = jView;
		resp["data"]["type"] = 0;
		std::string etime = "";
		if (req.isMember("needEtime") and req["needEtime"].isInt() and req["needEtime"].asInt()) {
			if (jViewDayList.size() == 1) {
				Json::Value jDay = jViewDayList[0u];
				if (jDay.isMember("view") and jDay["view"].isArray() and jDay["view"].size()>0) {
					Json::Value jView = jDay["view"][jDay["view"].size()-1];
					if (jView.isMember("etime") and jView["etime"].isString() and jView["etime"].asString().length() == 14) {
						etime = jView["etime"].asString().substr(9);
					}
				}
			}
		}
		resp["data"]["petime"] = etime;
	}

	return true;
}
//不包含warning
bool DaysPlan::MakejSummary (BasePlan* basePlan, const Json::Value& req, const Json::Value& jWarningList,Json::Value& jDayList, Json::Value& jSummary) {
	jSummary["days"] = Json::arrayValue;
	//jDaysList -- view.summary.days
	//jDays -- view.summary.days[i]
	//jDayList -- view.day
	PostProcessor::MakeJSummary(basePlan, req, jWarningList, jDayList, jSummary);
	Json::Value& jDays = jSummary["days"];
	for(int i = 0; i < jDays.size(); i++) {
		Json::Value& jDay = jDays[i];
		jDay["expire"] = hasExpire(i);
	}
	return true;
}

bool DaysPlan::InitDayPlan (BasePlan* basePlan, const Json::Value& req) {
	const Json::Value& jDays = req["days"];
	m_cityID = req["city"]["cid"].asString();
	std::tr1::unordered_set<std::string> trafKeySet;
	for(int i = 0; i < jDays.size(); i ++) {
		if (basePlan->m_qParam.type == "s131" and !req.isMember("needView") and i>0) break;
		if (basePlan->m_notPlanDateSet.count(jDays[i]["date"].asString())) {
			continue;
		}
		const Json::Value& jDaysPoisList = jDays[i]["pois"];
		std::string date = jDays[i]["date"].asString();
		int left = 0, right = 0;
		Json::FastWriter fw;
		std::cerr << "date: " << date << std::endl << fw.write(jDaysPoisList) << std::endl;
		const Json::Value& jFirstPoi = jDaysPoisList[0u];
		int pdur = 0;
		if (jFirstPoi.isMember("pdur") && jFirstPoi["pdur"].isInt()) pdur = jFirstPoi["pdur"].asInt();
		if (!jFirstPoi.isMember("stime") || !jFirstPoi["stime"].isString() || ToolFunc::FormatChecker::CheckTime(jFirstPoi["stime"].asString()) != 0) {
			_INFO("DaysPlan::InitDayPlan, days %d pois 0 has error", i);
			return false;
		}
		left = MJ::MyTime::toTime(jFirstPoi["stime"].asString(), basePlan->m_TimeZone)+pdur;
		const Json::Value& lastPoi = jDaysPoisList[jDaysPoisList.size()-1];
		if (!lastPoi.isMember("stime") || !lastPoi["stime"].isString() || ToolFunc::FormatChecker::CheckTime(lastPoi["stime"].asString())!=0) {
			_INFO("DaysPlan::InitDayPlan, days %d last poi has error", i);
			return false;
		}
		pdur = 0;
		if (lastPoi.isMember("pdur") && lastPoi["pdur"].isInt()) pdur = lastPoi["pdur"].asInt();
		right = MJ::MyTime::toTime(lastPoi["stime"].asString(), basePlan->m_TimeZone) + pdur;
		if (lastPoi.isMember("etime")) {
			right = MJ::MyTime::toTime(lastPoi["etime"].asString(), basePlan->m_TimeZone);
		}
		m_date2playRange[date] = std::make_pair(left, right);
		m_date2pois[date] = jDaysPoisList;
	}
	return true;
}

//构造routeJ时 使用时刻时间戳 减去日期 (可能 <0 or >86400)
bool DaysPlan::InitRouteJ (BasePlan* basePlan) {
	int dayMax = m_date2pois.size();
	int didx = 0;
	std::vector<Json::Value> RouteJList;
	for (auto it = m_date2pois.begin(); it != m_date2pois.end(); it++) {
		//构造每日routeJ
		Json::Value& jPoisList = it->second;
		std::string date = it->first;
		Json::Value RouteJ = Json::arrayValue;
		std::pair<int, int> playRange = m_date2playRange[date];
		for (int j = 0; j < jPoisList.size(); j++) {
			Json::Value& jPoi = jPoisList[j];
			std::string id = jPoi["id"].asString();
			const LYPlace* place = basePlan->GetLYPlace(id);

			Json::Value nodeJ;
			NodeJ::setId(nodeJ, id);
			NodeJ::setDefaultError(nodeJ);
			if (jPoi.isMember("stime") && jPoi["stime"].isString() && ToolFunc::FormatChecker::CheckTime(jPoi["stime"].asString())==0 && jPoi.isMember("pdur")) {
				int stime = MJ::MyTime::toTime(jPoi["stime"].asString(), basePlan->m_TimeZone);
				int etime = stime + jPoi["pdur"].asInt();
				if ((j == 0 || j == jPoisList.size() -1)
						&& jPoi.isMember("etime") && jPoi["etime"].isString()
					   	&& jPoi.isMember("type") && jPoi["type"].isInt() && jPoi["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
					if (j+1 == jPoisList.size()) {
						//构造一个free酒店,一个fix酒店, dur均为0,交通置0, 两个点之间的time就为酒店dur
						Json::Value freeHotel = Json::Value();
						NodeJ::setId(freeHotel, id);
						NodeJ::setDefaultError(freeHotel);
						NodeJ::addOpenClose(freeHotel, playRange.first, playRange.second);
						NodeJ::setDurs(freeHotel, 0, 0, 0);
						_INFO("add freehotel id: %s, openClose: %s - %s, dur:0", id.c_str(), MJ::MyTime::toString(playRange.first,basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(playRange.second, basePlan->m_TimeZone).c_str());
						RouteJ.append(freeHotel);
					}
					etime = MJ::MyTime::toTime(jPoi["etime"].asString(), basePlan->m_TimeZone);
					NodeJ::setFixedCanDel(nodeJ, 0);
					NodeJ::addTime(nodeJ, etime, etime);
					_INFO("add fixed hotel: %s, stime: %s, etime: %s", id.c_str(), jPoi["stime"].asString().c_str(), jPoi["etime"].asString().c_str());
				} else {
					_INFO("add stime poi: %s, stime: %s, etime: %s", id.c_str(), jPoi["stime"].asString().c_str(), jPoi["etime"].asString().c_str());
					NodeJ::addTime(nodeJ, stime, etime);
					if (place->_t & LY_PLACE_TYPE_TOURALL) {
						NodeJ::setFixedCanDel(nodeJ, 1);
					} else {
						NodeJ::setFixedCanDel(nodeJ, 0);
					}
				}
			}
			else {
				int dur = 0;
				if (jPoi.isMember("pdur") && jPoi["pdur"].isInt()) dur = jPoi["pdur"].asInt();
				//需要支持多段开关门
				std::vector<std::pair<int, int>> openCloseList;
				bool setOpenClose = false;
				if (basePlan->GetAdjacentDaysOpenCloseTime(date, place, openCloseList) == 0
						&& openCloseList.size()>0) {
					for (int i = 0; i < openCloseList.size(); i++) {
						//open close playRange 均为带日期的时间戳
						time_t open = openCloseList[i].first;
						time_t close = openCloseList[i].second;
						_INFO("open:%s, close:%s, playRange: %s - %s",MJ::MyTime::toString(open,basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(close,basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(playRange.first,basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(playRange.second, basePlan->m_TimeZone).c_str());
						//游玩范围之外的开关门过滤
						if (close - dur < playRange.first || open + dur > playRange.second) continue;
						NodeJ::addOpenClose(nodeJ, open, close);
						setOpenClose = true;
						_INFO("add poi: %s, open: %s, close: %s", id.c_str(), MJ::MyTime::toString(open, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(close, basePlan->m_TimeZone).c_str());
					}
				}
				if (!setOpenClose) {
					if (place->_t == LY_PLACE_TYPE_HOTEL) {
						NodeJ::addOpenClose(nodeJ, playRange.first, playRange.second);
						_INFO("add freehotel id: %s, openClose: %s - %s", id.c_str(), MJ::MyTime::toString(playRange.first,basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(playRange.second, basePlan->m_TimeZone).c_str());
					} else {
						NodeJ::addOpenClose(nodeJ, -1, -1);
						_INFO("add poi: %s, open: 0, close: 0", id.c_str());
					}
				}
				NodeJ::setDurs(nodeJ, dur, dur, dur);
				//修改不保持时长应放在这里
				//用于智能优化不保持时长的情况 修改dur
				if(!basePlan->m_keepTime and place->_t & (LY_PLACE_TYPE_VIEW | LY_PLACE_TYPE_SHOP | LY_PLACE_TYPE_RESTAURANT)) {
					//取15分钟整数倍
					const DurS durS = basePlan->GetDurS(place);
					int zipDur = durS.m_zip/900 ? durS.m_zip/900 * 900 : 900;
					int rcmdDur = durS.m_rcmd/900 ? durS.m_rcmd/900 * 900 : 900;
					int extendDur = durS.m_extend/900 ? durS.m_extend/900 * 900 : 900;
					NodeJ::setDurs(nodeJ, zipDur, rcmdDur, extendDur);
				}
				_INFO("add poi: %s, dur: %d", id.c_str(), dur);
			}
			_INFO("date: %s, didx: %d, add node: %s",date.c_str(), didx, id.c_str());
			RouteJ.append(nodeJ);
		}
		//设置交通
		for (int j = 1; j < RouteJ.size(); j++) {
			Json::Value& lastNode = RouteJ[j-1];
			Json::Value& theNode = RouteJ[j];
			std::string id = theNode["id"].asString();
			std::string lastNodeId = lastNode["id"].asString();
			const TrafficItem* trafItem = basePlan->GetTraffic(lastNodeId, id, date);
			if (trafItem == NULL) {
				_INFO("get traffic err, from %s to %s", lastNodeId.c_str(), id.c_str());
				continue;
			}
			int trafTime = trafItem->_time;
			NodeJ::addTraffic(lastNode, id, trafTime);
			//酒店时间前推
			//仅紧挨酒店的租车点或者站点可能有needAdjustHotel字段
			//lastNodeNeedStime 前推酒店需要的离开时间点
			//优先满足最后一天前推酒店时间 即满足酒店离开时间
			if (didx > 0 && j == 1 && jPoisList.size() > 1) {
				Json::Value& lastPoi = jPoisList[0u];
				Json::Value& jPoi = jPoisList[1];
				if (lastPoi["type"].asInt() == LY_PLACE_TYPE_HOTEL && jPoi.isMember("needAdjustHotel") && jPoi["needAdjustHotel"].isInt() && jPoi["needAdjustHotel"].asInt() && jPoi["stime"].isString()) {
					int stime = MJ::MyTime::toTime(jPoi["stime"].asString(), basePlan->m_TimeZone);
					int lastNodeNeedStime = stime - trafTime;
					if (lastNodeNeedStime < lastNode["fixed"]["times"][0u][0u].asInt()) { //times[0][0] == times[0][1] 
						//同时更新前一天RouteJ的右边界
						Json::Value& lastRouteJ = RouteJList[didx-1];
						Json::Value& lastDaystheLastPoi = lastRouteJ[lastRouteJ.size()-1];
						NodeJ::delTimes(lastDaystheLastPoi);
						NodeJ::addTime(lastDaystheLastPoi, lastNodeNeedStime, lastNodeNeedStime);

						NodeJ::delTimes(lastNode);
						NodeJ::addTime(lastNode, lastNodeNeedStime, lastNodeNeedStime);
					}
				}
			}
			if (j+1 == RouteJ.size()) {
				NodeJ::setEndFlag(theNode);
			}
		}
		RouteJList.push_back(RouteJ);
		didx ++;
	}
	didx = 0;
	for (auto it = m_date2pois.begin(); it != m_date2pois.end(); it++) {
		Json::Value& RouteJ = RouteJList[didx];
		std::string date = it->first;
		Json::FastWriter fw;
		std::cerr << "didx:" << didx << " date:" << date << std::endl << fw.write(RouteJ) << std::endl;
		Json::Value controls = Json::Value();
		if (basePlan->m_keepTime) controls["keepTime"] = 1;
		DayRoute* dayRoute = new DayRoute(didx, date, RouteJ, dayMax, controls);
		m_dayPlan.push_back(dayRoute);
		didx ++;
	}
	return true;
}
//增加error和warning
bool DayRoute::getErrorAndWarnings (BasePlan* basePlan, Json::Value& jError, Json::Value& jWarningList) {
	Json::Value jTips = Json::arrayValue;
	m_expire = 0;
	//先增加不舒适提示
	bool isBackHotelUncomfortable = false, isDeptHotelUncomfortable = false;
	//中间天第一个点为离开酒店 倒数第二个点为回酒店时间
	//首天只可能存在回酒店时间 尾天只有离开酒店时间
	int uncomfortableBackTime = MJ::MyTime::toTime(m_date+"_"+UNCOMFORTABLE_BACK_HOTELTIME, basePlan->m_TimeZone);
	int uncomfortableDeptTime = MJ::MyTime::toTime(m_date+"_"+UNCOMFORTABLE_DEPT_HOTELTIME, basePlan->m_TimeZone);
	int backHotelTime = uncomfortableBackTime, deptHotelTime = uncomfortableDeptTime;
	if (m_dayMax > 1) {
		if (m_didx+1 != m_dayMax) {
			if (m_outRouteJ.size() < 3) {
				_INFO("the outRouteJ is Err, m_didx: %d, date: %s", m_didx, m_date.c_str());
				return false;
			}
			const Json::Value& backHotel = m_outRouteJ[m_outRouteJ.size()-2];
			backHotelTime = backHotel["arrange"]["time"][0u].asInt();
			if (backHotelTime >= uncomfortableBackTime) isBackHotelUncomfortable = true;
		}
		if (m_didx != 0) {
			const Json::Value& deptHotel = m_outRouteJ[0u];
			deptHotelTime = deptHotel["arrange"]["time"][0u].asInt();
			if (deptHotelTime <= uncomfortableDeptTime) isDeptHotelUncomfortable = true;
		}
		{
			std::string tip = "请注意，当天行程";
			//add 不舒适tips
			if (isDeptHotelUncomfortable) {
				std::string deptHotelStr = MJ::MyTime::toString(deptHotelTime, basePlan->m_TimeZone);
				std::string deptDate = deptHotelStr.substr(0,8);
				long long cmpDate = MJ::MyTime::compareDayStr(deptDate, m_date);
				if (cmpDate != 0) {
					tip += "前" +std::to_string(cmpDate)+ "天";
				}
				tip += deptHotelStr.substr(9) + "开始";
			}
			if (isBackHotelUncomfortable) {
				if (isDeptHotelUncomfortable) tip += "，";
				std::string backHotelStr = MJ::MyTime::toString(backHotelTime, basePlan->m_TimeZone);
				std::string backDate = backHotelStr.substr(0,8);
				long long cmpDate = MJ::MyTime::compareDayStr(m_date, backDate);
				if (cmpDate == 1) {
					tip += "次日";
				}
				tip += backHotelStr.substr(9) + "返回休息";
			}
			if (isDeptHotelUncomfortable || isBackHotelUncomfortable) {
				Json::Value jWarning = Json::Value();
				jWarning["desc"] = tip;
				jWarning["vidx"] = -1;
				jWarning["didx"] = m_didx;
				jWarning["type"] = 9;
				jWarningList.append(jWarning);
			}
		}
	}

	for (int i = 0; i < m_outRouteJ.size(); i++) {
		const Json::Value& nodeJ = m_outRouteJ[i];
		if (!NodeJ::hasError(nodeJ)) continue;
		bool isTheNodeCanDel = NodeJ::isNodeCanDel(nodeJ);
		//先报客观冲突
		{
			std::vector<int> inverseList;
			std::vector<int> overlapList;
			int thePreFixedIdx = -1;
			for (int j = 0; j < i; j++) {
				Json::Value& preNode = m_outRouteJ[j];
				if (NodeJ::isFree(preNode)) continue;
				if (preNode["arrange"]["time"][0u] >= nodeJ["arrange"]["time"][1]) {
					inverseList.push_back(j);
				} else if (nodeJ["arrange"]["time"][0] < preNode["arrange"]["time"][1]) {
					overlapList.push_back(j);
				}
				if (thePreFixedIdx < j) thePreFixedIdx = j;
			}
			{
				//顺序颠倒
				if (nodeJ["arrange"]["error"][0u].asInt() == 1) {
					std::string tip = "";
					bool isInverseCanDel = true;
					for (int j = 0; j < inverseList.size(); j++) {
						const Json::Value& inverseNode = m_outRouteJ[inverseList[j]];
						if (isInverseCanDel && !NodeJ::isNodeCanDel(inverseNode)) isInverseCanDel = false;  //只要顺序颠倒的点中有不可删除点 就无需再判断
						std::string placeName = getPlaceName(basePlan, inverseList[j]);
						tip += placeName;
						if (j != inverseList.size() - 1) {
							tip += "、";
						}
					}
					std::string placeName = getPlaceName(basePlan, i);
					tip = tip + "与" + placeName + "，指定时间与当前顺序冲突";
					Json::Value jErr = Json::Value();
					Json::Value jWarning = Json::Value();
					jErr["type"] = 3;
					jErr["content"] = tip;
					//均不可删除时才不可智能优化
					if (!isTheNodeCanDel && !isInverseCanDel) {
						jErr["cannot_smart_opt"] = 1;
						jWarning["cannot_smart_opt"] = 1;
					} else if (i == m_outRouteJ.size()-1 and m_dayMax > 1 and m_didx != m_dayMax-1) {
						goto lastErr;
					}
					jTips.append(jErr);

					jWarning["didx"] = m_didx;
					jWarning["vidx"] = -1;
					jWarning["desc"] = tip;
					jWarning["type"] = 3;
					jWarning["inner_vidx"] = i;
					jWarningList.append(jWarning);
					m_expire = 1;
				}
			}
			{
				Json::Value jErr = Json::Value();
				Json::Value jWarning = Json::Value();
				//重叠、可用时间不足
				std::string placeName = getPlaceName(basePlan, i);
				if (nodeJ["arrange"]["error"][1].asInt() == 1 and overlapList.size()>0) {
					bool isPreNodeCanDel = false;
					std::string tip = "";
					for (int j = 0; j < overlapList.size(); j++) {
						const Json::Value& preFixedNode = m_outRouteJ[overlapList[j]];
						std::string prePlaceName = getPlaceName(basePlan, overlapList[j]);
						if (!isPreNodeCanDel) isPreNodeCanDel = NodeJ::isNodeCanDel(preFixedNode);
						tip += prePlaceName + "、";
					}
					if (!isTheNodeCanDel && !isPreNodeCanDel) {
						jErr["cannot_smart_opt"] = 1;
						jWarning["cannot_smart_opt"] = 1;
					} else if (i == m_outRouteJ.size()-1 and m_dayMax > 1 and m_didx != m_dayMax-1) {
						goto lastErr;
					}
					m_expire = 1;
					std::string errTip = tip + placeName + "，指定的停留时间段重叠";
					jErr["type"] = 2;
					jErr["content"] = errTip;
					jTips.append(jErr);

					jWarning["didx"] = m_didx;
					jWarning["vidx"] = -1;
					jWarning["desc"] = errTip;
					jWarning["type"] = 2;
					jWarning["inner_vidx"] = i;
					jWarningList.append(jWarning);
				} else if (nodeJ["arrange"]["error"][2].asInt() == 1 and thePreFixedIdx != -1) {
					if (i == m_outRouteJ.size()-1 and m_dayMax > 1 and m_didx != m_dayMax-1) goto lastErr;
					std::string tip = "";
					const Json::Value& thePreFixedNode = m_outRouteJ[thePreFixedIdx];
					tip = getPlaceName(basePlan, thePreFixedIdx);
					m_expire = 1;
					std::string errTip = tip + "与" + placeName + "之间的可用时间不足";
					jErr["type"] = 1;
					jErr["content"] = errTip;
					jTips.append(jErr);

					jWarning["didx"] = m_didx;
					jWarning["vidx"] = -1;
					jWarning["desc"] = errTip;
					jWarning["type"] = 1;
					jWarning["inner_vidx"] = i;
					jWarningList.append(jWarning);
				}
			}
		}
lastErr:
		//最后一个点无法按时到达
		{
			if (i == m_outRouteJ.size()-1 && nodeJ["arrange"]["error"][5].asInt() == 1) {
				Json::Value jErr = Json::Value();
				Json::Value jWarning = Json::Value();
				jWarning["didx"] = m_didx;

				std::string errTip = "";
				std::string warningTip = "";
				//行程最后一天
				if (m_dayMax == 1 //不过夜行程
						|| (m_dayMax > 1 && m_didx == m_dayMax-1)) { //过夜行程最后一天
					//最后一天行程出错 需要锁定点之间有free点才报
					int thePreNodeIdx = i-1;
					if (thePreNodeIdx >= 0) {
						const Json::Value& thePreNode = m_outRouteJ[thePreNodeIdx];
						if (!NodeJ::isFree(thePreNode)) continue;
					}
					m_expire = 1;
					jErr["type"] = 4;
					jWarning["type"] = 4;
					if (basePlan->isChangeTraffic) {
						errTip = "考虑市内交通后当日行程已经超出离开城市时间，请缩短自定义交通时长或减少景点";
					} else {
						errTip = "考虑市内交通后当日行程已经超出离开城市时间，需要减少游玩时长或地点";
					}
					warningTip = "当日行程已经超出离开城市时间，需要减少游玩时长或地点";
				}
				//中间天休息不足
				else {
					m_expire = 1;
					jErr["type"] = 5;
					jWarning["type"] = 5;
					if (basePlan->isChangeTraffic) {
						errTip = "考虑市内交通后当日行程已经超出下一天酒店出发时间，请缩短自定义交通时长或减少景点";
					} else {
						errTip = "考虑市内交通后当日行程已经超出下一天酒店出发时间，需要减少游玩时长或地点";
					}
					warningTip = "当日行程已经超出下一天酒店出发时间，需要减少游玩时长或地点";
					jWarning["didx"] = m_didx;
				}
				jErr["content"] = errTip;
				jTips.append(jErr);
 
				jWarning["vidx"] = -1;
				jWarning["inner_vidx"] = -1;
				jWarning["desc"] = warningTip;
				jWarningList.append(jWarning);
			}
		}
	}
	//玩乐不可用
	for (int i = 0; i < m_outRouteJ.size(); i++) {
		const Json::Value& nodeJ = m_outRouteJ[i];
		std::string id = nodeJ["id"].asString();
		const LYPlace* place = basePlan->GetLYPlace(id);
		if (place == NULL) continue;
		if (place->_t & LY_PLACE_TYPE_TOURALL) {
			bool isAvail = false;
			const Tour* tour = dynamic_cast<const Tour*>(place);
			if (tour && LYConstData::IsTourAvailable(tour, m_date)) {
				std::vector<const TicketsFun*> tickets;
				basePlan->GetProdTicketsListByPlace(tour, tickets, m_date);
				if (tickets.size() > 0) {
					isAvail = true;
					_INFO("wanle id:%s,date:%s",place->_ID.c_str(),m_date.c_str());
				}
			}
			if (!isAvail) {
				std::string placeName = getPlaceName(basePlan, i);
				std::string tip = placeName + "当前日期不可用，请重新选择日期";
				Json::Value jErr = Json::Value();
				jErr["type"] = 6;
				jErr["content"] = tip;
				jTips.append(jErr);

				Json::Value jWarning = Json::Value();
				jWarning["didx"] = m_didx;
				jWarning["vidx"] = -1;
				jWarning["desc"] = tip;
				jWarning["type"] = 6;
				jWarningList.append(jWarning);
			}
		}
	}

	if (jTips.size() > 0) {
		jError["didx"] = m_didx;
		jError["tips"] = jTips;
	}
	return true;
}

bool DayRoute::GetDayView (BasePlan* basePlan, Json::Value& jError, Json::Value& jDay, Json::Value& jWarningList) {
	bool ret = true;
	ret = dealDayPois();
	if (debugInfo) Show(basePlan->m_TimeZone);
	if (ret) {
		ret = routeJ2ViewDay(basePlan, jError, jDay, jWarningList);
	}
	return ret;
}

bool DayRoute::dealDayPois() {
	bool ret = true;
	PathGenerator* pathGen = new PathGenerator(m_inRouteJ, m_controls);
	MJ::MyTimer t;
	t.start();
	ret = pathGen->DayPathExpandOpt();
	//ret = pathGen->DayPathSearch();
	int cost = t.cost();
	if (ret) m_outRouteJ = pathGen->GetResult();
	Json::FastWriter fw;
	std::cerr << "date: " << m_date << "  m_outRouteJ: " << fw.write(m_outRouteJ) << std::endl;
	if (pathGen) {
		delete pathGen;
		pathGen = NULL;
	}
	return ret;
}

bool DayRoute::routeJ2ViewDay (BasePlan* basePlan, Json::Value& jError, Json::Value& jDay, Json::Value& jWarningList) {
	//报错
	getErrorAndWarnings(basePlan, jError, jWarningList);

	Json::Value jViewList = Json::arrayValue;
	makeJViewList(basePlan, jViewList);
	jDay["date"] = m_date;
	jDay["view"] = jViewList;
	int stime = m_outRouteJ[0u]["arrange"]["time"][1].asInt();
	int etime = m_outRouteJ[m_outRouteJ.size()-1]["arrange"]["time"][0u].asInt();
	std::string stimeStr = MJ::MyTime::toString(stime, basePlan->m_TimeZone);
	std::string etimeStr = MJ::MyTime::toString(etime, basePlan->m_TimeZone);
	PostProcessor::SetDayPlayRange(jDay, stimeStr, etimeStr);
	Json::FastWriter fw;
	std::cerr << "makejView:" << fw.write(jViewList) << std::endl
		<< "warning list: " << fw.write(jWarningList) << std::endl
		<< "error list: " << fw.write(jError) << std::endl;
	return true;
}

bool DayRoute::makeJViewList(BasePlan* basePlan, Json::Value& jViewList) {
	if (basePlan->m_useTrafShow) {
		PathPerfect::AddTrafShow(basePlan);
	}
	for (int i = 0; i < m_outRouteJ.size(); i++) {
		Json::Value& nodeJ = m_outRouteJ[i];
		Json::Value jView = Json::Value();
		//特殊处理routeJ 最后的两个酒店点合并
		if (i+1 == m_outRouteJ.size()-1 && m_didx != m_dayMax-1 && m_dayMax > 1) {
			Json::Value& freeHotelNode = nodeJ;
			Json::Value& fixHotelNode = m_outRouteJ[i+1];
			int stime = freeHotelNode["arrange"]["time"][0u].asInt();
			int etime = fixHotelNode["arrange"]["time"][0u].asInt();
			//非首尾天只有酒店时，酒店时间改为偏好时间 size为3表示只有早上出发酒店 free酒店 和 晚上酒店
			if (m_didx > 0 && m_outRouteJ.size() == 3) {
				stime = MJ::MyTime::toTime(m_date+"_00:00", basePlan->m_TimeZone);
				if (basePlan->m_HotelOpenTime != 0) stime += basePlan->m_HotelOpenTime;
				else {
					stime += 3600*19;
				}
				if (stime > etime) stime = etime;
			}
			Json::Value hotelNode = fixHotelNode;
			NodeJ::setPlayRange(hotelNode, stime, etime);
			nodeJ = hotelNode;
			//结束循环
			i++;
		}
		if(!makeJView(basePlan, nodeJ, jView)) continue;
		jView["traffic"] = Json::Value();
		// size 为 2  两个站点
		// size 为 3  站点和酒店 或者 酒店和酒店
		if (jViewList.size() == 1 and 
				((m_didx + 1 == m_dayMax and m_outRouteJ.size() == 2)
				 || (m_didx+1 < m_dayMax and m_outRouteJ.size() == 3))) {
			int thePoiEtime = m_outRouteJ[0u]["arrange"]["time"][1].asInt();
			int nextPoiStime = m_outRouteJ[1]["arrange"]["time"][0u].asInt();
			int trafTime = 0;
			const Json::Value& nodeJ = m_outRouteJ[0];
			std::string nextNodeId = m_outRouteJ[1]["id"].asString();
			if (nodeJ.isMember("toNext") and nodeJ["toNext"].isObject() and nodeJ["toNext"].isMember(nextNodeId) and nodeJ["toNext"][nextNodeId].isInt()) {
				trafTime = nodeJ["toNext"][nextNodeId].asInt();
			}
			if (nextPoiStime - thePoiEtime - trafTime > 3600 * 3) {
				//自由活动 - 交通时间
				jViewList[jViewList.size()-1]["idle"]["desc"] = "自由活动";
				jViewList[jViewList.size()-1]["idle"]["dur"] = (nextPoiStime - thePoiEtime - trafTime)/900 * 900;
			}
		}

		//traffic
		if (i+1 != m_outRouteJ.size()) {
			std::string nodeId = m_outRouteJ[i]["id"].asString();
			std::string nextNode = m_outRouteJ[i+1]["id"].asString();
			const TrafficItem* trafItem = basePlan->GetTraffic(nodeId, nextNode, m_date);
			if (trafItem) {
				Json::Value jTrafItem;
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
				if(trafItem->m_warnings.size() > 0)
					jTrafItem["tips"] = trafItem->m_warnings[0].asString();
				jTrafItem["transfer"] = Json::arrayValue;
				for(int it = 0; it < trafItem->m_transfers.size(); it++) {
					jTrafItem["transfer"].append(trafItem->m_transfers[it]);
				}
				jView["traffic"] = jTrafItem;
			} else {
				Json::Value jTrafItem;
				jTrafItem["id"] = "";
				jTrafItem["dur"] = 0;
				jTrafItem["type"] = 2;
				jTrafItem["dist"] = 0;
				jTrafItem["custom"] = 0;
				jTrafItem["eval"] = 0;
				jTrafItem["tips"] = "";
				jTrafItem["transfer"] = Json::arrayValue;
				jView["traffic"] = jTrafItem;
			}
		} else {
			jView["traffic"] = Json::Value();
			if (jView.isMember("idle")) jView.removeMember("idle");
		}
		jViewList.append(jView);
	}
	if (jViewList.size() > 0) {
		jViewList[jViewList.size()-1]["traffic"] = Json::Value();
	}
}

bool DayRoute::makeJView (BasePlan* basePlan, const Json::Value& nodeJ, Json::Value& jView) {
	//构造ViewList
	const LYPlace* place = basePlan->GetLYPlace(nodeJ["id"].asString());
	if (place == basePlan->m_arvNullPlace || place == basePlan->m_deptNullPlace) return false;
	jView["id"] = basePlan->GetCutId(nodeJ["id"].asString());
	jView["custom"] = place->m_custom;
	jView["type"] = place->_t;
	jView["name"] = place->_name;
	if (place->_name == "") jView["name"] = place->_enname;
	jView["lname"] = place->_lname;
	jView["coord"] = place->_poi;
	jView["do_what"] = "";
	if (basePlan->m_poiPlays.find(nodeJ["id"].asString()) != basePlan->m_poiPlays.end()) {
		jView["play"] = basePlan->m_poiPlays[nodeJ["id"].asString()];
	} else {
		jView["play"] = "";
	}
	if (basePlan->m_poiFunc.count(nodeJ["id"].asString())) {
		jView["func_type"] = basePlan->m_poiFunc[nodeJ["id"].asString()];
	} else {
		jView["func_type"] = NODE_FUNC_NULL;
	}

	if (place->_t & LY_PLACE_TYPE_VAR_PLACE && nodeJ["arrange"]["error"][4] == 1) {
		jView["close"] = 1;
	} else {
		jView["close"] = 0;
	}
	/*
	int lunchTime = MJ::MyTime::toTime(m_date+"_"+RESTAURANT_LUNCH_TIME, basePlan->m_TimeZone);
	int supperTime = MJ::MyTime::toTime(m_date+"_"+RESTAURANT_SUPPER_TIME, basePlan->m_TimeZone);
	if ((nodeJ["arrange"]["time"][0u] <= lunchTime && nodeJ["arrange"]["time"][1] > lunchTime)
		||(nodeJ["arrange"]["time"][0u] <= supperTime && nodeJ["arrange"]["time"][1] > supperTime)) {
		jView["dining_nearby"] = 1;
	} else {
		jView["dining_nearby"] = 0;
	}
	*/
	jView["dining_nearby"] = 0;
	int stime = nodeJ["arrange"]["time"][0u].asInt();
	int etime = nodeJ["arrange"]["time"][1].asInt();
	jView["stime"] = MJ::MyTime::toString(stime, basePlan->m_TimeZone);
	jView["etime"] = MJ::MyTime::toString(etime, basePlan->m_TimeZone);
	if (place->_t & LY_PLACE_TYPE_HOTEL && NodeJ::isFixed(nodeJ)) {
		jView["dur"] = -1;
	} else {
		jView["dur"] = etime - stime;
	}

	//product
	if (place->_t & LY_PLACE_TYPE_TOURALL) {
		jView["type"] = place->_t;
		if (place->_t == LY_PLACE_TYPE_VIEWTICKET) {
			const ViewTicket* viewTicket = dynamic_cast<const ViewTicket*>(place);
			if( viewTicket != NULL ) {
				// 展示关联景点id和name
				jView["id"] = BasePlan::GetCutId(viewTicket->Getm_view());
				auto oriView = basePlan->GetLYPlace(viewTicket->Getm_view());
				if (oriView == NULL) {
					jView["name"] = "";
					jView["lname"] = "";
				} else {
					jView["name"] = oriView->_name;
					jView["lname"] = oriView->_enname;
				}
			}
			//展示景点type
			jView["type"] = LY_PLACE_TYPE_VIEW;
		}
		std::string id = jView["id"].asString();
		const LYPlace* vPlace = basePlan->GetLYPlace(id);
		const VarPlace* varPlace = NULL;
		if (vPlace) varPlace = dynamic_cast<const VarPlace*>(vPlace);
		if (varPlace and varPlace->_t == LY_PLACE_TYPE_VIEW) {
			std::vector<std::pair<int,int>> openCloseList;
			basePlan->GetOpenCloseTime(jView["stime"].asString().substr(0,8), varPlace, openCloseList);
			if(openCloseList.empty()) {
				jView["close"] = 1;
			} else {
				time_t open=openCloseList.front().first;
				time_t close=openCloseList.back().second;
				if(stime < open-900 || etime > close + 900)
					jView["close"] = 1;
			}
		}
		PostProcessor::SetProduct(basePlan, place, jView);
	}
	return true;
}

std::string DayRoute::getPlaceName(BasePlan* basePlan, int index) {
	const LYPlace* place = basePlan->GetLYPlace(basePlan->GetCutId(m_outRouteJ[index]["id"].asString()));
	if (place == NULL) return "";
	std::string placeName = place->_name;
	if (placeName == "") placeName = place->_enname;
	if (place == basePlan->m_arvNullPlace) {
		placeName = "到达城市时刻";
	} else if (place == basePlan->m_deptNullPlace) {
		placeName = "离开城市时刻";
	} else if (place->_t == LY_PLACE_TYPE_HOTEL) {
	   if (index == 0 && m_didx != 0) {
			placeName += "出发时间";
	   } else if (index+1 == m_outRouteJ.size()) {
		   placeName += "下一天出发时间";
	   }
	} else if (place->_t & LY_PLACE_TYPE_ARRIVE && index == 0) {
		placeName += "（到达）" ;
	} else if (place->_t & LY_PLACE_TYPE_ARRIVE && index == m_outRouteJ.size()-1) {
		placeName += "（离开）";
	} else if (place->_t & LY_PLACE_TYPE_CAR_STORE) {
		if (place->m_mode == 0) {
			placeName = "取车门店" + placeName;
		} else if (place->m_mode == 1) {
			placeName = "还车门店" + placeName;
		}
	}
	return placeName;
}

bool DaysPlan::ChangeConflict2TourErr(const Json::Value& req, const Json::Value& jWarningList, Json::Value& resp) {
	resp["data"]["type"] = 1;
	Json::Value jWarning = Json::Value();
	for (int i = 0; i < jWarningList.size(); i++) {
		jWarning = jWarningList[i];
		if (jWarning.isMember("inner_vidx") and jWarning["didx"] == 0) {
			break;
		}
	}
	int vidx = jWarning["inner_vidx"].asInt() - 1; //减去加入的到达点
	if (vidx >= 0 and vidx < req["city"]["view"]["day"][0u]["view"].size()) {
		const Json::Value& jView = req["city"]["view"]["day"][0u]["view"][vidx];
		if (jView.isMember("type") and jView["type"].isInt() and jView["type"].asInt() & (LY_PLACE_TYPE_CAR_STORE|LY_PLACE_TYPE_VAR_PLACE)) {
			std::string stime = jView["stime"].asString().substr(9);
			std::string name = jView["name"].asString();
			if (jView["type"].asInt() == LY_PLACE_TYPE_CAR_STORE) {
				resp["data"]["content"] = "您选择的开始时间将导致"+stime+"预约的租车服务无法使用，请重新选择";
			} else if (jView["type"].asInt() & LY_PLACE_TYPE_TOURALL) {
				resp["data"]["content"] = "您选择的开始时间将导致"+stime+"开始的"+name+"不可用，请重新选择";
			} else if (jView["type"].asInt() == LY_PLACE_TYPE_VIEW) {
				if (jView.isMember("product") and jView["product"].isMember("product_id") and jView["product"]["product_id"].isString()) {
					std::string productId = jView["product"]["product_id"].asString();
					if (req.isMember("product") and req["product"].isMember("wanle") and req["product"]["wanle"].isMember(productId)) {
						const Json::Value& jProduct = req["product"]["wanle"][productId];
						if (jProduct.isMember("name") and jProduct["name"].isString()) name = jProduct["name"].asString();
					}
				}
				resp["data"]["content"] = "您选择的开始时间将导致"+stime+"开始的"+name+"不可用，请重新选择";
			}
			return true;
		}
	}
	if (req["city"]["view"]["day"].size() > 1) {
		resp["data"]["content"] = "当前开始时间导致当天行程结束时间与第二天行程重叠，请调整开始时间";
	} else {
		if (req.isMember("needEtime")) {
			resp["data"]["content"] = "当前开始时间导致当天行程结束时间超过24:00，请调整开始时间";
		} else {
			std::string deptTime = "";
			if (req.isMember("city") and req["city"].isMember("dept_time") and req["city"]["dept_time"].isString() and req["city"]["dept_time"].asString().length() == 14) {
				deptTime = req["city"]["dept_time"].asString().substr(9);
			}
			resp["data"]["content"] = "您选择的开始时间，将导致"+deptTime+"出发的交通不可使用，请重新选择";
		}
	}
	return true;
}

const LYPlace* DaysPlan::GetHotelByDidx (BasePlan* basePlan, int didx) {
	const LYPlace* hotel = NULL;
	if (didx < 0) didx = 0;
	const HInfo* hInfo = basePlan->GetHotelByDidx(didx);
	if (hInfo) hotel = hInfo->m_hotel;
	else {
		hotel = LYConstData::GetCoreHotel(m_cityID);
	}
	if (hotel == NULL) {
		_INFO("get core hotel error, city:%s",m_cityID.c_str());
	}
	return hotel;
}

int DaysPlan::hasExpire(int didx) {
	if (didx > m_dayPlan.size()) return 0;
	if (didx == m_dayPlan.size()) {
		int expire = 0;
		for (int i = 0; i < didx; i++) {
			expire |= m_dayPlan[i]->hasExpire();
		}
		return expire;
	}
	return m_dayPlan[didx]->hasExpire();
}
int DayRoute::hasExpire() {
	return m_expire;
}

bool DayRoute::Show(int timeZone) {
	Json::Value ShowList = Json::arrayValue;
	if (m_outRouteJ.isArray()) {
		for (int i = 0; i < m_outRouteJ.size(); i++) {
			const Json::Value& node = m_outRouteJ[i];
			Json::Value ShowItem = Json::Value();
			ShowItem["id"] = node["id"];
			ShowItem["error"] = node["arrange"]["error"];
			int stime = node["arrange"]["time"][0u].asInt();
			int etime = node["arrange"]["time"][1].asInt();
			std::string sTimeStr = MJ::MyTime::toString(stime,timeZone);
			std::string eTimeStr = MJ::MyTime::toString(etime,timeZone);
			ShowItem["time"] = Json::arrayValue;
			ShowItem["time"].append(sTimeStr);
			ShowItem["time"].append(eTimeStr);
			ShowItem["selectRangeIdx"] = node["arrange"]["controls"]["rangeIdx"];
			Json::Value jTimeList = Json::arrayValue;
			if (node.isMember("fixed")) {
				jTimeList = node["fixed"]["times"];
			} else if (node.isMember("free")) {
				jTimeList = node["free"]["openClose"];
			}
			for (int j = 0; j < jTimeList.size(); j++) {
				Json::Value& jTimes = jTimeList[j];
				stime = jTimes[0u].asInt();
				etime = jTimes[1].asInt();
				std::string sTimeStr = MJ::MyTime::toString(stime,timeZone);
				std::string eTimeStr = MJ::MyTime::toString(etime,timeZone);
				Json::Value jStringTime = Json::arrayValue;
				jStringTime.append(sTimeStr);
				jStringTime.append(eTimeStr);
				if (node.isMember("fixed")) ShowItem["fixedTimes"].append(jStringTime);
				else ShowItem["freeOpenClose"].append(jStringTime);
			}
			ShowItem["toNext"] = node["toNext"];
			ShowList.append(ShowItem);
		}
		Json::FastWriter fw;
		//std::cerr << "date: " << m_date << "  didx: " << m_didx << std::endl << fw.write(ShowList) << std::endl;
		std::cerr << "date: " << m_date << "  didx: " << m_didx << std::endl << ShowList << std::endl;
	}
	return true;
}
