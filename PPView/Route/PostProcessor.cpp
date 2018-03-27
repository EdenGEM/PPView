#include "MJCommon.h"
#include "Route/base/TrafficData.h"
#include "Route/base/PathPerfect.h"
#include "Route/base/PathEval.h"
#include "Route/base/PathStat.h"
//#include "Widget.h"
#include "PostProcessor.h"
#include <MyTime.h>
#include "PlanThroughtPois.h"
#include "Route/base/TimeIR.h"
//市内行程规划主函数 return->0:正常结束 1:用户取消请求
int PostProcessor::PostProcess(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	MJ::PrintInfo::PrintDbg("[%s]PostProcessor::PostProcess, PostProcess ...", basePlan->m_qParam.log.c_str());

	if (basePlan->m_qParam.type == "ssv005_rich" || basePlan->m_qParam.type == "ssv005_s130") {
		MakeOutputSSV005(req, basePlan, resp);
	} else if (basePlan->m_qParam.type == "p202"
			|| basePlan->m_qParam.type == "p101") {
		MakeOutputP202(req, basePlan, resp);
	} else if (basePlan->m_qParam.type == "p104") {
		MakeOutputP104(req, basePlan, resp);
	} else if (basePlan->m_qParam.type == "p105") {
		MakeOutputP105(req, basePlan, resp);
	} else if (basePlan->m_qParam.type == "p201") {
		MakeOutputP201(req, basePlan, resp);
	} else if (basePlan->m_qParam.type == "b116") {
		MakeOutputB116(req, basePlan, resp);
    } else if (basePlan->m_qParam.type == "s126") {
        MakeOutputS126(req, basePlan, resp);
    } else if (basePlan->m_qParam.type == "s130") {
		MakeOutputS130(req, basePlan, resp);
	}
    return 0;
}

int PostProcessor::SetProduct(BasePlan* basePlan, const LYPlace* place, Json::Value& jView) {
	Json::Value product = Json::Value();
	Json::Value tickets = Json::Value();
	const Tour* tour = dynamic_cast<const Tour*>(place);
	if (tour) {
		// 展示产品本身id及name
		product["pid"] = tour->m_pid;
		product["name"] = tour->_name;
		product["lname"] = tour->_enname;
		product["mode"] = tour->_t;
		product["date"] = jView["stime"].asString().substr(0,8);
		std::string stime = jView["stime"].asString().substr(9);
		product["product_id"] = product["pid"].asString()+"#"+product["date"].asString()+"#"+stime;
		//有场次信息
		if (tour->m_srcTimes.size() > 0 && tour->m_srcTimes[tour->m_srcTimes.size()-1].isMember("t") && tour->m_srcTimes[tour->m_srcTimes.size()-1]["t"].isString() && tour->m_srcTimes[tour->m_srcTimes.size()-1]["t"].asString() != "") {
			product["stime"] = stime;
		} else {
			product["stime"] = "";
		}
		jView["info"]["enterPre"] = tour->m_preTime;
		int dur = jView["dur"].asInt();
		if (tour->m_srcTimes.isArray() && tour->m_srcTimes.size() > 0) {
			if (tour->m_srcTimes[0].isMember("dur") && tour->m_srcTimes[0]["dur"].isInt()) {
				dur = tour->m_srcTimes[0]["dur"].asInt();
				jView["info"]["dur"] = dur;
			} else {
				jView["info"]["dur"] = "";
			}
		}

		//const TicketsFun *ticket = NULL;
		std::vector<const TicketsFun *> ticketsList;
		basePlan->GetProdTicketsListByPlace(place, ticketsList, product["date"].asString());
		for (int j = 0; j < ticketsList.size(); ++j) {
			Json::Value vticket;
			if (LYConstData::MakeTicketSource(tour, product["date"].asString(), ticketsList[j], vticket["source"]) == 0) {
				if (basePlan->m_pid2ticketIdAndNum.find(place->_ID) != basePlan->m_pid2ticketIdAndNum.end()) {
					auto &list = basePlan->m_pid2ticketIdAndNum[place->_ID];
					auto it = list.find(ticketsList[j]->m_id);
					if (it != list.end()) {
						vticket["num"] = it->second;
					}
				} else {
					if (ticketsList[j]->m_userNum > 0) {
						vticket["num"] = ((basePlan->m_AdultCount + ticketsList[j]->m_userNum-1) / (ticketsList[j]->m_userNum));
					} else {
						vticket["num"] = basePlan->m_AdultCount;
					}
				}
				tickets.append(vticket);
			}
		}

		jView["product"] = product;
		jView["product"]["tickets"] = tickets;
	}
	return 0;
}

