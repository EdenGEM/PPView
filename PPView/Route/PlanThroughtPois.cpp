#include "PlanThroughtPois.h"
#include "PostProcessor.h"
#include "base/LYConstData.h"

int PlanThroughtPois::PlanPath(const QueryParam& param, Json::Value& req, Json::Value& resp, ErrorInfo& error) {
	MJ::PrintInfo::PrintDbg("[%s]PlanThrought::PlanPath,S128", param.log.c_str());
	int ret = 0;
	ret = ReqChecker::DoCheck(param, req, error);
	if (ret) {
		return ret;
	}
	ret = LoadReq(req, param);
	if (ret) {
		return ret;
	}
	ret = MakeOutputS128(req, param, resp);
	if (ret) {
		return ret;
	}
	return 0;
}

int PlanThroughtPois::LoadReq(const Json::Value& req, const QueryParam& param) {
	int ret = 0;
	ret = LoadCityInfo(param, req);
	ret = LoadReqPois(param);
	return ret;
}
int PlanThroughtPois::LoadCityInfo(const QueryParam& param,const Json::Value& req) {
	MJ::PrintInfo::PrintLog("PlanThroughtPois::LoadCityInfo !");
	m_cityInfo = req["city"];
	const Json::Value& jDays = req["city"]["traffic_pass"]["summary"]["days"];
	for (int i = 0; i < jDays.size(); i ++) {
		Json::Value jFilterPois;
		jFilterPois.resize(0);
		Json::Value jPois = jDays[i]["pois"];
		for (int j = 0; j < jPois.size(); j ++) {
			std::string id = jPois[j]["id"].asString();
			const LYPlace *place = LYConstData::GetLYPlace(id, param.ptid);
			if (place == NULL and jPois[j]["custom"].asInt()!=1) {
				MJ::PrintInfo::PrintLog("PlanDayInfo::LoadCityInfo, %s not found!", id.c_str());
				continue;
			}
			jFilterPois.append(jPois[j]);
		}
		m_cityInfo["traffic_pass"]["summary"]["days"][i]["pois"] = jFilterPois;
	}
	m_trafficOfCity = req["city"]["traffic"];
	if (m_cityInfo["traffic_pass"]["summary"].isMember("customTraffic") && m_cityInfo["traffic_pass"]["summary"]["customTraffic"].isObject()) {
		m_customTraffic = m_cityInfo["traffic_pass"]["summary"]["customTraffic"];
	}
	if(req.isMember("product") and req["product"].isObject()) m_product = req["product"];
	return 0;
}
int PlanThroughtPois::LoadReqPois(const QueryParam& param) {
	MJ::PrintInfo::PrintDbg("[%s]PlanThrought::GetReqPois", param.log.c_str());
	Json::Value& jDays = m_cityInfo["traffic_pass"]["summary"]["days"];

	for (int i = 0; i < jDays.size(); i ++) {
		std::string date = jDays[i]["date"].asString();
		Json::Value& jPois = jDays[i]["pois"];
		PlanDayInfo planDayInfo;
		planDayInfo.LoadPoisData(jPois, param, date, m_product);
		planDayInfo.LoadPoisTraffic(m_customTraffic);
		m_planDays.insert(make_pair(date, planDayInfo));
	}
	return 0;
}

int PlanDayInfo::LoadPoisTraffic(Json::Value& jTraffic) {
	MJ::PrintInfo::PrintDbg("PlanDayInfo::LoadPoisTraffic");
	for (int i = 1; i < m_pois.size(); i ++) {
		std::string from_Id = m_pois[i-1].id;
		if (!m_pois[i-1].product.isNull() && m_pois[i-1].product.isMember("pid") && m_pois[i-1].product["pid"].isString()) {
			from_Id = m_pois[i-1].product["pid"].asString();
		}
		std::string to_Id = m_pois[i].id;
		if (!m_pois[i].product.isNull() && m_pois[i].product.isMember("pid") && m_pois[i].product["pid"].isString()) {
			to_Id = m_pois[i].product["pid"].asString();
		}
		std::string mid = from_Id + "_" + to_Id;
		Json::Value traffic;
		traffic["id"] = mid;
		if (jTraffic.isMember(mid) && jTraffic[mid].isObject()) {
			traffic["custom"] = 1;
			traffic["eval"] = 0;
			traffic["dur"] = jTraffic[mid]["dur"];
			traffic["dist"] = jTraffic[mid]["dist"];
			traffic["no"] = jTraffic[mid]["no"];
			traffic["type"] = jTraffic[mid]["type"];
		} else {
			MakeTraffic(traffic);
		}
		m_trafficBetweenPoi.push_back(traffic);
	}
}

