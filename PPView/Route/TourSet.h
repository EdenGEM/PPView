#ifndef __TOURSET_H__
#define __TOURSET_H__

#include <iostream>
#include "base/define.h"
#include "base/ReqParser.h"
#include "Planner.h"

class TourViewItem {
	public:
		TourViewItem (BasePlan* basePlan, const LYPlace* place, int placeType, std::string date) {
			m_id = place->_ID;
			m_type = place->_t;
			m_custom = place->m_custom;
			m_name = place->_name;
			if (m_name == "") m_name = place->_enname;
			m_lname = place->_enname;
			m_coord = place->_poi;
			if (placeType == NODE_FUNC_KEY_ARRIVE) {
				m_stime = MJ::MyTime::toString(basePlan->m_ArriveTime, basePlan->m_TimeZone);
				m_dur = basePlan->GetEntryTime(place);
				m_etime = MJ::MyTime::toString(basePlan->m_ArriveTime + m_dur, basePlan->m_TimeZone);
			} else if (placeType == NODE_FUNC_KEY_DEPART) {
				m_etime = MJ::MyTime::toString(basePlan->m_DepartTime, basePlan->m_TimeZone);
				m_dur = basePlan->GetExitTime(place);
				m_stime = MJ::MyTime::toString(basePlan->m_DepartTime - m_dur, basePlan->m_TimeZone);
			} else if (placeType == NODE_FUNC_KEY_HOTEL) {
				//酒店先初始化为19:00入住,9:00出门
				m_stime = date + "_19:00";
				m_dur = -1;
				m_etime = MJ::MyTime::datePlusOf(date,1) + "_09:00";
			}
			//m_doWhat 通过adddowhat 修改
			m_doWhat = "";
			m_funcType = placeType;
		}
		TourViewItem (const Json::Value& tour, BasePlan* basePlan, const Json::Value& jView, std::string date, std::string srcDate, const bool& includeProduct) {
			const LYPlace* place = basePlan->GetLYPlace(jView["id"].asString());
			//可能公转私以后，屏蔽公有id，这时根据公有id拿到私有点数据
			if (place == NULL) place = LYConstData::GetLYPlace(jView["id"].asString(),basePlan->m_qParam.ptid);
			//如果仍没有得到，设置为自定义点
			if (place == NULL) place = basePlan->SetCustomPlace(jView["type"].asInt(), jView["id"].asString(), jView["name"].asString(), jView["lname"].asString(), jView["coord"].asString(), POI_CUSTOM_MODE_CUSTOM);
			if (place) {
				m_id = place->_ID;
				m_custom = place->m_custom;
				m_name = place->_name;
				if (place->_name == "") {
					m_name = place->_enname;
				}
				m_lname = place->_enname;
				m_coord = place->_poi;
			}
			//type不会变
			m_type = jView["type"].asInt();
			m_doWhat = jView["doWhat"].asString();
			m_dur = jView["dur"].asInt();
			std::string startDate = date;
			int dayTransLate = MJ::MyTime::compareDayStr(srcDate, jView["stime"].asString().substr(0,8));
			startDate = MJ::MyTime::datePlusOf(date, dayTransLate);
			m_stime = startDate + jView["stime"].asString().substr(8);
			std::string endDate = date;
			dayTransLate = MJ::MyTime::compareDayStr(srcDate,jView["etime"].asString().substr(0,8));
			endDate = MJ::MyTime::datePlusOf(date, dayTransLate);
			m_etime = endDate + jView["etime"].asString().substr(8);
			if (jView.isMember("info")) {
				m_info = jView["info"];
			}
			if (jView.isMember("idle")) {
				m_idle = jView["idle"];
			}
			if (jView.isMember("close") && jView["close"].isInt()) {
				m_close = jView["close"].asInt();
			} else {
				m_close = -1;
			}
			if (jView.isMember("diningNearby") && jView["diningNearby"].isInt()) {
				m_diningNearby = jView["diningNearby"].asInt();
			} else {
				m_diningNearby = -1;
			}
			if (jView.isMember("product") && jView["product"].isObject()) {
				if (jView["product"].isMember("productId") && jView["product"]["productId"].isString()) {
					std::string productId = jView["product"]["productId"].asString();
				   	if (tour["product"]["wanle"].isMember(productId)) {
						m_product = tour["product"]["wanle"][productId];
						m_product["fixed"] = 1;
						if (includeProduct) {
							m_product["noQuotn"] = 1;
						} else {
							m_product["noQuotn"] = 0;
						}
						m_product["date"] = date;
						std::string stime = jView["stime"].asString().substr(9);
						m_product["productId"] = m_product["pid"].asString()+"#"+date+"#"+stime;
					} else {
						m_product = jView["product"];
					}
				}
			}
			if (jView.isMember("traffic") && jView["traffic"].isObject()) {
				m_traffic = jView["traffic"];
			}
			m_funcType = NODE_FUNC_NULL;
		}
	public:
		std::string m_id;
		int m_custom;
		int m_type;
		std::string m_name;
		std::string m_lname;
		std::string m_coord;
		std::string m_doWhat;
		Json::Value m_info;
		Json::Value m_idle;
		int m_close;
		int m_diningNearby;
		int m_dur;
		std::string m_stime;
		std::string m_etime;
		Json::Value m_product;
		Json::Value m_traffic;
		int m_funcType;
};