//构造resp["data"]["view"]
//jPois -- resp["data"]["view"]["summary"]["days"]["pois"]
//jView -- resp["data"]["view"]["day"]["view"]
int PostProcessor::MakeJView(const PlanItem* item, const PlanItem* nextItem, BasePlan* basePlan, Json::Value& jView, Json::Value& jWarningList, int dIdx, int vIdx) {
	jView["id"] = BasePlan::GetCutId(item->_place->_ID);
	jView["stime"] = MJ::MyTime::toString(item->_arriveTime, basePlan->m_TimeZone);
	jView["etime"] = MJ::MyTime::toString(item->_departTime, basePlan->m_TimeZone);
	jView["dur"] = static_cast<int>(item->_departTime - item->_arriveTime);
	jView["play"] = basePlan->m_poiPlays[item->_place->_ID];
	//玩乐 场次 dur信息 --- view[info][pdur]
	if(item->_place->_t & LY_PLACE_TYPE_TOURALL) {
		SetProduct(basePlan, item->_place, jView);
	}

	//add by shyy
	jView["name"] = item->_place->_name;
	if (item->_place->m_custom == POI_CUSTOM_MODE_CUSTOM) {
		jView["lname"] = item->_place->_lname;//name_en 和产品确认过
	} else {
		jView["lname"] = item->_place->_enname;
	}

	if(item->_place->_t == LY_PLACE_TYPE_VIEWTICKET) {
		const ViewTicket* viewTicket = dynamic_cast<const ViewTicket*>(item->_place);
		if( viewTicket != NULL ) {
			// 展示关联景点id和name
			jView["id"] = BasePlan::GetCutId(viewTicket->Getm_view());
			auto oriView = basePlan->GetLYPlace(viewTicket->Getm_view());
			if (oriView == NULL) {
				jView["name"] = "";
				jView["lname"] = "";
			} else {
				jView["name"] = oriView->_name;
				if (oriView->m_custom == POI_CUSTOM_MODE_CUSTOM) {
					jView["lname"] = oriView->_lname;//name_en 和产品确认过
				} else {
					jView["lname"] = oriView->_enname;
				}
			}
		}
	}


	jView["custom"] = item->_place->m_custom;
	jView["coord"] = item->_place->_poi;
	jView["close"] = 0;
	jView["dining_nearby"] = item->m_hasAttach ? 1:0;
	if (item->_type & NODE_FUNC_PLACE) {
		std::vector<std::pair<int, int>> openCloseList;
		std::string id = jView["id"].asString();
		const LYPlace* place = basePlan->GetLYPlace(id);
		const VarPlace* vPlace = NULL;
		if (place) vPlace = dynamic_cast<const VarPlace*>(place);
		std::string date = (MJ::MyTime::toString(item->_arriveTime, basePlan->m_TimeZone)).substr(0,8);
		std::string deptDate = (MJ::MyTime::toString(item->_departTime, basePlan->m_TimeZone)).substr(0,8);
		if (vPlace) {
			std::vector<std::pair<int,int>> openCloseList;
			basePlan->GetOpenCloseTime(date, vPlace, openCloseList);
			if(openCloseList.empty()) {
				jView["close"] = 1;
			} else if (date == deptDate) {
				time_t open=openCloseList.front().first;
				time_t close=openCloseList.back().second;
				if(item->_arriveTime < open-900 || item->_departTime > close + 900)
					jView["close"] = 1;
			} else {
				std::vector<std::pair<int, int>> nextDayOpenCloseList;
				basePlan->GetOpenCloseTime(deptDate, vPlace, nextDayOpenCloseList);
				if (nextDayOpenCloseList.empty()) {
					jView["close"] = 1;
				} else {
					time_t open = openCloseList.back().first;
					time_t close = nextDayOpenCloseList.front().second;
					if (item->_arriveTime < open-900 or item->_departTime > close + 900) {
						jView["close"] = 1;
					}
				}
			}
		}
	}
	//func_type
	//目前仅输出arv/deptPoi 放/取行李点 酒店睡觉点 (keynode)
	if (item->_type & NODE_FUNC_KEY) {
		jView["func_type"] = item->_type;
	} else {
		jView["func_type"] = NODE_FUNC_NULL;
	}
	if (LYConstData::IsRealID(item->_place->_ID)) {
		jView["type"] = item->_place->_t;
		if (item->_place->_t & LY_PLACE_TYPE_TOURALL) {
			jView["type"] = item->_place->_t;
		}
	} else {
		jView["type"] = 0;
	}
    // 景点门票展示景点类型
    if((item->_place->_t == LY_PLACE_TYPE_VIEWTICKET)) {
        jView["type"] = LY_PLACE_TYPE_VIEW;
    }


	jView["do_what"] = item->_remind_tag;
	if (item->_departTraffic && TrafficData::IsReal(item->_departTraffic->_mid)) {
		Json::Value jTrafItem;
		jTrafItem["id"] = item->_departTraffic->_mid;
		jTrafItem["dur"] = item->_departTraffic->_time;
		bool isCharterCar = (basePlan->m_travalType == TRAVAL_MODE_CHARTER_CAR) ? true : false;
		jTrafItem["type"] = TrafficItem::InnerTrafMode2UserDefine(item->_departTraffic->_mode, isCharterCar, item->_departTraffic->m_custom);
		jTrafItem["dist"] = item->_departTraffic->m_realDist;
		jTrafItem["custom"] = item->_departTraffic->m_custom == true? 1 : 0;
		if (item->_departTraffic->_mid.find("Sphere") != std::string::npos) {
			jTrafItem["eval"] = 1;
		} else {
			jTrafItem["eval"] = 0;
		}
		jTrafItem["tips"] = "";
		if (item->_departTraffic->m_warnings.size() > 0)
			jTrafItem["tips"] = item->_departTraffic->m_warnings[0].asString();
		jTrafItem["transfer"] = Json::arrayValue;
		for(int it = 0; it < item->_departTraffic->m_transfers.size(); it++) {
			jTrafItem["transfer"].append(item->_departTraffic->m_transfers[it]);
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
	jView["idle"];
	if (item && nextItem && (item->_type & (NODE_FUNC_KEY_STATION | NODE_FUNC_KEY_HOTEL )) && (nextItem->_type & (NODE_FUNC_KEY_STATION | NODE_FUNC_KEY_HOTEL ))) {
		int trafTime = 0;
		if (item->_departTraffic) {
			trafTime = item->_departTraffic->_time;
		}
		int idle = nextItem->_arriveTime - item->_departTime - trafTime;
		if (idle >= 3 * 60 * 60) {
			char buff[100];
			if (basePlan->m_qParam.lang != "en") {
				snprintf(buff, sizeof(buff), "自由活动 %s", ToolFunc::NormSeconds(idle).c_str());
			} else {
				snprintf(buff, sizeof(buff), "Unscheduled time %s", ToolFunc::NormSecondsEn(idle).c_str());
			}
			jView["idle"]["desc"] = std::string("自由活动");
			jView["idle"]["dur"] = idle / 900 * 900;
		}
	}
	jView["lock"] = 0;
	jView["commercial"] = 0;
	return 0;
}

int PostProcessor::MakeTrafficJDetail(BasePlan* basePlan, const TrafficDetail* tDetail, Json::Value& jItem) {
	char buff[1024] = {};
	std::string about = "approx ";
	if (basePlan->m_qParam.lang != "en") {
		about = "约";
	}
	jItem["id"] = tDetail->m_trafficItem->_mid;
	if (tDetail->m_trafficItem->_mid.find("Sphere") != std::string::npos) {
		jItem["eval"] = 1;
	} else {
		jItem["eval"] = 0;
	}
	int walkDist = 0;
	Json::Value jSummary;
	jSummary["dist"] = tDetail->m_trafficItem->_dist;
	jSummary["dur"] = tDetail->m_trafficItem->_time;
	if (tDetail->m_trafficItem->_mode == TRAF_MODE_BUS) {
		jSummary["walkDist"] = tDetail->m_walkDist;
	}
	jItem["summary"] = jSummary;
	return 0;
}

std::string PostProcessor::GetDate(Json::Value& jViewList, bool isLastDay, bool isFirstDay) {
	if (jViewList.empty()) return "20140101";
	Json::Value& jView = jViewList[0u];
	Json::Value& jLastView = jViewList[jViewList.size()-1];
	//凌晨入住酒店，日期使用酒店日期-1 非最后一天
	if (jViewList.size() > 1
			and jLastView["type"].asInt() & LY_PLACE_TYPE_HOTEL
			and jLastView["stime"].asString().substr(0,8) == jLastView["etime"].asString().substr(0,8)
			and jLastView["etime"].asString().substr(9,2) < "12"
			and !isLastDay) {
		return MJ::MyTime::datePlusOf(jLastView["stime"].asString().substr(0,8),-1);
	}
	else if (jView["type"].asInt() & LY_PLACE_TYPE_ARRIVE) {
		return jView["stime"].asString().substr(0, 8);
	}
	else if (jView["type"].asInt() & LY_PLACE_TYPE_HOTEL
			and jView["stime"].asString().substr(0, 8) == jView["etime"].asString().substr(0, 8)
			and jViewList.size()>1
			and jView["etime"].asString().substr(9,2) >"12"
			and !isFirstDay) {
		//处理下午入住,且在午夜12点之前从酒店出发的情形
		return MJ::MyTime::datePlusOf(jView["etime"].asString().substr(0,8), 1);
	}
	else {
		return jView["etime"].asString().substr(0, 8);
	}
}

int PostProcessor::SetDayPlayRange (Json::Value& jDay, std::string sTime, std::string eTime) {
	Json::Value& jViewList = jDay["view"];
	jDay["stime"] = sTime;
	jDay["etime"] = eTime;
	if (jViewList.size() == 0) {
		//
	} else {
		if (jViewList.size() == 1) {
			if (jViewList[0u]["type"].asInt() & (LY_PLACE_TYPE_HOTEL | LY_PLACE_TYPE_ARRIVE)) {
				if (sTime == jViewList[0u]["stime"].asString()) {
					jDay["stime"] = jViewList[0u]["etime"];
				} else {
					jDay["etime"] = jViewList[0u]["stime"];
				}
			} else {
				jDay["stime"] = jViewList[0u]["stime"];
				jDay["etime"] = jViewList[0u]["etime"];
			}
		} else {
			if (jViewList[0u]["type"].asInt() & (LY_PLACE_TYPE_HOTEL | LY_PLACE_TYPE_ARRIVE)) {
				jDay["stime"] = jViewList[0u]["etime"];
			} else {
				jDay["stime"] = jViewList[0u]["stime"];
			}
			if (jViewList[jViewList.size()-1]["type"].asInt() & (LY_PLACE_TYPE_HOTEL | LY_PLACE_TYPE_ARRIVE)) {
				jDay["etime"] = jViewList[jViewList.size()-1]["stime"];
			} else {
				jDay["etime"] = jViewList[jViewList.size()-1]["stime"];
			}
		}
	}
	return 0;
}

int PostProcessor::MakeJDay(BasePlan* basePlan, Json::Value& jViewList, Json::Value& jDay, bool isLastDay, bool isFirstDay) {
//	AddExtra(basePlan, jViewList);
	jDay["view"] = jViewList;

	std::string date = "";
	if (!jViewList.empty()) {
		date = GetDate(jViewList, isLastDay, isFirstDay);
	} else {
		if (basePlan->m_PlanList.Length() > 0) {
			date = MJ::MyTime::toString((basePlan->m_PlanList.GetItemIndex(0))->_arriveTime, basePlan->m_TimeZone).substr(0,8);
		} else {
			date = "date error";
		}
	}
	jDay["date"] = date;

	SetDayPlayRange(jDay);
	return 0;
}

//传入一天的景点view  生成poiList
int PostProcessor::MakejDays (const Json::Value& req, const Json::Value& jDayList, Json::Value& jSummaryDaysList) {
	for (int i = 0; i < jDayList.size(); i++) {
		Json::Value jPoisList = Json::arrayValue;
		const Json::Value& jViewList = jDayList[i]["view"];
		jPoisList = Json::arrayValue;
		for (int j = 0; j < jViewList.size(); j++) {
			const Json::Value& jView = jViewList[j];
			Json::Value jPoi = Json::Value();
			jPoi["id"] = jView["id"];
			jPoi["pdur"] = jView["dur"];
			jPoi["type"] = jView["type"];
			if(jView.isMember("play"))
				jPoi["play"] = jView["play"];
			else
				jPoi["play"]="";
			if (jView.isMember("func_type")) {
				jPoi["func_type"] = jView["func_type"];
			} else {
				jPoi["func_type"] = NODE_FUNC_NULL;
			}
			//酒店pdur 保持和test一致
			if (jPoi["pdur"] == -1) {
				std::string stimeStr = jView["stime"].asString();
				std::string etimeStr = jView["etime"].asString();
				int stime = MJ::MyTime::toTime(stimeStr);
				int etime = MJ::MyTime::toTime(etimeStr);
				jPoi["pdur"] = etime - stime;
			}
			if (jView.isMember("product")) {
				jPoi["product"] = jView["product"];
				if (jView["product"].isMember("stime") && jView["product"]["stime"].isString() && jView["product"]["stime"].asString()!="") {
					jPoi["stime"] = jView["stime"].asString();
				}
				if (jView["product"].isMember("product_id") and jView["product"]["product_id"].isString()) {
					std::string productId = jPoi["product"]["product_id"].asString();
					if (req.isMember("product") and req["product"].isMember("wanle") && req["product"]["wanle"].isObject() && req["product"]["wanle"].isMember(productId) && req["product"]["wanle"][productId].isMember("stime") && req["product"]["wanle"][productId]["stime"].isString() && req["product"]["wanle"][productId]["stime"].asString().length() == 5) {
						jPoi["stime"] = jView["stime"].asString();
					}
				}
			}
			if (jView["type"].isInt() && jView["type"].asInt() & (LY_PLACE_TYPE_CAR_STORE | LY_PLACE_TYPE_ARRIVE)) {
				jPoi["stime"] = jView["stime"].asString();
			}
			if (not jView.isMember("func_type")
					and jDayList.size() > 1
					and jPoi["type"].isInt() && jPoi["type"].asInt() & LY_PLACE_TYPE_HOTEL
					and ((i==0  and j==jViewList.size()-1)  //首天
						or (i==jDayList.size()-1 and j==0)  //尾天
						or (i!=0 and i!=jDayList.size()-1 and (j==0 || j==jViewList.size()-1))) //中间天
			   ) {
				jPoi["stime"] = jView["stime"].asString();
				jPoi["etime"] = jView["etime"].asString();
				jPoi["func_type"] = NODE_FUNC_KEY_HOTEL_SLEEP;
			}
			if (jView.isMember("func_type") and jView["func_type"].isInt() and jView["func_type"].asInt() == NODE_FUNC_KEY_HOTEL_SLEEP) {
				jPoi["stime"] = jView["stime"].asString();
				jPoi["etime"] = jView["etime"].asString();
			}
			if (jView.isMember("needAdjustHotel")) jPoi["needAdjustHotel"] = jView["needAdjustHotel"];
			if (jView.isMember("custom") and jView["custom"].isInt() and jView["custom"].asInt() == POI_CUSTOM_MODE_CUSTOM
					and jView["coord"].isString()
					and jView["name"].isString()
					and jView["lname"].isString()) {
				jPoi["coord"] = jView["coord"];
				jPoi["name"] = jView["name"];
				jPoi["lname"] = jView["lname"];
				jPoi["custom"] = jView["custom"];
			}
			jPoisList.append(jPoi);
		}
		Json::Value jDay;
		jDay["expire"] = 0;
		jDay["date"] = jDayList[i]["date"];
		jDay["pois"] = jPoisList;
		jSummaryDaysList.append(jDay);
	}
	return 0;
}

int PostProcessor::MakeJSummary(BasePlan* basePlan, const Json::Value& req, const Json::Value& jWarningList, Json::Value& jDayList, Json::Value& jSummary) {
	jSummary["cntView"] = PathStat::GetViewNum(jDayList);
	jSummary["cntRes"] = PathStat::GetRestNum(jDayList);
	jSummary["cntAct"] = PathStat::GetActNum(jDayList);
	Json::Value jMissPoiList;
	jMissPoiList.resize(0);
	MakeMissPoi(basePlan, jMissPoiList);
	jSummary["missPoi"] = jMissPoiList;
	if(basePlan->m_qParam.type == "s202" || basePlan->m_qParam.type == "ssv006_light" || basePlan->m_qParam.type == "ssv005_rich" || basePlan->m_qParam.type == "ssv007"
			|| basePlan->m_qParam.type == "s125") {
		jSummary["warning"] = jWarningList;
	} else {
		jSummary["warning"] = Json::Value(Json::arrayValue);
	}
	Json::Value jSummaryDaysList = Json::arrayValue;
	MakejDays(req, jDayList, jSummaryDaysList);
	jSummary["days"] = jSummaryDaysList;
	//临时策略
	if (req["city"].isMember("hotel")) jSummary["hotel"] = req["city"]["hotel"];
	if (req["city"].isMember("arv_time")) jSummary["arv_time"] = req["city"]["arv_time"];
	if (req["city"].isMember("dept_time")) jSummary["dept_time"] = req["city"]["dept_time"];
	if (req["city"].isMember("traffic_in")) jSummary["traffic_in"] = req["city"]["traffic_in"];
	return 0;
}

//summary["days"]
int PostProcessor::MakeJDayList(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList) {
	PathView& path = basePlan->m_PlanList;

	Json::Value jViewList;
	jViewList.resize(0);

	int dIdx = 0;
	int vIdx = 0;

	//将伪装点换成原来真实的点
	for(int i=0; i< path.Length(); ++i)
	{
		const PlanItem* item = path.GetItemIndex(i);
		if(item and item->_place )
		{
			if(item->_place->_rawPlace)
			{
				PlanItem * tplanItem = const_cast<PlanItem *>(item);
				tplanItem->_place = item->_place->_rawPlace;
			}
		}
	}

	for (int i = 0; i < path.Length(); ++i) {
		const PlanItem* item = path.GetItemIndex(i);
		const PlanItem* lastItem = path.GetItemIndex(i - 1);
		const PlanItem* nextItem = path.GetItemIndex(i + 1);

		Json::Value jView;
		MakeJView(item, nextItem, basePlan, jView, jWarningList, dIdx, vIdx);
		jViewList.append(jView);
		vIdx++;
		//兼容包车 第0天第0个点为酒店 则补充作站点的hotel不计入vIdx的偏移
		if (jView["id"] == "arvNullPlace") vIdx--;
		// 按hotel切
		if (item->_place->_t & LY_PLACE_TYPE_HOTEL && item->_type & NODE_FUNC_KEY_HOTEL_SLEEP) {
			if (jViewList.size() > 0) {
				jViewList[jViewList.size() - 1]["traffic"] = Json::Value();
				if (jViewList[jViewList.size() - 1]["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
					jViewList[jViewList.size() - 1]["idle"] = Json::Value();
				}
				if (jViewList[0U]["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
					jViewList[0U]["dur"] = 0;
					jViewList[0u]["dining_nearby"] = 0;
				}
			}
			Json::Value jDay;
			if (jDayList.size() == 0) {
				MakeJDay(basePlan, jViewList, jDay, false, true);
			} else {
				MakeJDay(basePlan, jViewList, jDay, false, false);
			}
			jDayList.append(jDay);
			dIdx++;
			vIdx = 0;
			if (jViewList.size() > 0) {
				jViewList.clear();
				jViewList.append(jView);
				vIdx++;
			}
		}
	}

	//add by shyy  删除必去点报错
	DelUseMustPoi(basePlan, jWarningList);

	if (jViewList.size() > 0) {
		jViewList[jViewList.size() - 1]["traffic"] = Json::Value();
		jViewList[jViewList.size() - 1]["idle"] = Json::Value();
		if (jViewList[0U]["type"].asInt() == LY_PLACE_TYPE_HOTEL) {
			jViewList[0U]["dur"] = 0;
			jViewList[0u]["dining_nearby"] = 0;
		}
//		jViewList[0U]["dur"] = 0;
	}
	Json::Value jDay;
	if (jDayList.size() == 0) {
		MakeJDay(basePlan, jViewList, jDay, true, true);
	} else {
		MakeJDay(basePlan, jViewList, jDay, true, false);
	}
	Json::StyledWriter jsw;
	jDayList.append(jDay);
	if (jDayList.size() > 1) {//第一天的 酒店特殊需求 天数大于一生效
		MergeDay1Hotel(basePlan, jDayList[0U]["view"], jDayList[1U]["view"]);
	}
	dIdx++;

	RemoveArvDeptHotel(basePlan, jDayList, jWarningList);

	return 0;
}

int PostProcessor::MergeDay1Hotel(BasePlan* basePlan, Json::Value& jViewList, Json::Value& jSecondDay) {
	bool dayOne17Limit = false;
	Json::StyledWriter jsw;
	if (strcmp(jViewList[0U]["stime"].asString().substr(9).c_str(), "17:00") >= 0) {
		dayOne17Limit = true;
	}
	bool dayOneNoView = true;
	for (int i = 0; i < jViewList.size(); i ++) {
		if (jViewList[i]["type"].asInt() & LY_PLACE_TYPE_VAR_PLACE) {
			dayOneNoView = false;
		}
	}
	bool dayOne3hMergeHotel = false;
	time_t viewStart = 0;
	time_t hotelStart = 0;
	for (int i = 0 ; i < jViewList.size(); i ++) {
		if (jViewList[i]["func_type"].asInt() & (NODE_FUNC_KEY_ARRIVE | NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE)) {
			viewStart = MJ::MyTime::toTime(jViewList[i]["etime"].asString(), basePlan->m_TimeZone);
		}
		if (jViewList[i]["func_type"].asInt() & NODE_FUNC_KEY_HOTEL_SLEEP) {
			hotelStart = MJ::MyTime::toTime(jViewList[i]["stime"].asString(), basePlan->m_TimeZone);
		}
	}
	if (viewStart >= hotelStart - 3 * 3600) {
		dayOne3hMergeHotel = true;
	}
	bool needMerge = dayOne17Limit | dayOne3hMergeHotel;
	if (!needMerge) {
		return 0;
	}
	MJ::PrintInfo::PrintLog("[%s]PostProcessor::MergeDay1Hotel needMerge", basePlan->m_qParam.log.c_str());
	//无景点模式
	//酒店 无行李 衔接到达点 和 酒店
	if (jViewList.size() == 2 && (jViewList[1U]["type"].asInt() & LY_PLACE_TYPE_HOTEL)) {
		time_t oriSTime = MJ::MyTime::toTime(jViewList[1U]["stime"].asString(), basePlan->m_TimeZone);
		time_t arvEndTime = MJ::MyTime::toTime(jViewList[0U]["etime"].asString(), basePlan->m_TimeZone);
		int arvEndTrafT = !jViewList[0U]["traffic"].isNull() ? jViewList[0U]["traffic"]["dur"].asInt() : 0;
		time_t newSTime = arvEndTime + arvEndTrafT;
		int newDur = jViewList[1U]["dur"].asInt() + (oriSTime - newSTime);
		if (newDur >= basePlan->m_MinSleepTimeCost) {//正常 非006容错模式
			jViewList[1U]["dur"] = newDur;
			jViewList[1U]["stime"] = MJ::MyTime::toString(newSTime, basePlan->m_TimeZone);
			jViewList[0U]["idle"] = Json::Value();
			jViewList[1U]["idle"] = Json::Value();
			std::string stime = jViewList[1U]["stime"].asString();
			jSecondDay[0U]["stime"] = stime;
		}
	} else if (jViewList.size() == 3 && (jViewList[1U]["type"].asInt() & jViewList[2U]["type"].asInt() & LY_PLACE_TYPE_HOTEL)) {//含行李
		time_t oriSTime = MJ::MyTime::toTime(jViewList[2U]["stime"].asString(), basePlan->m_TimeZone);
		time_t arvEndTime = MJ::MyTime::toTime(jViewList[0U]["etime"].asString(), basePlan->m_TimeZone);
		int arvEndTrafT = !jViewList[0U]["traffic"].isNull() ? jViewList[0U]["traffic"]["dur"].asInt() : 0;
		time_t newSTime = arvEndTime + arvEndTrafT;
		int newDur = jViewList[2U]["dur"].asInt() + (oriSTime - newSTime);
		if (newDur >= basePlan->m_MinSleepTimeCost) {//正常 非006容错模式
			jViewList[2U]["dur"] = newDur;
			jViewList[2U]["stime"] = MJ::MyTime::toString(newSTime, basePlan->m_TimeZone);
			Json::Value jArv = jViewList[0U];
			Json::Value jHotelSleep = jViewList[2U];
			jViewList.resize(0);
			jViewList.append(jArv);
			jViewList.append(jHotelSleep);
			jViewList[0U]["idle"] = Json::Value();
			jViewList[1U]["idle"] = Json::Value();
			std::string stime = jViewList[1U]["stime"].asString();
			jSecondDay[0U]["stime"] = stime;
		}
	}
	return 0;
}

bool PostProcessor::IsSleepCost(BasePlan* basePlan, Json::Value& jView) {
	if (jView["dur"].asInt() == 0 || jView["dur"].asInt() >= basePlan->m_MinSleepTimeCost) {
		return true;
	}
	return false;
}

int PostProcessor::PerfectHotelDoWhat(Json::Value& jDayList, BasePlan* basePlan) {
	for (int i = 0;i < jDayList.size(); i++) {
		std::vector<int> hotelIdx;
		for (int j = 0; j < jDayList[i]["view"].size(); j++) {
			Json::Value& jView = jDayList[i]["view"][j];
			if (jView["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
				hotelIdx.push_back(j);
			}
		}
		Json::Value& jViewList = jDayList[i]["view"];
		if (hotelIdx.size() == 1) {
			if (i == 0 && jDayList.size() > 1) {
				//办理入住
				Json::Value& jHotel = jViewList[hotelIdx[0]];
				jHotel["do_what"] = "办理入住后休息";
			} else if (i > 0 && i == jDayList.size() - 1) {
				//退房
				Json::Value& jHotel = jViewList[hotelIdx[0]];
				jHotel["do_what"] = "退房离开";
			} else {
				//只有一天的情况
				Json::Value& jHotel = jViewList[hotelIdx[0]];
				jHotel["do_what"] = "办理入住-休息-退房离开";
			}
		}
		if (hotelIdx.size() == 2) {
			if (i == 0 && jDayList.size() > 1) {
				//存行李
				Json::Value& jFirstHotel = jViewList[hotelIdx[0]];
				time_t now = MJ::MyTime::toTime(jFirstHotel["stime"].asString(), basePlan->m_City->_time_zone);
				time_t start = MJ::MyTime::toTime(jFirstHotel["stime"].asString().substr(0,8) + "_00:00", basePlan->m_City->_time_zone);
				int dur = now - start;
				//14点之前存行李 14点之后办理入住
				if (dur < 14 * 3600) {
					jFirstHotel["do_what"] = "行李寄存";
				} else {
					jFirstHotel["do_what"] = "办理入住";
				}
				Json::Value& jLastHotel = jViewList[hotelIdx[1]];
				jLastHotel["do_what"] = "返回休息";
			} else if (i > 0 && i < jDayList.size()-1) {
				//中间天
				if (jViewList[hotelIdx[0]]["id"].asString() != jViewList[hotelIdx[1]]["id"].asString()) {
					// 退房
					Json::Value& jFirstHotel = jViewList[hotelIdx[0]];
					jFirstHotel["do_what"] = "退房离开";
					// 办理入住
					Json::Value& jLastHotel = jViewList[hotelIdx[1]];
					jLastHotel["do_what"] = "办理入住后休息";
				} else {//不同的需求 时间不同
					//A 出发游玩
					Json::Value& jFirstHotel = jViewList[hotelIdx[0]];
					jFirstHotel["do_what"] = "出发游玩";
					//B 休息
					Json::Value& jLastHotel = jViewList[hotelIdx[1]];
					jLastHotel["do_what"] = "返回休息";
				}
			}
			 else if (i > 0 && i == jDayList.size()-1) {
				//取行李
				Json::Value& jLastHotel = jViewList[hotelIdx[1]];
				jLastHotel["do_what"] = "取行李";
				Json::Value& jFirstHotel = jViewList[hotelIdx[0]];
				jFirstHotel["do_what"] = "出发游玩";
			} else {
				MJ::PrintInfo::PrintErr("the day has more than one hotel, didx: %d", i);
			}
		}
	}
	return 0;
}

int PostProcessor::AddDoWhat(Json::Value& jDayList, BasePlan* basePlan) {
	for (int i = 0;i < jDayList.size(); i++) {
		int dayHotelNum = 0;
		for (int j = 0; j < jDayList[i]["view"].size(); j++) {
			Json::Value& jView = jDayList[i]["view"][j];
			if (jView["type"].asInt() & LY_PLACE_TYPE_ARRIVE) {
				if (jView.isMember("func_type") and jView["func_type"].isInt()) {
					if (jView["func_type"].asInt() == NODE_FUNC_KEY_ARRIVE) {
						if (jView["type"].asInt() & LY_PLACE_TYPE_AIRPORT) {//机场
							jView["do_what"] = "取行李";
						} else {
							jView["do_what"] = "出站";
						}
					} else if (jView["func_type"].asInt() == NODE_FUNC_KEY_DEPART) {
						if (jView["type"].asInt() & LY_PLACE_TYPE_AIRPORT) {//机场
							jView["do_what"] = "办理登机";
						} else if (jView["type"].asInt() & LY_PLACE_TYPE_SAIL_STATION) {
							jView["do_what"] = "验票上船";
						} else {
							jView["do_what"] = "验票上车";
						}
					}
				}
			} else if (jView["type"].asInt() & LY_PLACE_TYPE_CAR_STORE) {
				const LYPlace* place = basePlan->GetLYPlace(jView["id"].asString());
				if(place)
				{
					if(jView["info"].isNull()) jView["info"]=Json::Value();
					jView["info"]["mode"]=place->m_mode;
					jView["info"]["corp"]=place->_corp;
					if(jView["product"].isNull()) jView["product"]=Json::Value();
					jView["product"]["product_id"]=place->_pid;
					jView["product"]["unionkey"]=place->_unionkey;
					if(place->m_mode==0){
						jView["do_what"] = "取车";
					}
					if(place->m_mode==1){
						jView["do_what"] = "还车";
					}
				}
			} else if (jView["type"].asInt() & LY_PLACE_TYPE_RESTAURANT) {
				int beginOffset = 0;
				int endOffset = 0;
				ToolFunc::toIntOffset(jView["stime"].asString().substr(9), beginOffset);
				ToolFunc::toIntOffset(jView["etime"].asString().substr(9), endOffset);
				time_t sTime = MJ::MyTime::toTime(jView["stime"].asString(), basePlan->m_TimeZone);
				time_t eTime = MJ::MyTime::toTime(jView["etime"].asString(), basePlan->m_TimeZone);
				int dur = eTime - sTime;
				if (beginOffset >= BasePlan::m_lunchTime._begin && beginOffset < BasePlan::m_lunchTime._end && endOffset > BasePlan::m_lunchTime._begin && dur >= 900 || //头触碰就餐时间点 且时间大于15分钟就餐
					endOffset <= BasePlan::m_lunchTime._end && endOffset > BasePlan::m_lunchTime._begin && beginOffset < BasePlan::m_lunchTime._end && dur >= 900) { //尾触碰就餐时间点 且时间大于15分钟就餐
					jView["do_what"] = "午餐";
				} else if (beginOffset >= BasePlan::m_supperTime._begin && beginOffset < BasePlan::m_supperTime._end && endOffset > BasePlan::m_supperTime._begin && dur >= 900  || //头触碰就餐时间点 且时间大于15分钟就餐
					endOffset <= BasePlan::m_supperTime._end && endOffset > BasePlan::m_supperTime._begin && beginOffset < BasePlan::m_supperTime._end && dur >= 900) { //尾触碰就餐时间点 且时间大于15分钟就餐
					jView["do_what"] = "晚餐";
				} else {
					jView["do_what"] = "就餐";
				}
				if (basePlan->m_qParam.dev != 0) {
					jView["do_what"] = jView["do_what"].asString() +  ToolFunc::NormSeconds(jView["dur"].asInt());
				}
			} else if (jView["type"].asInt() & LY_PLACE_TYPE_SHOP) {
				jView["do_what"] = "购物";
			} else if (jView["type"].asInt() & LY_PLACE_TYPE_VIEW) {
				jView["do_what"] = "游玩";
			} else if (jView["id"].asString() == "attach" ) {
				jView["do_what"] = "就餐";
			} else if (jView["type"].asInt() == LY_PLACE_TYPE_PLAY			//play
					|| jView["type"].asInt() == LY_PLACE_TYPE_ACTIVITY) {	//活动
				jView["do_what"] = "游玩";
			}
		}
	}
//	std::cerr << "smz type " << basePlan->m_travalType << std::endl;
		int ret = PerfectHotelDoWhat(jDayList, basePlan);
		if (ret) {
			MJ::PrintInfo::PrintErr("[%s]PostProcessor::AddDoWhat, PerfectDoWhat err", basePlan->m_qParam.log.c_str());
			return 1;
		}
	return 0;
}

int PostProcessor::MakeMissPoi(BasePlan* basePlan, Json::Value& jMissPoiList) {
	std::tr1::unordered_set<const LYPlace*> planPlaceList;
	for (int i = 0; i < basePlan->m_PlanList.Length(); i++) {
		const PlanItem* pItem = basePlan->m_PlanList.GetItemIndex(i);
		planPlaceList.insert(pItem->_place);
	}
	if (!basePlan->m_userDelSet.empty()) {
		for (std::tr1::unordered_set<const LYPlace*>::iterator it = basePlan->m_userDelSet.begin(); it != basePlan->m_userDelSet.end(); ++it) {
			const LYPlace* place = *it;
			if (planPlaceList.find(*it) != planPlaceList.end()) {
				continue;
			}
			Json::Value jPoi;
			jPoi["id"] = basePlan->GetCutId(place->_ID);
			jPoi["name"] = place->_name;
			if(place->_name == "") jPoi["name"] = place->_lname;
			//jMissPoiList.append(Json::Value(place->_name));
			jMissPoiList.append(jPoi);
		}
	}
	return 0;
}


int PostProcessor::MakeOutputSSV005(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	if (basePlan->m_error.m_errID != 0) return 0;
	PathView& path = basePlan->m_PlanList;

	Json::StyledWriter jsw;
	Json::Value jDayList;
	jDayList.resize(0);
	Json::Value jWarningList;
	jWarningList.resize(0);
	MakeJDayList(basePlan, jDayList, jWarningList);
	AddDoWhat(jDayList, basePlan);
	//AddResult 智能优化时拼结果(notPlanDates.size != 0时)，同时FixHotelTime
	if (!req.isMember("list") || !req["list"].isArray() || !req["list"].size() > 0) {
		Json::Value jReqDayList = req["city"]["view"]["day"];
		jDayList = AddResult(jDayList,jReqDayList,basePlan->m_notPlanDateSet);
	}
	else {
		for(int i = 0; i < req["list"].size(); i++) {
			if(req["list"][i].isMember("view") && req["list"][i]["view"].isMember("day")){
				Json::Value jReqDayList = req["list"][i]["view"]["day"];
				jDayList = AddResult(jDayList,jReqDayList,basePlan->m_notPlanDateSet);
			}
		}

	}
	Json::Value jSummary;

	MakeJSummary(basePlan, req, jWarningList, jDayList, jSummary);

	Json::Value jRView;
	jRView["day"] = jDayList;
	jRView["expire"] = 0;
	jRView["summary"] = jSummary;

	Json::Value jCity = req["city"];
	jCity["view"] = jRView;

	resp["data"]["city"] = jCity;
	Json::Value servInfo;
	MakeServInfo(basePlan, servInfo);
	resp["servInfo"] = servInfo;
	return 0;
}

//deptHotel 为当天早上出发酒店时间  lastNightHotel为前一晚返回酒店时间
//当天view.hotel poi.hotel  前一天view.hotel poi.hotel
//bool PostProcessor::FixHotelTime (Json::Value& jDeptHotel, Json::Value& jLastNightHotel) {
bool PostProcessor::FixHotelTime (Json::Value& jDeptHotel, Json::Value& jDeptHotelPoi, Json::Value& jLastNightHotel, Json::Value& jLastNightHotelPoi) {
	std::string stime = jLastNightHotel["stime"].asString();
	std::string etime = jDeptHotel["etime"].asString();
	jDeptHotel["stime"] = stime;
	jLastNightHotel["etime"] = etime;
	jDeptHotel["dur"] = -1;
	jLastNightHotel["dur"] = -1;
	if (jDeptHotelPoi != Json::nullValue && jLastNightHotelPoi != Json::nullValue) {
		jDeptHotelPoi["stime"] = stime;
		jLastNightHotelPoi["etime"] = etime;
		int dur = MJ::MyTime::toTime(etime) - MJ::MyTime::toTime(stime);
		jDeptHotelPoi["pdur"] = dur;
		jLastNightHotelPoi["pdur"] = dur;
	}
	return true;
}
//从jDayList中取符合Dates中的日期的结果与jBaseDayList中的结果拼接
Json::Value PostProcessor::AddResult(const Json::Value jBaseDayList, const Json::Value jDayList,std::tr1::unordered_set<std::string> Dates) {
	Json::Value resp;
	using namespace std;
	using namespace std::tr1;
	if (jBaseDayList.size() != 0) {
		int j = 0;
		for(int i = 0; i < jBaseDayList.size(); i++) {
			std::string date = jBaseDayList[i]["date"].asString();
			if(Dates.count(date)) {
				for (;j<jDayList.size();j++) {
					std::string jDate = jDayList[j]["date"].asString();
					if (jDate == date){
						resp.append(jDayList[j]);
						break;
					}
				}
			} else {
				resp.append(jBaseDayList[i]);
			}
		}
	} else {
		resp = jDayList;
	}
	if (resp.size() > 1) {
		for (int i = 1; i < resp.size(); i++) {
			Json::Value& lastDayViewList = resp[i-1]["view"];
			Json::Value& theDayViewList = resp[i]["view"];
			//前一晚酒店 当天的酒店 下一天酒店均需要修改
			if (lastDayViewList.size()>0 && theDayViewList.size()>0) {
				Json::Value& lastDayHotel = lastDayViewList[lastDayViewList.size()-1];
				Json::Value& theDayDeptHotel = theDayViewList[0u];
				//当天view.hotel poi.hotel  前一天view.hotel poi.hotel
				Json::Value lastDayHotelPoi = Json::nullValue;;
				Json::Value theDayHotelPoi = Json::nullValue;;
				FixHotelTime(theDayDeptHotel, theDayHotelPoi, lastDayHotel, lastDayHotelPoi);
			}
		}
	}
	return resp;
}

int PostProcessor::MakeOutputP202(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	Json::Value jData;
	char buff[1000]={0};
	jData["total"] = static_cast<int>(basePlan->m_showItemList.size());
	Json::Value jList;
	jList.resize(0);
	int page = 0;
	int itemNum = 0;
	int itemSum = 0;
	if (basePlan->m_key != "") {
		for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
			const ShowItem& showItem = basePlan->m_showItemList[i];
			const LYPlace* place = showItem.GetPlace();
			const VarPlace* vp = dynamic_cast<const VarPlace*>(place);
			if (vp == NULL) {
				MJ::PrintInfo::PrintErr("not a varPlace %s", place->_ID.c_str());
				return 1;
			}
			const LYPlace* city = NULL;
			if(basePlan->m_listCenter.size() != 0)
				city = LYConstData::GetLYPlace(basePlan->m_listCenter[0], basePlan->m_qParam.ptid);
			Json::Value jPlaceItem;
			Json::Value& jPlaceInfo = jPlaceItem;
			jPlaceInfo["id"] = vp->_ID;
			jPlaceInfo["name"] = vp->_name;
			jPlaceInfo["lname"] = vp->_enname;
			jPlaceInfo["custom"] = vp->m_custom;
			jPlaceInfo["dur"] = basePlan->GetRcmdDur(vp);
			if (city != NULL) {
				jPlaceInfo["dist"] = LYConstData::CaluateSphereDist(vp, city) ==0.0 ? 0:int(LYConstData::CaluateSphereDist(vp, city) + 1);
			} else {
				jPlaceInfo["dist"] = rand() % int(basePlan->m_maxDist);
			}
			snprintf(buff, 1000, "%.1lf", vp->_grade);
			jPlaceInfo["score"] = std::string(buff);
			jPlaceItem["mode"] = place->_t;
			jList.append(jPlaceItem);
		}
	} else {
		for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
			if (page == basePlan->m_pageIndex && itemNum < basePlan->m_numEachPage) {
				const ShowItem& showItem = basePlan->m_showItemList[i];
				const LYPlace* place = showItem.GetPlace();
				const VarPlace* vp = dynamic_cast<const VarPlace*>(place);
				if (vp == NULL) {
					MJ::PrintInfo::PrintErr("not a varPlace %s", place->_ID.c_str());
					return 1;
				}
				const LYPlace* city = NULL;
				if(basePlan->m_listCenter.size() != 0)
					city = LYConstData::GetLYPlace(basePlan->m_listCenter[0], basePlan->m_qParam.ptid);
				Json::Value jPlaceItem;
				Json::Value& jPlaceInfo = jPlaceItem;
				jPlaceInfo["id"] = vp->_ID;
				jPlaceInfo["name"] = vp->_name;
				jPlaceInfo["lname"] = vp->_enname;
				jPlaceInfo["custom"] = vp->m_custom;
				jPlaceInfo["dur"] = basePlan->GetRcmdDur(vp);
				if (city != NULL) {
					jPlaceInfo["dist"] = LYConstData::CaluateSphereDist(vp, city) ==0.0 ? 0:int(LYConstData::CaluateSphereDist(vp, city) + 1);
				} else {
					jPlaceInfo["dist"] = rand() % int(basePlan->m_maxDist);
				}
				snprintf(buff, 1000, "%.1lf", vp->_grade);
				jPlaceInfo["score"] = std::string(buff);
				jPlaceItem["mode"] = place->_t;
				jList.append(jPlaceItem);
				++itemSum;
				if (itemSum == basePlan->m_numEachPage) {
					break;
				}
			}
			++itemNum;
			if (itemNum == basePlan->m_numEachPage) {
				itemNum = 0;
				++page;
			}
		}
	}
	jData["list"] = jList;
	resp["data"] = jData;
	basePlan->m_dbg.SetPlaceList(basePlan->m_error.m_errID, basePlan->m_error.m_errReason, basePlan->m_showItemList.size());
	resp["dbg"] = basePlan->m_dbg.GetDbgStr();
	return 0;
}

//性能统计先保留
//目前无用
int PostProcessor::MakeServInfo(BasePlan* basePlan, Json::Value& servInfo) {
	Json::Value jMissPois = Json::Value();
	jMissPois.resize(0);
	if (!basePlan->m_userDelSet.empty()) {
		for (std::tr1::unordered_set<const LYPlace*>::iterator it = basePlan->m_userDelSet.begin(); it != basePlan->m_userDelSet.end(); ++it) {
			const LYPlace* place = *it;
			Json::Value jPoi;
			jPoi["id"] = BasePlan::GetCutId(place->_ID);
			jPoi["name"] = place->_name;
			if (place->_name == "") {
				jPoi["name"] = place->_enname;
			}
			jMissPois.append(jPoi);
		}
	}
	servInfo["missPoi"] = jMissPois;
}

int PostProcessor::RemoveArvDeptHotel(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList) {
	if (jDayList.empty()) return 1;
	Json::StyledWriter jsw;
	int firstPos = 0;
	Json::Value& jFirstDay = jDayList[firstPos];
	if (jFirstDay.isMember("view") && !jFirstDay["view"].empty()) {
		Json::Value& jFirstView = jFirstDay["view"][firstPos];
		if (jFirstView["id"].asString() == "arvNullPlace") {
			//更新view
			Json::Value jViewList;
			jViewList.resize(0);
			for (int i = firstPos + 1; i < jFirstDay["view"].size(); ++i) jViewList.append(jFirstDay["view"][i]);
			jFirstDay["view"] = jViewList;
			/*更新时间信息*/
			//stime, etime 考虑多天和只有一天的情况
			if (!jViewList.empty())	{
				std::string stimeS;
				std::string etimeS;
				stimeS = jViewList[0u]["stime"].asString();	//实际上起码有一个点
				if (jViewList[jViewList.size() - 1]["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
					etimeS = jViewList[jViewList.size() - 1]["stime"].asString();
				} else {
					etimeS = jViewList[jViewList.size() - 1]["etime"].asString();
				}
				jFirstDay["stime"] = stimeS;
				jFirstDay["etime"] = etimeS;
			}
		}
	}
	Json::Value& jLastDay = jDayList[jDayList.size() - 1];
	if (jLastDay.isMember("view") && !jLastDay["view"].empty()) {
		//最后一个充作站点的酒店需删除
		int len = jLastDay["view"].size();
		Json::Value& jLastView = jLastDay["view"][len - 1];
		if (jLastView["id"].asString() == "deptNullPlace") {
			int delNum = 1;
			//更新view
			Json::Value jViewList;
			jViewList.resize(0);
			for (int i = 0; i < len - delNum; ++i) jViewList.append(jLastDay["view"][i]);
			int newLen = jViewList.size();
			if (newLen > 0) {
				Json::Value& jNewlastView = jViewList[newLen - 1];
				if (jNewlastView.isMember("traffic")) jNewlastView["traffic"] = Json::Value();
			}
			jLastDay["view"] = jViewList;
			/*更新时间信息*/
			//获取stimeS和etimeS 即对GetRange的修
			if (!jViewList.empty()) {
				std::string stimeS;
				std::string etimeS;
				if (jDayList.size() == 1) {
					stimeS = jViewList[0u]["stime"].asString();//酒店或机场或景点
				} else {
					stimeS = jViewList[0u]["etime"].asString();//酒店
				}
				etimeS = jViewList[jViewList.size() - 1]["etime"].asString();//只有一天则无酒店
				std::string sDate = stimeS.substr(0, 8);
				std::string eDate = etimeS.substr(0, 8);
				if (sDate != eDate) {
					etimeS = sDate + "_23:59";
				}
				jLastDay["stime"] = stimeS;
				jLastDay["etime"] = etimeS;
			}
		}
	}
	return 0;
}


int PostProcessor::SetDurNegative1ForCList(Json::Value& resp) {
	int ret = 0;
	if (resp.isMember("data") && resp["data"].isMember("list")) {
		Json::Value& jList = resp["data"]["list"];
		for (int c = 0; c < jList.size(); ++c) {
			Json::Value& jCity = jList[c];
			if (!jCity["view"].isNull()) SetDurNegative1(jCity["view"]["day"]);
		}
	}
	return 0;
}

int PostProcessor::SetDurNegative1(Json::Value& jDays) {
	int ret = 0;
	for (int i = 0; i < jDays.size(); ++i) {
		Json::Value& jDay = jDays[i];
		if (jDay.isMember("view")) {
			for (int j = 0; j < jDay["view"].size(); ++j) {
				Json::Value& jView = jDay["view"][j];
				if (jView.isMember("do_what") &&
					(jView["do_what"].asString() == "返回休息" ||
					jView["do_what"].asString() == "出发游玩" ||
					jView["do_what"].asString() == "退房离开" ||
					jView["do_what"].asString() == "办理入住后休息" ||
					jView["do_what"].asString() == "办理入住-休息-退房离开" ||
					jView["do_what"].asString() == "办理入住-休息-出发游玩" ||
					jView["do_what"].asString() == "返回休息-退房离开")) {
					jView["dur"] = -1;
				}
			}
		}
	}
	return 0;
}

int PostProcessor::MakeP104Node(const LYPlace *place, Json::Value &jPlaceInfo) {
	const VarPlace *vp = dynamic_cast<const VarPlace*>(place);
	if (vp == NULL) {
		MJ::PrintInfo::PrintErr("not a varPlace %s", place->_ID.c_str());
		return 1;
	}
	jPlaceInfo["rec"] = "Hello PHP";
	jPlaceInfo["id"] = vp->_ID;
	jPlaceInfo["name"] = vp->_name;
	jPlaceInfo["mode"] = vp->_t;
    jPlaceInfo["coord"] = vp->_poi;
	jPlaceInfo["custom"] = vp->m_custom;
	jPlaceInfo["realPrice"] = Json::nullValue;
	float price = 0;
	const TicketsFun* ticket = NULL;
	LYConstData::GetBottomTicketAndPrice(vp, ticket, price);

	jPlaceInfo["realPrice"]["sid"] = ticket->m_sid;
	if (ticket == NULL) {
		jPlaceInfo["realPrice"]["ccy"] = "CNY";
		jPlaceInfo["realPrice"]["price"] = "-1";
		return 0;
		//return 1;
	}
	jPlaceInfo["realPrice"]["ccy"] = ticket->m_ccy;

	std::stringstream s;
	s <<std::setiosflags(std::ios::fixed)<<std::setprecision(2)<< price;
	std::string str;
	s >> str;
	jPlaceInfo["realPrice"]["price"] = str;
	return 0;
}

int PostProcessor::MakeOutputP104(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	Json::Value jData;
	char buff[1000]={0};
	Json::Value jList;
	jList.resize(0);
	int page = 0;
	int itemNum = 0;
	int itemSum = 0;
	int total =0;
	for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
		const ShowItem& showItem = basePlan->m_showItemList[i];
		Json::Value jPlaceItem;
		if(MakeP104Node(showItem.GetPlace(), jPlaceItem)) {
			continue;
		}
		total++;
	}
	jData["total"] = total;
	if (basePlan->m_key != "") {
		for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
			const ShowItem& showItem = basePlan->m_showItemList[i];
			Json::Value jPlaceItem;
			if(MakeP104Node(showItem.GetPlace(), jPlaceItem)) {
				continue;
			}
			jList.append(jPlaceItem);
		}
	} else {
		for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
			if (page == basePlan->m_pageIndex && itemNum < basePlan->m_numEachPage) {
				const ShowItem& showItem = basePlan->m_showItemList[i];
				Json::Value jPlaceItem;
				if (MakeP104Node(showItem.GetPlace(), jPlaceItem)) {
					continue;
				}
				jList.append(jPlaceItem);
				++itemSum;
				if (itemSum == basePlan->m_numEachPage) {
					break;
				}
			}
			++itemNum;
			if (itemNum == basePlan->m_numEachPage) {
				itemNum = 0;
				++page;
			}
		}
	}
	jData["list"] = jList;
	resp["data"] = jData;
	basePlan->m_dbg.SetPlaceList(basePlan->m_error.m_errID, basePlan->m_error.m_errReason, basePlan->m_showItemList.size());
	resp["dbg"] = basePlan->m_dbg.GetDbgStr();
	return 0;
}

int PostProcessor::MakeOutputP105(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	Json::Value jData;
	char buff[1000]={0};
	Json::Value jList;
	jList.resize(0);

	for (int i = 0; i < basePlan->m_POIList.size(); ++i) {
		const auto& POIItem = basePlan->m_POIList[i];
		const LYPlace* place = POIItem.GetPlace();
		if (place == NULL ) {
			MJ::PrintInfo::PrintErr("do not get varPlace ");
			return 1;
		}
		const Tour* vp = dynamic_cast<const Tour*>(place);
		if (vp == NULL) {
			MJ::PrintInfo::PrintErr("not a varPlace %s", place->_ID.c_str());
			return 1;
		}
		Json::Value jPlaceItem;
		Json::Value& jPlaceInfo = jPlaceItem;
		Json::Reader jr;
		jPlaceInfo["rec"] = "Hello PHP";
		jPlaceInfo["id"] = vp->_ID;
		jPlaceInfo["name"] = vp->_name;
		jPlaceInfo["lname"] = vp->_enname;
		jPlaceInfo["mode"] = vp->_t;
		if (vp->_t & LY_PLACE_TYPE_VIEWTICKET) {
			const ViewTicket* vt = dynamic_cast<const ViewTicket*>(vp);
			if (vt != NULL) {
				//jPlaceInfo["refId"] = vt->Getm_view();
				const LYPlace* viewPlace = LYConstData::GetViewByViewTicket(const_cast<ViewTicket*>(vt),basePlan->m_qParam.ptid);
				if (viewPlace) jPlaceInfo["refId"] = viewPlace->_ID;
			}
		}
		jPlaceInfo["custom"] = vp->m_custom;
		//rule
		if (vp->m_open.isArray()) jPlaceInfo["rule"] = vp->m_open;
		else jPlaceInfo["rule"] = Json::arrayValue;
		Json::Value date;
		for (int idate = 0; ; ++idate) {
			const std::string newDate = MJ::MyTime::datePlusOf(POIItem.m_sDate, idate);
			date["date"] = newDate;
			date["week"] = TimeIR::getWeekByDate(newDate)-1;
			if (LYConstData::IsTourAvailable(vp,newDate)) {
				date["valid"] = 1;
			} else {
				date["valid"] = 0;
			}
			//tickets
			{
				std::vector<const TicketsFun*> tickets;
				const TicketsFun* ticket = NULL;
				date["tickets"] = Json::arrayValue;
				LYConstData::GetProdTicketsListByPlace(vp, tickets);
				for (int idx = 0; idx < tickets.size(); ++idx) {
					Json::Value ticket;
					if (LYConstData::MakeTicketSource(vp, newDate, tickets[idx], ticket["source"]) == 0) {
						date["tickets"].append(ticket);
					}
				}
			}

			jPlaceInfo["dates"].append(date);
			if (newDate == POIItem.m_eDate) {
				break;
			}
		}

		jr.parse(vp->m_srcJieSongPOI, jPlaceInfo["jiesongPoi"]);
		if (vp->m_srcTimes.isArray() && vp->m_srcTimes.size() > 0) {
			for (int k = 0; k < vp->m_srcTimes.size(); ++k) {
				if (vp->m_srcTimes[k]["t"] != "") {
					jPlaceInfo["times"][k]["time"] = vp->m_srcTimes[k]["t"];
				}
				else {
					jPlaceInfo["times"] = Json::Value();
					break;
				}
			}
		}
		else {
			jPlaceInfo["times"] = Json::Value();
		}
		jList.append(jPlaceItem);
	}
	jData["list"] = jList;
	resp["data"] = jData;
	basePlan->m_dbg.SetPlaceList(basePlan->m_error.m_errID, basePlan->m_error.m_errReason, basePlan->m_POIList.size());
	resp["dbg"] = basePlan->m_dbg.GetDbgStr();
	return 0;
}

int PostProcessor::MakeOutputP201(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	std::cerr<<"in MakeOutputP201"<<std::endl;
	Json::Value jData;
	Json::Value jList;
	jList.resize(0);
	for (int i = 0; i < basePlan->m_showItemList.size(); ++i) {
		const ShowItem& showItem = basePlan->m_showItemList[i];
		const LYPlace* place = showItem.GetPlace();
		Json::Value jPlaceItem;
		Json::Value& jPlaceInfo = jPlaceItem;
		jPlaceInfo["id"] = place->_ID;
		jPlaceInfo["name"] = place->_name;
		jPlaceItem["mode"] = place->_t;
		jList.append(jPlaceItem);
	}
	jData["list"] = jList;
	resp["data"] = jData;
	basePlan->m_dbg.SetPlaceList(basePlan->m_error.m_errID, basePlan->m_error.m_errReason, basePlan->m_showItemList.size());
	resp["dbg"] = basePlan->m_dbg.GetDbgStr();
	return 0;
}
int PostProcessor::MakeOutputB116(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	Json::Value jData;
	char buff[1000]={0};
	Json::Value jList;
	jList.resize(0);

	for (int i = 0; i < basePlan->m_productList.size(); ++i) {
		const auto& productTicket = basePlan->m_productList[i];
		const std::string ticketDate = productTicket.date;

		Json::Value jPlaceItem;
		Json::Value& jPlaceInfo = jPlaceItem;
		Json::Reader jr;
		jPlaceInfo["rec"] = "Hello PHP";
		const Tour *tour = dynamic_cast<const Tour* >(LYConstData::GetLYPlace(BasePlan::GetCutId(productTicket.productId),basePlan->m_qParam.ptid));
		if (tour == NULL) {
			resp["error"]["error_id"] = 50116;
			resp["error"]["error_str"] = "数据已被删除，无法修改";
			resp["error"]["error_reason"] = "数据已被删除，无法修改";
			return 0;
		}
		std::string sql;
		std::vector<std::vector<std::string> > results;
		if (tour && tour->m_custom == POI_CUSTOM_MODE_CONST) {
			LYConstData::GetConstTickets(BasePlan::GetCutId(productTicket.productId),productTicket.ticketId,results);
		}
		else {
			if (productTicket.ticketId != "") {
				sql = "select id,ticket_id,pid,name,info,ccy,'' as ticket_3rd from tickets_fun where pid=\""+BasePlan::GetCutId(productTicket.productId)+"\" and id="+productTicket.ticketId+" and disable = 0;";
			} else {
				sql = "select id,ticket_id,pid,name,info,ccy,'' as ticket_3rd from tickets_fun where pid=\""+BasePlan::GetCutId(productTicket.productId)+"\" and disable = 0;";
			}
			LYConstData::GetPrivateData(sql, results);
		}

		jPlaceInfo["product_id"] = productTicket.productId;
		jPlaceInfo["finish"] = 1;
		jPlaceInfo["verify"].resize(0);
		for (int j = 0; j < results.size(); ++j) {
			Json::Value verify;
			verify["sellout"] = 0;

			Json::Value source;
			if (tour->m_custom == POI_CUSTOM_MODE_CONST) {
				//if (LYConstData::IsApi(tour->m_sid)) {
				//	source["sourceType"] = 2;
				//} else {
				//	source["sourceType"] = 1;
				//}
				source["sourceType"] = 2;
			} else if (tour->m_custom == POI_CUSTOM_MODE_PRIVATE) {
				source["sourceType"] = 3;
			}
			source["utime"] = int(MJ::MyTime::getNow());
			source["product"] = 0x80; //代表玩乐
			source["sid"] = tour->m_sid;
			source["sname"] = LYConstData::GetSourceNameBySid(tour->m_sid);
			source["miojiBuy"] = 0;
			source["refund"] = 1;
			source["apiAcc"] = 1;
			source["book_pre"] = tour->m_preBook;
			source["times"] = tour->m_srcTimes;
			std::string date = "";
			Json::FastWriter jw;
			if (tour->m_open.isArray() && tour->m_open.size()>0) date = jw.write(tour->m_open);
			source["date"] = date;
			source["disable"] = 0;

			if (tour) {
				source["name"] = tour->_name;
				source["lname"] = tour->_lname;
				source["pid_3rd"] = tour->m_pid_3rd;
			}

			enum {id,ticket_id,pid,name,info,ccy,ticket_3rd};
			Json::Reader jr;
			jr.parse(results[j][info], source["t_apply"]);
			int num = 0;
			for (int k = 0; k < source["t_apply"]["info"].size(); ++k) {
				if (source["t_apply"]["info"][k]["num"].isInt()) {
					num += source["t_apply"]["info"][k]["num"].asInt();
				}
			}
			source["pid"] = results[j][pid];
			source["t_num"] = num;
			source["ticketId"] = atoi(results[j][id].c_str());
			source["ticket_id"] = results[j][ticket_id];
			source["t_name"] = results[j][name];
			source["realPrice"]["ccy"] = results[j][ccy];
			source["ticket_3rd"] = results[j][ticket_3rd];

			verify["failed"] = 3;
			const TicketsFun* ticket = NULL;
			LYConstData::GetProdTicketsByPlaceAndId(tour, source["ticketId"].asInt(), ticket);
			std::string str="";
			if (ticket) {
				if(tour->m_ptid.empty() and not LYConstData::IsSourceAvailable(basePlan->m_qParam.ptid,ticket->m_sid)
					or not tour->m_ptid.empty() and not LYConstData::IsSourceAvailable(tour->m_ptid,ticket->m_sid)) continue;
				//获取价格
				float price = 0;
				if (LYConstData::GetTicketPriceOfTheDate(ticket, ticketDate, price)) {
					std::stringstream s;
					s << std::setiosflags(std::ios::fixed)<<std::setprecision(2) << price;
					s >> str;
					verify["failed"] = 0;
				} else {
					str = "-1";
				}
			}
			else
			{
				continue;
			}
			source["realPrice"]["val"] = str;
			//保证source唯一
			struct timeval tv;
			gettimeofday(&tv, NULL);
			long long timeofusec = tv.tv_sec*1000000 + tv.tv_usec;
			srand(time(NULL));
			source["unionkey"] = source["pid"].asString() + "#" + source["ticketId"].asString() + "#" + std::to_string(timeofusec) + "#" + std::to_string(rand());

			if (tour) {
				std::string warning;
				int t = MJ::MyTime::toTime(productTicket.date);
				t -= tour->m_preBook;
				int now = MJ::MyTime::getNow();
				if (tour->m_preBook && t < now) {
					warning = "此产品需要提前" + std::to_string(tour->m_preBook/86400) + "天预订，现已不可预订";
					source["warning"] = warning;
				}
			}
			verify["source"] = source;
			jPlaceInfo["verify"].append(verify);
		}
		if(jPlaceInfo["verify"].size() == 0 or not LYConstData::IsTourAvailable(tour, ticketDate))
		{
			Json::Value verify;
			verify["source"] = Json::nullValue;
			verify["sellout"] = 0;
			verify["failed"] = 1;
			jPlaceInfo["verify"].append(verify);
		}
		if (tour) {
			for(int i = 0; i < tour->m_srcTimes.size(); i ++) {
				std::string time = tour->m_srcTimes[i]["t"].asString();
				if (time == "") {
					jPlaceInfo["times"].resize(0);
					break;
				}
				Json::Value jTime;
				jTime["time"] = time;
				jPlaceInfo["times"].append(jTime);
			}
		}
		if (req["wanle"][i].isMember("key") && req["wanle"][i]["key"].isString()) {
			jPlaceItem["key"] = req["wanle"][i]["key"];
		}
		jList.append(jPlaceItem);
	}
	jData["wanle"] = jList;
	resp["data"] = jData;
	//basePlan->m_dbg.SetPlaceList(basePlan->m_error.m_errID, basePlan->m_error.m_errReason, basePlan->m_productList.size());
	//resp["dbg"] = basePlan->m_dbg.GetDbgStr();
	return 0;
}

//add by shyy  删除必去点报错
int PostProcessor::DelUseMustPoi(BasePlan* basePlan, Json::Value& jWarningList) {
	std::vector<const LYPlace* > delMustPlaceList;
	if (!basePlan->m_userDelSet.empty()) {
		for(auto it = basePlan->m_userDelSet.begin(); it != basePlan->m_userDelSet.end(); it++) {
			if ((*it)->_t & LY_PLACE_TYPE_TOURALL) {
				delMustPlaceList.insert(delMustPlaceList.begin(), *it);
			} else {
				delMustPlaceList.push_back(*it);
			}
		}
	}
	if (delMustPlaceList.size() > 0) {
		Json::Value jWarning;
		jWarning["didx"] = -1;
		jWarning["vidx"] = -1;
		jWarning["type"] = 7;
		int len = 0;
		int ret = 0;
		char buf[1024*1024];
		len = snprintf(buf, sizeof(buf), "%s", "已为您尽量安排必去地点，考虑行程安排合理性，以下行程未安排，建议在“编辑行程”中手动添加：<br/>");
		for (int j = 0; j < delMustPlaceList.size(); ++j) {
			std::string name = delMustPlaceList[j]->_name;
			if (name == "") name = delMustPlaceList[j]->_enname;
			ret = snprintf(buf + len, sizeof(buf) - len, "%d.%s<br/>", j + 1 , name.c_str());
			if (ret < 0) {
				MJ::PrintInfo::PrintErr("PostProcess::MakeJView, add jWarning, buf is not enough");
				break;
			}
			len += ret;
		}
		jWarning["desc"] = buf;
		jWarningList.append(jWarning);
	}
	return 0;
}


int PostProcessor::MakeOutputS126(Json::Value& req, BasePlan* plan, Json::Value& resp) {
    Json::FastWriter jsw;
    MJ::PrintInfo::PrintLog("PostProcessor::MakeOutputS126, req is %s", jsw.write(req).c_str());
    Json::Value jReqList = req["list"];
    Json::Value& jRespList = resp["data"]["list"];
	int error_id = 5000;
	std::string error_str = "抱歉，服务器没有响应，请稍后再试。";
	std::string error_reason = "s126规划失败";
    for (int i = 0; i < jReqList.size() && i < jRespList.size() && jReqList[i].isObject(); i++) {
		if(jRespList[i]["view"].isObject()
				and jRespList[i]["view"]["summary"].isObject()
				and jRespList[i]["view"]["summary"]["days"].isArray()
				)
		{
			Json::Value & days= jRespList[i]["view"]["summary"]["days"];
			int expire=0;
			for(int j=0; j<days.size(); j++)
			{
				if(days[j]["pois"].isNull()) days[j]["pois"]=Json::arrayValue;
				if(days[j]["expire"].isInt()) expire+=days[j]["expire"].asInt();
			}
			if(expire) jRespList[i]["view"]["expire"] =1;
			error_id = 0;
			error_str = "";
			error_reason = "";
		}
		if(jRespList[i]["view"].isObject()
				and jRespList[i]["view"]["day"].isArray()
				and jRespList[i]["view"]["day"].size()>0
		  )
		{
			//s126交通报错
			Json::Value& jWarningList = jRespList[i]["view"]["summary"]["warning"];
			Json::Value & day= jRespList[i]["view"]["day"];
			for(int k=0; k<day.size();k++)
			{
				if(day[k]["view"].isNull()) day[k]["view"]=Json::arrayValue;
			}
			AddTrafficWarning(plan, day, jWarningList);
		}

    }
	resp["error"]["error_id"] = error_id;
	resp["error"]["error_str"] = error_str;
	resp["error"]["error_reason"] = error_reason;
    MJ::PrintInfo::PrintLog("PostProcessor::MakeOutputS126, Resp is %s", jsw.write(resp).c_str());
    return 0;
}

int PostProcessor::MakeOutputS130(Json::Value& req, BasePlan* basePlan, Json::Value& resp) {
	Json::Value jData;
	jData["stat"] = 1;
	jData["city"] = resp["data"]["city"];
	AddDiffAfterOption(req, basePlan, jData);
	resp["data"] = jData;
	return 0;
}

int PostProcessor::AddDiffAfterOption(Json::Value& req, BasePlan* basePlan, Json::Value& jData) {
	//优化对比
	//展示文案
	//jData-diff
	//用户偏好酒店时间
	std::string arriveHotel = req["cityPreferCommon"]["timeRange"]["to"].asString();
	std::string deptHotel = req["cityPreferCommon"]["timeRange"]["from"].asString();
	std::string warning = "";
	if (basePlan->m_OptionMode == 4) { //只选择了突破时间范围
		warning = "已根据每日行程来灵活安排出发和返回休息的时间";
	} else if (basePlan->m_OptionMode.test(1)) {  //选择了优化顺序
		warning = "已为您尽量在景点开放时间内找到最顺的路线";
	} else { //没有选择优化顺序
		warning = "已为您推荐合理的游玩时长";
	}
	jData["diff"]["title"] = warning;

	Json::Value jReqView, jRespView, reqPoisList, respPoisList;
	//从view结构中获取所有需要对比的信息
    MJ::PrintInfo::PrintLog("PostProcessor::AddDiffAfterOption, Resp Plan...");
	GetAllItemFromView(jData["city"]["view"], jRespView, respPoisList);

	std::string date = req["city"]["arv_time"].asString().substr(0,8);
	//不过夜
	if (req["city"]["arv_time"].asString().substr(0,8) == req["city"]["dept_time"].asString().substr(0,8) && jData["city"]["arv_time"] == req["city"]["arv_time"] && jData["city"]["dept_time"] == req["city"]["dept_time"]) {
		warning += "，并完全满足" + req["city"]["arv_time"].asString().substr(9) + "到达、"+ req["city"]["dept_time"].asString().substr(9) + "离开城市的要求";
	} else if (jRespView["hotelStime"] <= arriveHotel && jRespView["hotelEtime"] >= deptHotel) {
		warning += "，并完全满足每日" + deptHotel + "出发、" + arriveHotel + "返回的要求";
	}

	// 原方案不可用
	if (basePlan->s130reqAvail == 0) {
		jData["diff"]["desc"] = warning;
	}
	// 原方案可用
	else if (basePlan->s130reqAvail == 1) {
		MJ::PrintInfo::PrintLog("PostProcessor::AddDiffAfterOption, Req Plan...");
		GetAllItemFromView(req["city"]["view"], jReqView, reqPoisList);
		std::bitset<5> result;
		//0. 休息时间增加
		//1. 游玩时长增加
		//2. 交通距离减少
		//3. 交通时间减少
		//4. 不开门景点数减少
		if (jReqView["restTime"].asInt() <= 3600*10 && jRespView["restTime"].asInt() - jReqView["restTime"].asInt() > 30 * 60) {
			result.set(0);
		}
		if (jReqView["playTime"].asInt() > 0 && (jRespView["playTime"].asInt() > jReqView["playTime"].asInt() * 1.05)) {
			result.set(1);
		}
		if (jReqView["trafDist"].asInt() > 0 && (jReqView["trafDist"].asInt() - jRespView["trafDist"].asInt() >= 500)) {
			result.set(2);
		}
		if (jReqView["trafTime"].asInt() > 0 && (jReqView["trafTime"].asInt() - jRespView["trafTime"].asInt() >= 10 * 3600)) {
			result.set(3);
		}
		if (jReqView["closeViewNum"].asInt() - jRespView["closeViewNum"].asInt() > 0) {
			result.set(4);
		}

		bool satisfyPrefer = false;
		//原行程不满足每日的出发返回时间，现行程可以满足，并且用户没有选择优化出发返回
		if ((jReqView["hotelStime"] > arriveHotel || jReqView["hotelEtime"] < deptHotel) && (jRespView["hotelStime"] <= arriveHotel && jRespView["hotelEtime"] >= deptHotel) && !(basePlan->m_OptionMode.test(0))) {
			satisfyPrefer = true;
		}

		//如果s130req可以生成新方案,仍未命中,并且新方案不满足偏好
		if (result.none() && !satisfyPrefer) {
			//未命中任意一条,jData置null,stat = 0
			jData = Json::Value();
			jData["stat"] = 0;
			return 0;
		}
		if (result.any())
		{
			warning += "。同时，与旧方案相比";
			if (result[0]) {
				int restHour = (jRespView["restTime"].asInt() - jReqView["restTime"].asInt())/3600;
				int restMin = (jRespView["restTime"].asInt() - jReqView["restTime"].asInt())%3600/60;
				if(restHour) {
					warning += "，平均每日休息时长增加" + std::to_string(restHour) + "h" + std::to_string(restMin) + "min";
				} else {
					warning += "，平均每日休息时长增加" + std::to_string(restMin) + "min";
				}
			}
			if (result[1]) {
				warning += "，整体有效游玩时长增加" + std::to_string(100*(jRespView["playTime"].asInt() - jReqView["playTime"].asInt())/jReqView["playTime"].asInt()) + "%";
			}
			if (result[2]) {
				float trafDes = (jReqView["trafDist"].asInt() - jRespView["trafDist"].asInt())/1000.0;
				std::stringstream s;
				s <<std::setiosflags(std::ios::fixed)<<std::setprecision(1)<< trafDes;
				std::string str;
				s >> str;
				warning += "，整体交通距离节省" + str + "公里";
			}
			if (result[3]) {
				int trafHour = (jReqView["trafTime"].asInt() - jRespView["trafTime"].asInt())/3600;
				int trafMin = (jReqView["trafTime"].asInt() - jRespView["trafTime"].asInt())%3600/60;
				if(trafHour) {
					warning += "，整体交通时间节省" + std::to_string(trafHour) + "h" + std::to_string(trafMin) + "min";
				} else {
					warning += "，整体交通时间节省" + std::to_string(trafMin) + "min";
				}
			}
			if (result[4]) {
				warning += "，避开未开放景点" + std::to_string(jReqView["closeViewNum"].asInt() - jRespView["closeViewNum"].asInt()) + "个";
			}
			warning += "。";
		}
		jData["diff"]["desc"] = warning;
	}

	//不足
	//jData[warning]
	Json::Value jWarning = Json::Value();
	Json::Value jWarningList = Json::arrayValue;
	//突破偏好
	//s130智能优化选项 2.游玩时间范围|1.顺序|0.时长
	//OptionMode 为0 表示 不可以突破
	//selectMode 为0 表示 未保留
	std::bitset<3> optionMode;
	//突破hotel
	if (!basePlan->m_OptionMode.test(2) and !basePlan->m_SelectedOptMode.test(2)) {
		optionMode.set(2);
	}
	if (basePlan->s130reqAvail == 1) {
		//突破顺序
		if (!basePlan->m_OptionMode.test(1) and !basePlan->m_SelectedOptMode.test(1)) {
			optionMode.set(1);
		}
		//突破时长
		if (!basePlan->m_OptionMode.test(0) and !basePlan->m_SelectedOptMode.test(0)) {
			optionMode.set(0);
		}
	}
	warning = "";
	//数值: 4.出发返回 2.顺序 1.时长
	if (optionMode == 1) {
		warning = "未能保留原方案游玩时长";
	} else if (optionMode == 2) {
		warning = "未能保留原方案游玩顺序";
	} else if (optionMode == 3) {
		warning = "未能保留原方案游玩顺序和游玩时长";
	} else if (optionMode == 4) {
		warning = "未能保留每日"+ deptHotel + "出发、" + arriveHotel + "返回的要求";
	} else if (optionMode == 5) {
		warning = "未能保留原方案游玩时长和每日"+ deptHotel + "出发、" + arriveHotel + "返回的要求";
	} else if (optionMode == 6) {
		warning = "未能保留原方案游玩顺序和每日"+ deptHotel + "出发、" + arriveHotel + "返回的要求";
	} else if (optionMode == 7) {
		warning = "未能保留原方案游玩顺序、游玩时长和每日"+ deptHotel + "出发、" + arriveHotel + "返回的要求";
	}
	if (warning != "") {
		jWarning["title"] = warning;
		jWarningList.append(jWarning);
	}
	warning = "";
	//删点
	if (jData["city"]["view"]["summary"]["missPoi"].size() > 0) {
		warning = "由于时间安排过于紧张，需要删除以下地点";
		jWarning["title"] = warning;
		warning = "";
		for (int i = 0; i < jData["city"]["view"]["summary"]["missPoi"].size(); i++) {
			warning += jData["city"]["view"]["summary"]["missPoi"][i]["name"].asString();
			if (i != jData["city"]["view"]["summary"]["missPoi"].size() - 1) warning+= "，";
		}
		jWarning["desc"] = warning;
		jWarningList.append(jWarning);
	}
	//开关门不符
	warning = "";
	if (respPoisList["closePois"].size() > 0) {
		warning = "无法匹配以下景点的开关门时间";
		jWarning["title"] = warning;
		warning = "";
		for (int i = 0; i < respPoisList["closePois"].size(); i++) {
			warning += respPoisList["closePois"][i]["name"].asString();
			if (i != respPoisList["closePois"].size() - 1) warning += "，";
		}
		jWarning["desc"] = warning;
		jWarningList.append(jWarning);
	}
	jData["warning"] = jWarningList;
	return 0;
}
int PostProcessor::InsertToPoisSet(std::string id, std::tr1::unordered_map<std::string, int>& placeNumMap, std::tr1::unordered_set<std::string>& poiIdSet) {
	if (placeNumMap.find(id) == placeNumMap.end()) {
		placeNumMap[id] = 0;
	} else {
		placeNumMap[id] ++;
	}
	char buff[1000];
	snprintf(buff, sizeof(buff), "%s#%02d", id.c_str(), placeNumMap[id]);
	std::string newId = std::string(buff);
	poiIdSet.insert(newId);
	return 0;
}
int PostProcessor::GetAllItemFromView(Json::Value& jView, Json::Value& jReqView, Json::Value& poisList) {
	//休息时间
	int restTime = 0;
	//游玩时间
	int playTime = 0;
	//交通距离
	int trafDist = 0;
	//交通时间
	int trafTime = 0;
	//总景点数
	int viewNum = 0;
	//不开门景点
	int closeViewNum = 0;
	poisList["closePois"] = Json::arrayValue;
	//poisList["allPois"] = Json::arrayValue;
	Json::Value& jDayList = jView["day"];
	std::string hotelStime = "";
	std::string hotelEtime = "99:99";
	for (int i = 0; i < jDayList.size(); i ++) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); j ++) {
			Json::Value& jView = jViewList[j];
			//hotel休息时间
			if (jView["type"].asInt() & LY_PLACE_TYPE_HOTEL) {
				if (j == 0) continue;  //在前一晚已经加过
				std::string stime = jView["stime"].asString();
				std::string etime = jView["etime"].asString();
				int sTimeInt = MJ::MyTime::toTime(stime);
				int eTimeInt = MJ::MyTime::toTime(etime);
				restTime += eTimeInt - sTimeInt;
				if (jView["stime"].asString().substr(9) > hotelStime) hotelStime = jView["stime"].asString().substr(9);
				if (jView["etime"].asString().substr(9) < hotelEtime) hotelEtime = jView["etime"].asString().substr(9);
				if(jView["stime"].asString().substr(0,8) == jView["etime"].asString().substr(0,8)) hotelStime = "24:00";
			}
			if (jView.isMember("traffic") && !jView["traffic"].isNull()) {
				trafDist += jView["traffic"]["dist"].asInt();
				trafTime += jView["traffic"]["dur"].asInt();
			}
			//景点游玩时间
			if (jView["type"].asInt() & LY_PLACE_TYPE_VAR_PLACE) {
				Json::Value jPoi;
				jPoi["id"] = jView["id"];
				jPoi["name"] = jView["name"];
				//poisList["allPois"].append(jPoi);

				playTime += jView["dur"].asInt();
				viewNum ++;
				if (jView["close"].asInt() == 1) {
					poisList["closePois"].append(jPoi);
					closeViewNum ++;
				}
			}
		}
	}
	if (jDayList.size()) jReqView["restTime"] = restTime / jDayList.size();
	else jReqView["restTime"] = 1;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, restTime:%d", restTime);
	jReqView["playTime"] = playTime;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, playTime:%d", playTime);
	jReqView["trafDist"] = trafDist;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, trafDist:%d", trafDist);
	jReqView["trafTime"] = trafTime;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, trafTime:%d", trafTime);
	jReqView["viewNum"] = viewNum;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, viewNum:%d", viewNum);
	jReqView["closeViewNum"] = closeViewNum;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, closeViewNum:%d", closeViewNum);
	jReqView["hotelStime"] = hotelStime;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, hotelStime:%s", hotelStime.c_str());
	jReqView["hotelEtime"] = hotelEtime;
    MJ::PrintInfo::PrintLog("PostProcessor::GetAllItemFromView, hotelEtime:%s", hotelEtime.c_str());
	return 0;
}