int PlanDayInfo::LoadPoisData(Json::Value& jPois,const QueryParam& param, const std::string &date,const Json::Value & reqProduct) {
	for (int i = 0; i < jPois.size(); i ++) {
		PoisInfo poi;
		std::string id = jPois[i]["id"].asString();
		MJ::PrintInfo::PrintLog("PlanDayInfo::LoadPoisData, LoadPois %d, %s!", i, id.c_str());
		poi.id = id;
		poi.pdur = jPois[i]["pdur"].asInt();
		if (jPois[i].isMember("custom") && jPois[i]["custom"].isInt()) {
			poi.custom = jPois[i]["custom"].asInt();
		} else {
			poi.custom = 0;
		}
		if (jPois[i].isMember("product")) {
			poi.product = jPois[i]["product"];
		} else {
			poi.product = Json::Value();
		}
		if(poi.product.isMember("product_id"))
		{
			std::string productId = poi.product["product_id"].asString();
			if(reqProduct.isMember("wanle") and reqProduct["wanle"].isObject()
					and reqProduct["wanle"].isMember(productId) and reqProduct["wanle"][productId].isObject())
			{
				poi.product = reqProduct["wanle"][productId];
			}
		}
		//add by yangshu #add productId
		{
			std::string stime;
			if ( jPois[i].isMember("pstime") and jPois[i]["pstime"].isString()) {
				stime = jPois[i]["pstime"].asString();
			}
			if ( jPois[i].isMember("stime") and jPois[i]["stime"].isString()) {
				stime = jPois[i]["stime"].asString();
			}
			if (!poi.product.isNull()) {
				if(poi.product.isMember("stime") and poi.product["stime"].isString()) {
					stime = poi.product["stime"].asString();
				} else {
					poi.product["stime"] = "";
				}
				poi.product["product_id"] = poi.product["pid"].asString()+"#"+date+"#"+stime;
                //同时修改summary中的product_id;
                jPois[i]["product"]["product_id"] = poi.product["product_id"];
				//完善product信息pid,mode,name,lname,date,stime,tickets
				poi.product["date"] = date;

			}
		}
		//add end
		if (jPois[i].isMember("info")) {
			poi.addInfo = jPois[i]["info"];
		} else {
			poi.addInfo = Json::Value();
		}
        poi.rawInfo = jPois[i];
		if(jPois[i].isMember("play") and jPois[i]["play"].isString()) poi.m_play = jPois[i]["play"].asString();
		m_pois.push_back(poi);

	}
	MJ::PrintInfo::PrintLog("PlanDayInfo::LoadPoisData, LoadPois end!");
	return 0;
}