class TourCityInfo {
	private:
		//只解析团游相关信息
		Json::Value m_srcTourSet;  //原始团游产品 product中解析出来
		Json::Value m_srcRoute;  //团游中该城市的原始信息
		bool isFirstCity;  //团游首城市
		bool isLastCity;  //团游尾城市
		std::vector<TourViewItem> m_ViewList;
		bool m_includeWanle;
		//std::tr1::unordered_map<std::string, TrafficItem*> m_trafMap;
		//所需的交通数据，包括自定义交通通过basePlan->GetTraffic获取
	public:
		//修改了的酒店索引
		std::tr1::unordered_set<int> changedHotelIdx;
		TourCityInfo() {
			isFirstCity = false;
			isLastCity = false;
			m_includeWanle = false;
			m_srcTourSet = Json::Value();
			m_srcRoute = Json::arrayValue;
		}
		//城市相关信息 到达离开点/时间在basePlan中解析
		std::vector<HInfo*> m_hotelInfoList;  //酒店信息
		const LYPlace* m_arvPoi;
		const LYPlace* m_deptPoi;
		std::string m_arvDate;
		std::string m_deptDate;
		std::string m_checkIn;
		std::string m_checkOut;
		int m_arvTime;
		int m_deptTime;

		const LYPlace* GetHotelbyIndex(int didx);
		int GetCityInfoFromBasePlan(BasePlan* basePlan);
		int GetReqInfo(const QueryParam& param, Json::Value& req, BasePlan* basePlan);
		int GetTraffic(BasePlan* basePlan);
		int ExpandView(BasePlan* basePlan);
		int ChangeHotel (BasePlan* basePlan);
		int InitTourItem(Json::Value& req, BasePlan* basePlan);
		int MakejView(BasePlan* basePlan, const TourViewItem& tourViewItem, Json::Value& jView);
		int MakejDayList (BasePlan* basePlan, Json::Value& jDayList);
		int MakejSummary (BasePlan* basePlan, Json::Value& req, Json::Value& jSummary, Json::Value& jDayList);
		int MakeOutView(BasePlan* basePlan, Json::Value& req, Json::Value& resp);
		int MakeTrafficDetail (BasePlan* basePlan, const TrafficItem* trafItem, Json::Value& jTrafItem);
		int AttachArvDeptPoi(BasePlan* basePlan);
	public:
		int DoPlan(const QueryParam& param, BasePlan* basePlan, Json::Value& req, Json::Value& resp);
};

#endif