int PostProcessor::AddErrors(BasePlan* basePlan, Json::Value& jData) {
	Json::Value jErrorList = Json::arrayValue;
	for (BasePlan::TryErrMapIt it = basePlan->m_tryErrorMap.begin(); it != basePlan->m_tryErrorMap.end(); it = basePlan->m_tryErrorMap.upper_bound(it->first)) {
		Json::Value jError;
		jError["didx"] = it->first;
		Json::Value jTips;
		jTips.resize(0);
		std::pair<BasePlan::TryErrMapIt, BasePlan::TryErrMapIt> res = basePlan->m_tryErrorMap.equal_range(it->first);
		//去重
		std::set<std::pair<int, std::string> > dayErrSet;
		for (BasePlan::TryErrMapIt errMapIt = res.first; errMapIt != res.second; ++errMapIt) {
			dayErrSet.insert(errMapIt->second);
		}
		for (std::set<std::pair<int, std::string> >::iterator errIt = dayErrSet.begin(); errIt != dayErrSet.end(); ++errIt) {
			Json::Value jTip;
			jTip["type"] = errIt->first;
			jTip["content"] = errIt->second;
			std::tr1::unordered_set<int> checkInDidxSet;
			for (auto hIt = basePlan->m_failedHotelDidx.begin(); hIt != basePlan->m_failedHotelDidx.end(); hIt++) {
				const HInfo* hInfo = basePlan->GetHotelByDidx(*hIt);
				int checkInDidx = *hIt;
				if (hInfo) checkInDidx = hInfo->m_dayStart;
				if (checkInDidxSet.find(checkInDidx) == checkInDidxSet.end()) checkInDidxSet.insert(checkInDidx);
			}
			for (auto cIt=checkInDidxSet.begin(); cIt!=checkInDidxSet.end(); cIt++) {
				jTip["checkInDidx"].append(*cIt);
			}
			jTips.append(jTip);
		}
		jError["tips"] = jTips;
		jErrorList.append(jError);
	}
	jData["error"] = jErrorList;
}
int PostProcessor::AddTrafficWarning(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList) {
	time_t lastEnd = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 1; j < jViewList.size(); ++j) {
			Json::Value jLastView = Json::Value();
			jLastView = jViewList[j-1];
			int fixed = 0;
			if (jLastView.isMember("traffic") && jLastView["traffic"].isMember("fixed") && jLastView["traffic"]["fixed"].isInt()) {
				fixed = jLastView["traffic"]["fixed"].asInt();
			}
			//团游中所有点都被当做锁定时间点 其他情况下仅!!!锁定点!!!之间交通报错
			if (fixed) continue;
			if (!IsLockTimePoi(basePlan, jViewList[j],j)) continue;
			if (!IsLockTimePoi(basePlan, jViewList[j-1],j-1)) continue;

			lastEnd = MJ::MyTime::toTime(jLastView["etime"].asString().c_str(), basePlan->m_TimeZone);
			const LYPlace* lastPlace = basePlan->GetLYPlace(jLastView["id"].asString());
			if (lastPlace == NULL) {
				std::cerr << jLastView["id"].asString() << " is a null place!" << std::endl;
			}
			//从酒店出发交通时间不报错
			//if (lastPlace && lastPlace->_t & LY_PLACE_TYPE_HOTEL) continue;
			int lastTrafDur = 0;
			if (jLastView.isMember("traffic") && jLastView["traffic"].isMember("dur") && jLastView["traffic"]["dur"].asInt() >= 0) {
				lastTrafDur = jLastView["traffic"]["dur"].asInt();
			}
			time_t stime = MJ::MyTime::toTime(jViewList[j]["stime"].asString().c_str(), basePlan->m_TimeZone);
			//酒店报错使用
			time_t etime = MJ::MyTime::toTime(jViewList[j]["etime"].asString().c_str(), basePlan->m_TimeZone);
			const LYPlace* place = basePlan->GetLYPlace(jViewList[j]["id"].asString());
			if (place == NULL) {
				std::cerr << jViewList[j]["id"].asString() << " is a null place!" << std::endl;
			}
			Json::Value jWarning;
			jWarning["didx"] = i;
			jWarning["vidx"] = j - 1;
			jWarning["type"] = 11;
			char buf[128] = {0};
			if (lastEnd >= stime) {
				snprintf(buf, sizeof(buf), "前后安排过紧，未预留交通时间");
				jWarning["desc"] = buf;
				jWarningList.append(jWarning);
			} else if (lastEnd + lastTrafDur > stime) {
				if (lastEnd + lastTrafDur - stime >= 60) {
					snprintf(buf, sizeof(buf), "只预留了");
					if ((stime - lastEnd + 59) / 3600) {
						snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d小时", (stime - lastEnd + 59)/3600);
					}
					if (((stime - lastEnd + 59) / 60) % 60) {
						snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d分钟", ((stime - lastEnd + 59)/60)%60);
					}
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "的交通时间，实际交通已经超出");
					//snprintf(buf, sizeof(buf), "交通时间超出可用时间%d min", (lastEnd + lastTrafDur - stime + 59) / 60);
					if ((lastEnd + lastTrafDur - stime + 59) / 3600) {
						snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d小时", (lastEnd + lastTrafDur - stime + 59) / 3600);
					}
					if ((lastEnd + lastTrafDur - stime + 59) / 60 % 60) {
						snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d分钟", (lastEnd + lastTrafDur - stime + 59) / 60 % 60);
					}
					jWarning["desc"] = buf;
					jWarningList.append(jWarning);
				} else if (lastEnd + lastTrafDur - stime > 0) { //time_t 与string型转换所致误差
					snprintf(buf, sizeof(buf), "交通时间超出可用时间%d sec", lastEnd + lastTrafDur - stime);
					MJ::PrintInfo::PrintDbg("[%s]didx %d, vidx %d, %s", basePlan->m_qParam.log.c_str(), i, j - 1, buf);
				}
			}
		}
	}
}