int PlanThroughtPois::GetTrafficDur(Json::Value& jTraffic, Json::Value& jProduct) {
	int trafficDur = 0;
	if (jTraffic.isMember("tickets") && jTraffic["tickets"].isArray() && jTraffic["tickets"].size()>0) {
		Json::Value& jTickets = jTraffic["tickets"];
		Json::Value& first_ticket = jTickets[0u];
		int fIndex = 0;
		if(first_ticket.isMember("index") and first_ticket["index"].asInt() == 1) fIndex = 1;
		if (first_ticket.isMember("product_id") && first_ticket["product_id"].isString()) {
			std::string first_ticketId = first_ticket["product_id"].asString();
			if (jProduct.isMember("traffic") && jProduct["traffic"].isObject()
					&& jProduct["traffic"].isMember(first_ticketId)
					&& jProduct["traffic"][first_ticketId].isObject()
					&& jProduct["traffic"][first_ticketId].isMember("tickets")
					&& jProduct["traffic"][first_ticketId]["tickets"].isArray()
			   ){
				Json::Value& jFirstProductTickets = jProduct["traffic"][first_ticketId]["tickets"];
				if(jFirstProductTickets.size() > fIndex
						and jFirstProductTickets[fIndex].isMember("dur")
						and jFirstProductTickets[fIndex]["dur"].isInt()){
					trafficDur = jFirstProductTickets[fIndex]["dur"].asInt();
				}
			}
		}
	}
	return trafficDur;
}

int PlanThroughtPois::MakeOutputS128(const Json::Value& req, const QueryParam& param, Json::Value& resp) {
	MJ::PrintInfo::PrintLog("PlanDayInfo::MakeOutputS128");
	for (auto it = m_planDays.begin(); it != m_planDays.end(); it ++) {
		Json::Value jDay;
		jDay["date"] = it->first;
		jDay["stime"] = m_cityInfo["dept_time"];
		jDay["etime"] = m_cityInfo["dept_time"];
		Json::Value jDayView;
		std::string stime = jDay["stime"].asString();
		it->second.GetDayViewInfo(param, jDayView, stime);
		jDay["view"] = jDayView;
		m_cityInfo["traffic_pass"]["day"].append(jDay);
	}
	int trafficDur = GetTrafficDur(m_cityInfo["traffic"], m_product);
	const std::string deptTime = m_cityInfo["dept_time"].asString();
	const Json::Value& jViewList = m_cityInfo["day"];
	Json::Value jDays = Json::Value();
	PostProcessor::MakejDays(req, m_cityInfo["day"], jDays);
	for (int i = 0; i < jViewList.size() and i < jDays.size(); i++) {
		Json::Value& jPoisList = m_cityInfo["traffic_pass"]["summary"]["days"][i]["pois"];
		jPoisList.resize(0);
		if (jDays[i].isMember("pois")) jPoisList = jDays[i]["pois"];
	}
	HandleTrafficPass(m_cityInfo["traffic_pass"], param, deptTime, trafficDur);
	resp["data"]["city"] = m_cityInfo;
    // 填充error
    resp["error"]["error_id"] = 0;
    resp["error"]["error_reason"] = "";
	return 0;
}

int PlanDayInfo::GetDayViewInfo(const QueryParam& param, Json::Value& jDayViews, std::string stime) const{
	if (m_pois.size() == 0) {
		jDayViews.resize(0);
		return 0;
	}
	for (int i = 0; i < m_pois.size(); i ++) {
		Json::Value jDayView;
        if(m_pois[i].custom == 1)
        {
            jDayView = m_pois[i].rawInfo;
			if (!jDayView.isMember("play")) jDayView["play"] = "";
        }
        else
        {
            jDayView["id"] = m_pois[i].id;
			if (!m_pois[i].addInfo.isNull()) jDayView["info"] = m_pois[i].addInfo;
            //jDayView["custom"] = m_pois[i].custom;

            const LYPlace *place = LYConstData::GetLYPlace(m_pois[i].id, param.ptid);
            if (!m_pois[i].product.isNull()) {
                jDayView["product"] = m_pois[i].product;
                if (jDayView["product"].isMember("pid") && !jDayView["product"]["pid"].isNull() && jDayView["product"]["pid"].isString()) {
                    const LYPlace *product = LYConstData::GetLYPlace(jDayView["product"]["pid"].asString(), param.ptid);
                    if (product != NULL) {
                        jDayView["product"]["name"] = product->_name;
                    }
                }
            }
            jDayView["type"] = place->_t;
            if (place->_t & LY_PLACE_TYPE_TOURALL) {
                jDayView["type"] = place->_t;
            }
            jDayView["custom"] = place->m_custom;
            jDayView["name"] = place->_name;
            jDayView["lname"] = place->_enname;
            jDayView["coord"] = place->_poi;
			jDayView["play"] = m_pois[i].m_play;

            // note: 景点门票的dur为关联景点的推荐游玩时长
            //const View* view = dynamic_cast<const View*>(place);
            //if (view != NULL) {
            //    std::cerr << "zhangyang id is " << view->_ID << " dur is " << view->_rcmd_intensity->_dur << std::endl;
            //    jDayView["dur"] = view->_rcmd_intensity->_dur;
            //} else {
            //    jDayView["dur"] = m_pois[i].pdur;
            //}

        }

		jDayView["do_what"] = "游玩";
		if (jDayView["type"] == LY_PLACE_TYPE_SHOP)
			jDayView["do_what"] = "购物";
		if (jDayView["type"] == LY_PLACE_TYPE_RESTAURANT)
			jDayView["do_what"] = "就餐";

        jDayView["dur"] = m_pois[i].pdur;
		jDayView["idle"] = Json::Value();
		jDayView["close"] = 0;
		jDayView["dining_nearby"] = 0;

		jDayView["stime"] = stime;
		jDayView["etime"] = MJ::MyTime::toString(MJ::MyTime::toTime(stime, 0) + m_pois[i].pdur, 0, "%Y%m%d_%H:%M");

		Json::Value jTraffic;
		jDayView["traffic"];
		//jTraffic = GetTrafficByIdx(m_pois[i].trafficIdx);
		jTraffic = GetTrafficByIdx(i);
		if (!jTraffic.isNull()) {
			jDayView["traffic"] = jTraffic;
		}
		jDayViews.append(jDayView);
	}
	return 0;
}

Json::Value PlanDayInfo::GetTrafficByIdx(int trafficIdx) const{
	Json::Value traffic;
	if (trafficIdx >= 0 && trafficIdx < m_trafficBetweenPoi.size()) {
		return m_trafficBetweenPoi[trafficIdx];
	}
	return Json::Value();
}

int PlanDayInfo::MakeTraffic(Json::Value& traffic) const{
	traffic["custom"] = 0;
	traffic["eval"] = 0;
	traffic["dur"] = 0;
	traffic["dist"] = 0;
	traffic["no"] = "";
	traffic["type"] = 5;
}