int PostProcessor::MakeOutputProduct(Json::Value& req, Json::Value& resp) {
	_INFO("PostProcessor::MakeOutputProduct...");
	if (req.isMember("product")) {
		resp["data"]["product"] = req["product"];
	}
	Json::Value& jWanle = resp["data"]["product"]["wanle"];
	//jWanle = Json::Value();
	//s126.127
	if (resp.isMember("data") && resp["data"].isObject() && resp["data"].isMember("list") && resp["data"]["list"].isArray()) {
		jWanle = Json::Value();
		for(int i = 0; i < resp["data"]["list"].size(); i ++) {
			if (req["list"][i].isMember("ridx") && req["list"][i]["ridx"].isInt()) {
				Json::Value& jCity = resp["data"]["list"][i];
				if (jCity.isMember("view") && jCity["view"].isObject() && !jCity["view"].isNull()) {
					MakeProduct(req, jCity["view"], jWanle, req["list"][i]["ridx"].asInt(), 1);
				}
				if (jCity.isMember("traffic_pass") && jCity["traffic_pass"].isObject() && !jCity["traffic_pass"].isNull()) {
					MakeProduct(req, jCity["traffic_pass"], jWanle, req["list"][i]["ridx"].asInt(), 0);
				}
			}
		}
	//s128.s130
	} else if (resp.isMember("data") && resp["data"].isObject() && resp["data"].isMember("city") && resp["data"]["city"].isObject() && !resp["data"]["city"].isNull()) {
		int ridx = 0;
		if (req.isMember("ridx") && req["ridx"].isInt()) ridx = req["ridx"].asInt();

		Json::Value& jCity = resp["data"]["city"];
		if (jCity.isMember("view") && jCity["view"].isObject() && !jCity["view"].isNull()) {
			DelOriProduct(jWanle, ridx, 1);
			MakeProduct(req, jCity["view"], jWanle, ridx, 1);
		}
		if (jCity.isMember("traffic_pass") && jCity["traffic_pass"].isObject() && !jCity["traffic_pass"].isNull()) {
			DelOriProduct(jWanle, ridx, 0);
			MakeProduct(req, jCity["traffic_pass"],jWanle, ridx, 0);
		}
	//s125
	} else if (resp.isMember("data") && resp["data"].isObject() && resp["data"].isMember("view") && resp["data"]["view"].isObject() && !resp["data"]["view"].isNull()) {
		int ridx = 0;
		if (req.isMember("ridx") && req["ridx"].isInt()) ridx = req["ridx"].asInt();
		DelOriProduct(jWanle, ridx, 1);
		MakeProduct(req, resp["data"]["view"], jWanle, ridx, 1);
	}
	if (jWanle.size() == 0) {
		jWanle = Json::Value();
	}
	return 0;
}
int PostProcessor::MakeProduct(Json::Value& req, Json::Value& jRouteView, Json::Value& jWanle, int ridx, int inCity) {
	_INFO("PostProcessor::MakeProduct...");
	Json::Value product = Json::Value();
	Json::Value& jDayList = jRouteView["day"];
	for(int i = 0; i < jDayList.size(); i++) {
		Json::Value& jViewList = jDayList[i]["view"];
		for (int j = 0; j < jViewList.size(); j ++) {
			Json::Value& jView = jViewList[j];
			if (jView.isMember("product") && jView["product"].isObject() && jView["product"].isMember("pid")) {
				product = jView["product"];
				std::string productId = product["product_id"].asString();
				jView["product"] = Json::Value();
				jView["product"]["product_id"] = productId;
				product["pos"]["ridx"] = ridx;
				product["pos"]["inCity"] = inCity;
				jWanle[productId] = product;
				_INFO("PostProcessor::MakeProduct, from view.product insert %s", productId.c_str());
			} else if(jView.isMember("product") && jView["product"].isObject() && jView["product"].isMember("product_id") && jView["product"]["product_id"].isString()){
				std::string productId = jView["product"]["product_id"].asString();
				if(req.isMember("product") && req["product"].isObject() && req["product"].isMember("wanle") && req["product"]["wanle"].isObject() && req["product"]["wanle"].isMember(productId) && req["product"]["wanle"][productId].isObject()) {
					jWanle[productId] = req["product"]["wanle"][productId];
					_INFO("PostProcessor::MakeProduct, from req.product insert %s", productId.c_str());
				}
			}
		}
	}
	Json::Value& jDays = jRouteView["summary"]["days"];
	for(int i = 0; i < jDays.size(); i++) {
		Json::Value& jPois = jDays[i]["pois"];
		for(int j = 0; j < jPois.size(); j ++) {
			Json::Value& jPoi = jPois[j];
			if (jPoi.isMember("product") && jPoi["product"].isObject() && jPoi["product"].isMember("tickets") && jPoi["product"]["tickets"].isArray()) {
				std::string productId = jPoi["product"]["product_id"].asString();
				jPoi["product"] = Json::Value();
				jPoi["product"]["product_id"] = productId;
				std::string stime = "";
				if (jWanle[productId].isMember("stime") && jWanle[productId]["stime"].isString()) stime = jWanle[productId]["stime"].asString();
				if (stime != "") jPoi["stime"] = stime;
			}
		}
	}

	return 0;
}
int PostProcessor::DelOriProduct(Json::Value& jWanle, int ridx, int inCity) {
	if (!jWanle.isObject()) jWanle= Json::Value();
	Json::Value& jProdList = jWanle;
	Json::Value tmpProd = jProdList;
	Json::Value::Members keys = jProdList.getMemberNames();
	for (auto iter = keys.begin(); iter != keys.end(); iter++) {
		Json::Value& jProduct = jProdList[*iter];
		if (jProduct.isMember("pos") && jProduct["pos"].isObject()
				&& jProduct["pos"].isMember("ridx") && jProduct["pos"]["ridx"].isInt()
				&& jProduct["pos"].isMember("inCity") && jProduct["pos"]["inCity"].isInt()) {
			if (jProduct["pos"]["ridx"].asInt() == ridx && jProduct["pos"]["inCity"].asInt() == inCity) {
				tmpProd.removeMember(*iter);
				_INFO("PostProcessor::DelOriProduct...delete %s", (*iter).c_str());
			}
		}
	}
	jProdList = tmpProd;
	return 0;
}
bool PostProcessor::IsLockTimePoi(BasePlan* basePlan, const Json::Value& jView,int index) {
	//到达离开点 租车点
	if (jView.isMember("type") && (jView["type"].asInt() & (LY_PLACE_TYPE_ARRIVE | LY_PLACE_TYPE_CAR_STORE))) return true;
	//每天的出发酒店
	if (index == 0 && jView.isMember("type") && jView["type"].isInt() && (jView["type"].asInt() & LY_PLACE_TYPE_HOTEL)) return true;
	//有场次玩乐
	if (jView.isMember("product") && jView["product"].isMember("pid") && jView["product"]["pid"].isString()) {
		std::string id = jView["product"]["pid"].asString();
		const LYPlace* place = basePlan->GetLYPlace(id);
		if (place) {
			const Tour* tour = dynamic_cast<const Tour*>(place);
			if (tour && tour->m_srcTimes.size() > 0 && tour->m_srcTimes[tour->m_srcTimes.size()-1].isMember("t") && tour->m_srcTimes[tour->m_srcTimes.size()-1]["t"].isString() && tour->m_srcTimes[tour->m_srcTimes.size()-1]["t"].asString() != "") {
				return true;
			}
		}
	}

	return false;
}