int PlanThroughtPois::HandleTrafficPass(Json::Value& jTrafficPass, const QueryParam& param, const std::string& deptTime, int trafficDur, Json::Value jTourParam, Json::Value jProduct) {
    /*
     * input, jTrafficPass, 待修改的途经点结构
     * input, param
     * input, deptTime, 途经点的起始时间,针对s128， 为“”
     * input, trafficDur, 途经点段交通时长,做报错判断使用，无为0
	 * input, jTourParam, 团游途经点交通不可更改,团游报价是否包含玩乐
	 * input, product, 修改product日期
	 * 目前只做了修改交通，增加warning
     */
    Json::FastWriter jsw;
    MJ::PrintInfo::PrintLog("PlanDayInfo::HandleTrafficPass, before jTrafficPass is %s", jsw.write(jTrafficPass).c_str());
	std::string from_City = "";
	int time_Zone = 0;
	if (jTrafficPass.isMember("from") && jTrafficPass["from"].isString()) {
		from_City = jTrafficPass["from"].asString();
		const City* city = dynamic_cast<const City*>(LYConstData::GetLYPlace(from_City, param.ptid));
		if (city == NULL) {
			MJ::PrintInfo::PrintDbg("[%s]PlanThroughtPois::HandleTrafficPass traffic_pass from  error", param.log.c_str());
			return 1;
		}
		time_Zone = city->_time_zone;
	}
    Json::Value& jDays = jTrafficPass["day"];
	Json::Value& jSummary = jTrafficPass["summary"];
	Json::Value& jSummaryDayList = jSummary["days"];
    for (int i = 0; i < jDays.size() && i<jSummaryDayList.size() && i < 1; i++) {
        Json::Value& jDay = jDays[i];
		Json::Value& jPois = jSummaryDayList[i]["pois"];
        if( deptTime != "" ) {
            jDay["stime"] = deptTime;
            std::vector<std::string> itemlist;
            ToolFunc::sepString(deptTime, "_", itemlist);
            jDay["date"] = itemlist[0];
        }
        std::string poiEtime = jDay["stime"].asString();   //每个poi的结束时间
        for ( int j = 0; j < jDay["view"].size() && j<jPois.size(); j++ ) {
            Json::Value& jPoi = jDay["view"][j];
			int dur = 0;
			if (jPoi["dur"].asInt() == -1) {
				dur = 4*3600;
			}
			else if (jPoi["dur"].asInt() == -2) {
				dur = 8*3600;
			}
			else if (jPoi["dur"].asInt() == -3) {
				dur = 2*3600;
			}
			else {
				dur = jPoi["dur"].asInt();
			}
           // 处理traffic
	        if( j < jDay["view"].size()-1 ) {
            	const Json::Value& jNextPoi = jDay["view"][j+1];
            	std::ostringstream oss;
            	oss << jPoi["id"].asString() << "_" << jNextPoi["id"].asString();
            	std::string  trafKey = oss.str();
    	    	MJ::PrintInfo::PrintLog("PostProcessor::HandleTrafficPass, trafKey is %s", trafKey.c_str());
				if (jTourParam["fixed"].isInt() && jTourParam["fixed"].asInt()) jPoi["traffic"]["fixed"] = jTourParam["fixed"].asInt();
				if (jTrafficPass.isMember("summary") && jTrafficPass["summary"].isObject() && jTrafficPass["summary"].isMember("customTraffic") && jTrafficPass["summary"]["customTraffic"].isObject()) {
					Json::Value& jTraffic = jTrafficPass["summary"]["customTraffic"];
					if (jTraffic.isMember(trafKey)) {
						jPoi["traffic"]["id"] = trafKey;
						jPoi["traffic"]["custom"] = 1;
						jPoi["traffic"]["dur"] = jTraffic[trafKey]["dur"];
						jPoi["traffic"]["dist"] = jTraffic[trafKey]["dist"];
						jPoi["traffic"]["type"] = jTraffic[trafKey]["type"];
						jPoi["traffic"]["no"] = jTraffic[trafKey]["no"];
						MJ::PrintInfo::PrintLog("PlanThroughtPois::HandleTrafficPass, find trafKey is %s", trafKey.c_str());
					}
				}
	        }
            // 处理景点时间
			if (j == 0) {
				jPoi["stime"] = jDay["stime"];
				jPoi["etime"] = MJ::MyTime::toString( MJ::MyTime::toTime(jPoi["stime"].asString(), time_Zone) + dur, time_Zone, "%Y%m%d_%H:%M");
				poiEtime = jPoi["etime"].asString();
			}
			if (j > 0) {
            	const Json::Value& jPrevPoi = jDay["view"][j-1];
	    	    jPoi["stime"] = MJ::MyTime::toString( MJ::MyTime::toTime(poiEtime, time_Zone) + jPrevPoi["traffic"]["dur"].asInt(), time_Zone, "%Y%m%d_%H:%M");
            	jPoi["etime"] = MJ::MyTime::toString( MJ::MyTime::toTime(jPoi["stime"].asString(), time_Zone) + dur, time_Zone, "%Y%m%d_%H:%M");
            	poiEtime = jPoi["etime"].asString();
			}
			//处理product
			if (!jProduct.isNull() && jPoi.isMember("product") && jPoi["product"].isMember("product_id") && jPoi["product"]["product_id"].isString()) {
				std::string prodId = jPoi["product"]["product_id"].asString();
				if (jProduct.isMember(prodId)) {
					Json::Value product = jProduct[prodId];
					product["date"] = jPoi["stime"].asString().substr(0,8);
					product["product_id"] = product["pid"].asString() + "#" + product["date"].asString() + "#" + product["stime"].asString();
					if (jTourParam["noQuotn"].isInt() && jTourParam["noQuotn"].asInt()) product["noQuotn"] = jTourParam["noQuotn"];
					jPoi["product"] = product;

					//处理summary中的product
					if (jPois[j].isMember("product")) jPois[j]["product"] = product;
				}
			}
        }
        jDay["etime"] = poiEtime;
    }

    // ADD Warning
    int trafficPassDur = 0;
	jTrafficPass["summary"]["warning"];
    if (jTrafficPass["summary"].isMember("days") and jTrafficPass["summary"]["days"].isArray()) {
		const Json::Value & days = jTrafficPass["summary"]["days"];
		for(int i = 0; i < days[i].size(); i++)
		{
			if(days[i].isMember("pois") and days[i]["pois"].isArray()){
				const Json::Value & pois = days[i]["pois"];
				for(int j=0; j<pois.size(); j++)
				{
					if(pois[j].isMember("pdur") and pois[j]["pdur"].isInt())
					{
						trafficPassDur += pois[j]["pdur"].asInt();
					}
				}
			}
		}
    }
    Json::Value jWarning = Json::Value();
    MJ::PrintInfo::PrintLog("PostProcessor::HandleTrafficPass, trafficPassDur is %d, trafficDur is %d", trafficPassDur, trafficDur);
    if( trafficPassDur > 0 && trafficDur > 0 && trafficPassDur > trafficDur) {
        std::ostringstream oss, oss1, oss2;

        int trafficPassHour = trafficPassDur/3600;
        int trafficPassMin = (trafficPassDur%3600 + 59)/60;
        if (trafficPassHour != 0 && trafficPassMin != 0)
            oss1 << trafficPassHour << "h" << trafficPassMin << "m";
        else if (trafficPassHour != 0 && trafficPassMin == 0)
            oss1 << trafficPassHour << "h";
        else if (trafficPassHour == 0 && trafficPassMin != 0)
            oss1 << trafficPassMin << "m";

        int trafficHour = trafficDur/3600;
        int trafficMin = (trafficDur%3600 + 59)/60;
        if (trafficHour != 0 && trafficMin != 0)
            oss2 << trafficHour << "h" << trafficMin << "m";
        else if (trafficHour != 0 && trafficMin == 0)
            oss2 << trafficHour << "h";
        else if (trafficHour == 0 && trafficMin != 0)
            oss2 << trafficMin << "m";

        const LYPlace *from = LYConstData::GetLYPlace(jTrafficPass["from"].asString(), param.ptid);
        const LYPlace *to = LYConstData::GetLYPlace(jTrafficPass["to"].asString(), param.ptid);
        if (from != NULL && to != NULL)
            oss << "当前路途中的游玩总时间(" << oss1.str() << ")超出了" << ( (from->_name != "") ? from->_name : from->_lname ) << "-" << ( ( to->_name != "") ? to->_name : to->_lname ) << "的交通时间(" << oss2.str() <<")，请适当调整。";
        else
            oss << "当前路途中的游玩总时间(" << oss1.str() << ")超出了" <<  "交通时间(" << oss2.str() <<")，请适当调整。";

        jWarning["desc"] = oss.str();
        jWarning["type"] = 8;  // 不和s126的报错类型冲突
        jWarning["didx"] = -1;
        jWarning["vidx"] = -1;
        Json::Value& jWarnings = jTrafficPass["summary"]["warning"];
        // 首先删除已有的type 8报错
        for (int i = 0; i < jWarnings.size(); i++) {
            if (jWarnings[i]["type"].asInt() == 8) {
                Json::Value removed;
                jWarnings.removeIndex(i, &removed);
                break;    // 正常只有一个type 8报错
            }
        }
        jWarnings.append(jWarning);
    }
    else { // 检查此时是否有type 8报错
        Json::Value& jWarnings = jTrafficPass["summary"]["warning"];
        for (int i = 0; i < jWarnings.size(); i++) {
            if (jWarnings[i]["type"].asInt() == 8) {
                Json::Value removed;
                jWarnings.removeIndex(i, &removed);
                break;    // 正常只有一个type 8报错
            }
        }
    }

    MJ::PrintInfo::PrintLog("PlanThroughtPois::HandleTrafficPass, after jTrafficPass is %s", jsw.write(jTrafficPass).c_str());
    return 0;
}

int PlanThroughtPois::MakeOutputTrafficPass(Json::Value& req, const QueryParam& param, Json::Value& resp) {
	if (!resp.isMember("data") || !resp["data"].isMember("list") || !resp["data"]["list"].isArray()) {
		return 0;
	}

	if (!req.isMember("list") || !req["list"].isArray() || !req["list"][0].isMember("dept_time")) {
		return 0;
	}

    Json::Value jReqList = req["list"];
    Json::Value& jRespList = resp["data"]["list"];
    for (int i = 0; i < jReqList.size() && i < jRespList.size() && jReqList[i].isObject(); i++) {
		//如果是团游,交通不可修改/查看团游的报价是否包含玩乐
		int trafficFixed = 0;
		int noQuotn = 0;
		if (jReqList[i].isMember("tour") && jReqList[i]["tour"].isMember("product_id") && jReqList[i]["tour"]["product_id"].isString()) {
			std::string productId = jReqList[i]["tour"]["product_id"].asString();
			if (req["product"].isMember("tour") && req["product"]["tour"].isMember(productId)) {
				trafficFixed = 1;
				Json::Value& jTourSet = req["product"]["tour"][productId];
				if (jTourSet.isMember("includeProduct") && jTourSet["includeProduct"].isArray() && jTourSet["includeProduct"].size()>0) {
					for (int i = 0; i < jTourSet["includeProduct"].size(); i++) {
						if (jTourSet["includeProduct"][i].asInt() == 128) {
							noQuotn = 1;
							break;
						}
					}
				}
			}
		}
		Json::Value jTourParam = Json::Value();
		jTourParam["fixed"] = trafficFixed;
		jTourParam["noQuotn"] = noQuotn;
        if (jReqList[i].isMember("traffic_pass")) {
            jRespList[i]["traffic_pass"] = jReqList[i]["traffic_pass"];
            if(jRespList[i]["traffic_pass"].isObject() ) {
                std::vector<std::string> itemlist;
                ToolFunc::sepString(jReqList[i]["dept_time"].asString(), "_", itemlist);
                //jRespList[i]["traffic_pass"]["summary"]["days"][0]["date"] = itemlist[0];
				Json::Value& jTrafficPass = jRespList[i]["traffic_pass"];
				//days.date
				if (jTrafficPass.isMember("summary") && jTrafficPass["summary"].isMember("days") && jTrafficPass["summary"]["days"].isArray()) {
					for (int j = 0; j < jTrafficPass["summary"]["days"].size(); j ++) {
						if (jTrafficPass["summary"]["days"][j].isMember("date")) {
							jTrafficPass["summary"]["days"][j]["date"] = itemlist[0];
						}
					}
				}

                int trafficDur = 0;
				if (jReqList[i]["traffic"].isMember("dur") && jReqList[i]["traffic"]["dur"].isInt()) {
					trafficDur = jReqList[i]["traffic"]["dur"].asInt();
				} else {
					trafficDur = GetTrafficDur(jReqList[i]["traffic"], req["product"]);
				}
				PlanThroughtPois::HandleTrafficPass(jRespList[i]["traffic_pass"], param, jReqList[i]["dept_time"].asString(), trafficDur, jTourParam, req["product"]["wanle"]);
            }
            MJ::PrintInfo::PrintLog("PlanThroughPois::MakeOutputTrafficPass, replace trafficPass for cid %s", jReqList[i]["cid"].asString().c_str());
        }
    }
    return 0;
}
