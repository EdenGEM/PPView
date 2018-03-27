#include <iostream>
#include <cstring>
#include "Route/base/BasePlan.h"
#include "Route/bag/BagPlan.h"
#include "Route/base/ReqParser.h"
#include "Route/base/ReqChecker.h"
#include "Route/base/KeyNodeBuilder.h"
#include "Route/base/DataChecker.h"
#include "Route/base/TrafRoute.h"
#include "Route/base/PathPerfect.h"
#include "Route/base/PathUtil.h"
#include "Route/base/PathEval.h"
#include "Route/base/RealTimeTraffic.h"
#include "Route/base/TrafficPair.h"
#include "Route/bag/BagSearch.h"
#include "PostProcessor.h"
#include "Planner.h"
#include "LightPlan.h"
#include "PlanThroughtPois.h"
#include "TourSet.h"

class PathViewOptionCmp {
private:
	int Init() {
		selectMode = 0; //默认同时优化顺序和时长
		if(m_req["option"].isObject())
		{
			int _playTimeRange= 0;
			if(m_req["option"]["play_time_range"].isInt()) _playTimeRange = m_req["option"]["play_time_range"].asInt();
			int _playOrder= 0;
			if(m_req["option"]["play_order"].isInt()) _playOrder = m_req["option"]["play_order"].asInt();
			int _playDur= 0;
			if(m_req["option"]["play_dur"].isInt()) _playDur = m_req["option"]["play_dur"].asInt();
			//优先突破时长，后突破顺序，最后突破回酒店时间
			selectMode = (1-_playTimeRange)*2*2+(1-_playOrder)*2+(1-_playDur);
			_INFO("selectMode:%d",selectMode);
		}
		return 0;
	}
	Json::Value m_req;
	int selectMode;
public:
	bool operator() (const std::pair<int, PathView*>& lpathPair, const std::pair<int, PathView*>& rpathPair) const {

		//if(lpathPair.second->Length() != rpathPair.second->Length()) return lpathPair.second->Length()>rpathPair.second->Length();

		/*比较算法说明:
		 * selectMode 一共有3个二进制位,从高到低依次表征是否保持 出发返回时间、顺序和景点游玩时长
		 *			高低位的安排表示了优先突破景点游玩时长,其次是顺序,最后突破出发返回时间
		 *			智能优化安排了8个线程进行规划,线程序号可按selectMode来解释:
		 *				如5号线程,其二进制为101,则其规划时,保持出发返回时间、不保持顺序但保持景点游玩时长
		 *
		 *由线程号和selectMode来计算线程得分:
		 *	1. 先对两个值求 同或 得a
		 *	2. 对于a中值为1的位,如果该位在selectMode中也为1则有较大加分,否则加个小分
		 */
		int lIdx = lpathPair.first;
		int tlVal = ~(lIdx ^ selectMode);
		int lVal = 0;
		for(int i=0; i<3; i++)
		{
			int t = 1 << i;
			int forAdd = t&tlVal;
			if(t & selectMode) lVal += forAdd;
			else lVal += forAdd*1.0/(1<<3);
		}

		int rIdx = rpathPair.first;
		int trVal = ~(rIdx ^ selectMode);
		int rVal = 0;
		for(int i=0; i<3; i++)
		{
			int t = 1 << i;
			int forAdd = t&trVal;
			if(t & selectMode) rVal += forAdd;
			else rVal += forAdd*1.0/(1<<3);
		}

		return lVal > rVal;
	}
	PathViewOptionCmp(const Json::Value& req):m_req(req) {
		Init();
	}
};
int Planner::DoPlan(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  BagPlan * bagPlan) {
	int ret = 0;
	if (param.type == "ssv005_rich" || param.type == "ssv005_s130") {
		ret = DoPlanSSV005(param, req, resp, threadPool, bagPlan);
	} else if (param.type == "ssv006_light" || param.type == "s204"
			|| param.type == "s129") {
		ret = DoPlanSSV006(param, req, resp, bagPlan);
	} else if (param.type == "s201"
			|| param.type == "s127") {
		ret = DoPlanS201(param, req, resp, threadPool);
		Json::FastWriter jw;
		std::cerr<<"hyhy s201 ret "<<ret<<" res "<<jw.write(resp)<<std::endl;
    } else if (param.type == "s205"
			|| param.type == "s128") {
		ret = DoPlanS205(param, req, resp, threadPool);
    } else if (param.type == "s203"
			|| param.type == "s130") {
		ret = DoPlanS203(param, req, resp, threadPool);
	} else if (param.type == "s202"
			|| param.type == "s125") {
		ret = DoPlanS202(param, req, resp, threadPool);
	} else if (param.type == "s131") {
		ret = DoPlanS131(param, req, resp);
	}
	return ret;
}
int Planner::DoPlanS131(const QueryParam& param, Json::Value& req, Json::Value& resp) {
	MJ::MyTimer t;
	t.start();
	int ret = MakeS131toSSV006(param, req, resp);
	//006返回error_id 非0 错误
	if (ret) {
		resp["error"]["error_reason"] = "当前行程部分数据被删除，无法导入至团游";
		resp["error"]["error_str"] = "当前行程部分数据被删除，无法导入至团游";
	} else {
		resp["error"]["error_id"] = 0;
		resp["error"]["error_reason"] = "";
		resp["error"]["error_str"] = "";
	}
	std::cerr << "s131:cost: " << t.cost() << std::endl;
	return ret;
}
int Planner::DoPlanS202(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool) {
	if (req.isMember("city") && req["city"].isMember("tour")) {
		QueryParam qParam = param;
		qParam.type = "ssv007";
		req["city"]["ridx"] = req["ridx"];
		DoPlanSSV007(qParam, req, resp);
	}
	else {
		DoPlanSSV006(param, req, resp);
	}
	PostProcessor::MakeOutputProduct(req, resp);
}
// 团游相关 修改酒店/自定义交通/规划时添加团游
int Planner::DoPlanSSV007(const QueryParam& param, Json::Value& req, Json::Value& resp) {
	int ret = 0;
	BagPlan* plan = new BagPlan;
	plan->qType = param.type;
	TourCityInfo tourCity;
	ret = tourCity.DoPlan(param, plan, req, resp);
	if (ret and !resp.isMember("data")) {
		resp["data"]["view"] = Json::Value();
		Json::Value& jView = resp["data"]["view"];
		jView["expire"] = 1;
		jView["day"] = Json::arrayValue;
		jView["summary"]["days"] = Json::arrayValue;
	}

	Json::Value jLog;
	LogDump::Dump(plan->m_qParam, plan->m_error, plan->m_runType, plan->m_cost, plan->m_bagCost, plan->m_stat, plan->m_bagStat, jLog);
	ErrDump::Dump(plan->m_qParam, plan->m_error, req, resp);
	if (plan) {
		delete plan;
		plan=NULL;
	}
	return ret;
}
// ssv005:	指定POI规划
int Planner::DoPlanSSV005(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  BagPlan* bagPlan) {
	int ret = 0;
	int intervalDay = GetIntervalDay(req);
	if (intervalDay > kBagDayLimit) {
		resp["error"]["error_id"] = 1;
		char buff[1000] = {};
		if (param.lang != "en") {
			snprintf(buff, sizeof(buff), "暂不支持%d天以上。", kBagDayLimit);
		} else {
			snprintf(buff, sizeof(buff), "No more than %d days!", kBagDayLimit);
		}
		resp["error"]["error_str"] = std::string(buff);
		return 0;
	}

	ret = DoPlanBag(param, req, resp, threadPool, bagPlan);
	return ret;
}


int Planner::DoPlanBag(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  BagPlan* bagPlan) {
	int ret = 0;
	MJ::MyTimer t;
	MJ::MyTimer total;
	total.start();

	BagPlan* plan = NULL;
	if(bagPlan) plan = bagPlan;
	else plan = new BagPlan;
	plan->qType = param.type;

	t.start();
	ret = ReqParser::DoParse(param, req, plan);
	plan->m_cost.m_reqParse = t.cost();

	if (ret == 0) {
		ret = TrafficPair::GetTraffic(plan);
	}

	if (ret == 0) {
		t.start();
		ret = KeyNodeBuilder::BuildKeyNode(req, plan);
		plan->m_cost.m_keyBuild = t.cost();
	}

	if (ret == 0) {
		t.start();
		ret = BlockBuilder::BuildBlock(req, plan);
		plan->m_cost.m_blockBuild = t.cost();
	}

	if (ret == 0) {
		t.start();
		ret = DataChecker::DoCheck(plan);
		plan->m_cost.m_dataCheck = t.cost();
	}

	if (ret == 0) {
		t.start();
		ret = BagSearch::DoSearch(plan, threadPool);
		if(ret == 0)
		{
			plan->m_bagCost.m_bagSearch = t.cost();
		}

		if( ret ){
			plan->m_error.Set(55402, "规划失败");
			ret = 1;
		}
		else
		{
			PathUtil::TimeEnrich(plan, plan->m_PlanList);
			PathUtil::PerfectDelSet(plan, plan->m_PlanList);
		}
	}


	if (ret == 0 &&(plan->m_realTrafNeed & REAL_TRAF_REPLACE) && plan->m_useRealTraf) {
		ret = RealTimeTraffic::DoReplace(plan, plan->m_PlanList);
		plan->m_PlanList.Dump(plan, true);
		MJ::PrintInfo::PrintDbg("[%s] RealTimeTraffic::DoReplace end", plan->m_qParam.log.c_str());

		if (ret == 0) {
			MJ::PrintInfo::PrintDbg("[%s]second  Stretch ", plan->m_qParam.log.c_str());
			PathUtil::TimeEnrich(plan, plan->m_PlanList);
			plan->m_PlanList.Dump(plan, true);
			MJ::PrintInfo::PrintLog("[%s] Second Stretch End! ", plan->m_qParam.log.c_str());
		}
		else if (ret != 0 && plan->m_error.m_errID == 0) {
			plan->m_error.Set(55099, "RealTimeTraffic内部未报error");
		}
	}

	if (ret == 0) {
		plan->m_planStats.m_leftTime = PathUtil::CalIdel(plan, plan->m_PlanList);
		plan->m_planStats.m_trafTime = PathUtil::CalTraf(plan, plan->m_PlanList);
		PathUtil::SetDur2PlanStats(plan, plan->m_PlanList);
	}


	Json::Value jStats;
	Json::FastWriter fw;
	plan->m_planStats.GetStats(jStats);
	MJ::PrintInfo::PrintLog("[%s] PlanStats=%s", plan->m_qParam.log.c_str(), fw.write(jStats).c_str());

 	if (ret == 0) {
		MJ::PrintInfo::PrintLog("[%s]PlatUtil::Norm15Min......", plan->m_qParam.log.c_str());
		ret = PathUtil::Norm15Min(plan);
		plan->m_PlanList.Dump(plan, true);
		MJ::PrintInfo::PrintLog("[%s]PlatUtil::Norm15Min END", plan->m_qParam.log.c_str());
	}

	// Warning: 保证DoPerfect之后 不再修改PathView!!!
	if (ret == 0) {
		t.start();
		ret = PathPerfect::DoPerfect(plan);
		plan->m_bagCost.m_perfect = t.cost();
	}
	plan->m_cost.m_total = total.cost();
	plan->m_PlanList.Dump(plan, true);

	t.start();
	std::vector<PathView*> pathList;
	ret = PathUtil::CutPathBySegment(plan, plan->m_PlanList, pathList);
	if (!ret) {
		if (req.isMember("list")) {
			resp["data"]["list"].resize(0);
			for (int i = 0 ; i < pathList.size(); i ++) {
				Json::Value jResp;
				plan->m_PlanList.Reset();
				plan->m_PlanList.Copy(pathList[i]);
				PostProcessor::PostProcess(req, plan, jResp);

				jResp["data"]["city"]["view"]["summary"]["arv_time"] = req.isMember("list") ? req["list"][i]["city"]["arv_time"] : req["city"]["arv_time"];
				jResp["data"]["city"]["view"]["summary"]["dept_tTime"] = req.isMember("list") ? req["list"][i]["city"]["dept_time"] : req["city"]["dept_time"];
				jResp["data"]["city"]["view"]["summary"]["hotel"] = req.isMember("list") ? req["list"][i]["city"]["hotel"] : req["city"]["hotel"];
				jResp["data"]["city"]["view"]["summary"]["traffic_in"] = req.isMember("list") ? req["list"][i]["city"]["traffic_in"] : req["city"]["traffic_in"];
				Json::StyledWriter jsw;
	//			std::cerr << jsw.write(jResp) << std::endl;
				Json::Value jView;
	//			std::cerr << jsw.write(jResp) << std::endl;
	//			std::cerr << jsw.write(req) << std::endl;
				jView["view"] = Json::Value();
				if (jResp.isMember("data")) {
					resp["data"]["list"].append(jResp["data"]["city"]);
				}
				delete pathList[i];
			}
		} else {
			PostProcessor::PostProcess(req, plan, resp);
		}
	}

	plan->m_cost.m_postProcess = t.cost();

//	MJ::PrintInfo::PrintDbg("-----------cur-----------  cost : %.3lf", t.cost() / 1000.0);
	Json::Value jLog;
	LogDump::Dump(plan->m_qParam, plan->m_error, plan->m_runType, plan->m_cost, plan->m_bagCost, plan->m_stat, plan->m_bagStat, jLog);
	if (plan->m_useKpi) {
		resp["kpi"]["logdump"] = jLog;
		resp["kpi"]["planStats"] = jStats;
	}
	ErrDump::Dump(plan->m_qParam, plan->m_error, req, resp);
	fprintf(stderr, "miojiCost\t%s\t%d\t%d\t%d\t%d\t%d\n", plan->m_qParam.qid.c_str(), plan->m_bagCost.m_bagSearch, plan->m_bagCost.m_rootSearch, plan->m_bagCost.m_richSearch, plan->m_bagCost.m_dfSearch, plan->m_bagCost.m_routeSearch);

	if (plan != bagPlan) {
		delete plan;
		plan = NULL;
	}
	return ret;
}

int Planner::GetIntervalDay(Json::Value& req) {
	if (req.isMember("city")
			&& req["city"].isMember("arv_time") && req["city"]["arv_time"].isString()
			&& req["city"].isMember("dept_time") && req["city"]["dept_time"].isString()) {
		std::string arvTime = req["city"]["arv_time"].asString();
		std::string deptTime = req["city"]["dept_time"].asString();
		int stayDay = MJ::MyTime::compareTimeStr(arvTime, deptTime) / (24 * 3600);
		return stayDay;
	} else if (req.isMember("list")) {
		int stayDay = 0;
		for (int i = 0 ; i < req["list"].size(); i++) {
			std::string arvTime = req["list"][i]["city"]["arv_time"].asString();
			std::string deptTime = req["list"][i]["city"]["dept_time"].asString();
			stayDay += MJ::MyTime::compareTimeStr(arvTime, deptTime);
		}
		stayDay /= (24 * 3600);
		return stayDay;
	}
	return 100;
}

int Planner::AvailDurLevel(const QueryParam& param, Json::Value& req) {
	int ret = 0;
	int level = -1;

	BagPlan* plan = new BagPlan;
	ret = ReqParser::DoParse(param, req, plan);
	if (ret == 0) {
		ret = TrafficPair::GetTraffic(plan);
	}
	if (ret == 0) {
		ret = KeyNodeBuilder::BuildKeyNode(req, plan);
	}
	if (ret == 0) {
		ret = BlockBuilder::BuildBlock(req, plan);
	}
	if (ret == 0) {
		level = plan->AvailDurLevel();
	}

	if (plan) {
		delete plan;
		plan = NULL;
	}
	return level;
}

Json::Value Planner::GetReferTrafficPass(const Json::Value& req,const Json::Value& referTrips) {
	const Json::Value& basicReqParam = req;

	if( basicReqParam.isMember("ridx")
			and basicReqParam["ridx"].isInt()
			and basicReqParam.isMember("cityNum")
			and basicReqParam["cityNum"].isInt()
			and basicReqParam["ridx"].asInt()+1 == basicReqParam["cityNum"].asInt()) return Json::nullValue;

	Json::Value jRet = Json::nullValue;
	std::vector<Json::Value> referRouteList;
	std::string cid = basicReqParam["cid"].asString();
	for (int i = 0; i < referTrips.size(); ++i) {
		if (referTrips[i].isMember("route")) {
			for (int j = 0; j+1 < referTrips[i]["route"].size(); ++j) {
				if (referTrips[i]["route"][j]["cid"].asString() == cid) {
					std::string nextCity = basicReqParam["nextCid"].asString();
					if (referTrips[i]["route"][j]["traffic_pass"]["to"] != nextCity) {
						continue;
					}
					referRouteList.push_back(referTrips[i]["route"][j]);
				}
			}
		}
	}
	if (!referRouteList.empty()) {
		int reqSeconds = MJ::MyTime::compareTimeStr(basicReqParam["arv_time"].asString(), basicReqParam["dept_time"].asString());
		int referID = -1;
		int minDiff = std::numeric_limits<int>::max();
		for (int i = 0; i < referRouteList.size(); ++i) {
			int seconds = MJ::MyTime::compareTimeStr(referRouteList[i]["arv_time"].asString(), referRouteList[i]["dept_time"].asString());
			int diff = abs(seconds - reqSeconds);
			if (diff < minDiff) {
				minDiff = diff;
				referID = i;
			}
		}
		if(referID >=0) jRet = referRouteList[referID]["traffic_pass"];
		_INFO("referID:%d", referID);
	}
	return jRet;
}

void Planner::SetReferOriginViews(Json::Value& reqList, const Json::Value& referTrips, int isTourCity)
{
	Json::FastWriter fw;

	std::multimap<int , std::pair<int,int>> playDurDist;
	for(int k = 0; k < reqList.size(); k++)
	{
		Json::Value & basicReqParam = reqList[k];
		basicReqParam["originView"] = Json::nullValue;

		int reqSeconds = MJ::MyTime::compareTimeStr(basicReqParam["arv_time"].asString(), basicReqParam["dept_time"].asString());
		std::string cid = basicReqParam["cid"].asString();
		for (int i = 0; i < referTrips.size(); ++i) {
			if (referTrips[i].isMember("route")) {
				for (int j = 1; j+1 < referTrips[i]["route"].size(); ++j) {
					const Json::Value & referCity = referTrips[i]["route"][j];
					if (isTourCity && !referCity.isMember("tour")) continue;
					if (!isTourCity && referCity.isMember("tour")) continue;
					if (referCity["cid"].asString() == cid) {
						int seconds = MJ::MyTime::compareTimeStr(referCity["arv_time"].asString(), referCity["dept_time"].asString());
						int diff = abs(seconds - reqSeconds);
						playDurDist.insert(std::make_pair(diff,std::make_pair(k,100*i+j)));
					}
				}
			}
		}
	}

	std::set<int> reqRefered,viewRefered;
	for(auto it = playDurDist.begin(); it != playDurDist.end(); it++)
	{
		int reqCityIdx = it->second.first;
		int viewIdx = it->second.second;
		if(reqRefered.find(reqCityIdx) == reqRefered.end() and viewRefered.find(viewIdx) == viewRefered.end())
		{
			int tripIdx = viewIdx/100;
			int routeIdx = viewIdx%100;
			reqList[reqCityIdx]["originView"] = referTrips[tripIdx]["route"][routeIdx]["view"];
			if (referTrips[tripIdx]["route"][routeIdx].isMember("checkin") && referTrips[tripIdx]["route"][routeIdx]["checkin"].isString() && referTrips[tripIdx]["route"][routeIdx]["checkin"].asString().size() == 8
					&& referTrips[tripIdx]["route"][routeIdx].isMember("checkout") && referTrips[tripIdx]["route"][routeIdx]["checkout"].isString() && referTrips[tripIdx]["route"][routeIdx]["checkout"].asString().size() == 8) {
				reqList[reqCityIdx]["originView"]["checkin"] = referTrips[tripIdx]["route"][routeIdx]["checkin"];
				reqList[reqCityIdx]["originView"]["checkout"] = referTrips[tripIdx]["route"][routeIdx]["checkout"];
			}
			reqRefered.insert(reqCityIdx);
			viewRefered.insert(viewIdx);

			std::cerr<<"the "<<reqCityIdx <<" "<<reqList[reqCityIdx]["cname"].asString()+"("+reqList[reqCityIdx]["cid"].asString()+") "
				<<"ridx: "<<reqList[reqCityIdx]["ridx"].asInt()<<" refer to tripIdx: "<<tripIdx<<" routeIdx:"<<routeIdx<<std::endl;
			std::cerr<<"durDist: "<<it->first<<" referView: "<<fw.write(reqList[reqCityIdx]["originView"])<<std::endl;
		}
	}

	return;
}
int Planner::DoPlanSSV006(const QueryParam& param, Json::Value& req, Json::Value& resp,BagPlan* bagPlan) {
	int ret = 0;
	MJ::MyTimer t;
	MJ::MyTimer total;

	t.start();
	total.start();
	BagPlan* plan = NULL;
	if(bagPlan) plan = bagPlan;
	else plan = new BagPlan;

	ret = ReqParser::DoParse(param, req, plan);
	plan->m_cost.m_reqParse = t.cost();

	std::cerr << "before daysPlan cost time : " << t.cost() << std::endl;

	t.start();
	if (ret == 0) {
		DaysPlan* daysPlan = new DaysPlan;
		daysPlan->GetPlanResp(plan, req, resp);
		if (daysPlan) {
			delete daysPlan;
			daysPlan = NULL;
		}
	}
	plan->m_cost.m_daysPlan = t.cost();
	plan->m_cost.m_total = total.cost();
	Json::Value jLog;
	LogDump::Dump(plan->m_qParam, plan->m_error, plan->m_runType, plan->m_cost, plan->m_bagCost, plan->m_stat, plan->m_bagStat, jLog);
	ErrDump::Dump(plan->m_qParam, plan->m_error, req, resp);
	if (plan != bagPlan) {
		delete plan;
		plan = NULL;
	}
	return ret;
}

//检查并修改过期城市内行程
int Planner::DoPlanS126(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool) {
	int ret = 0;
	MJ::MyTimer t;

	ErrorInfo error;
	LightPlan* lightPlan = new LightPlan;

	t.start();

	int ppMode = 0;//非法值被格式化为0
	if (req.isMember("pp_mode") && req["pp_mode"].isInt()) {
		int tppMode = req["pp_mode"].asInt();
		if(tppMode<=2 and tppMode>=0) ppMode=tppMode;
		for (int i = 0; i < req["list"].size(); ++i) {
			req["list"][i]["pp_mode"] = ppMode;
		}
	}

	bool headCity = false, tailCity = false;
	int headCidx = 0, tailCidx = req["list"].size() - 1;
	if (req["list"].size() > 0
			and req["list"][headCidx].isMember("ridx")
			and req["list"][headCidx]["ridx"].isInt()
			and req["list"][headCidx]["ridx"].asInt() == 0){
		headCity = true;
		headCidx += 1;
	}
	if (req["list"].size() > 0
			and req["list"][tailCidx].isMember("ridx")
			and req["list"][tailCidx]["ridx"].isInt()
			and req["list"][tailCidx].isMember("cityNum")
			and req["list"][tailCidx]["cityNum"].isInt()
			and req["list"][tailCidx]["ridx"].asInt()+1 == req["list"][tailCidx]["cityNum"].asInt()){
		tailCity = true;
		tailCidx -= 1;
	}

	Json::Value reqOri = req, respOri;
	reqOri["list"].resize(0);
	for (int i = headCidx; i <= tailCidx; ++ i) {
		reqOri["list"].append(req["list"][i]);
	}
	if (reqOri["list"].size() > 0) {
		ret = lightPlan->PlanS126(param, reqOri, respOri, threadPool, error);
	}

	Json::Value jRespHead, jRespTail;
	jRespHead["traffic_pass"] = req["list"][0]["traffic_pass"];
	jRespTail["traffic_pass"] = Json::Value();
	Json::Value jList = respOri["data"]["list"];
	resp["error"] = respOri["error"];
	resp["data"] = Json::Value();
	resp["data"]["list"] = Json::Value(Json::arrayValue);

	if (headCity) {
		resp["data"]["list"].append(jRespHead);
	}
	for (int i = 0; i < respOri["data"]["list"].size(); ++ i) {
		resp["data"]["list"].append(respOri["data"]["list"][i]);
	}
	if (tailCity) {
		resp["data"]["list"].append(jRespTail);
	}

	ErrDump::Dump(param, error, req, resp);
	fprintf(stderr, "miojiCost %d\n", t.cost() / 1000);

	Json::FastWriter jw;
	std::cerr<<"hyhy res final\n"<<jw.write(resp)<<std::endl;

	if (lightPlan) {
		delete lightPlan;
		lightPlan = NULL;
	}
	PostProcessor::SetDurNegative1ForCList(resp);
	return ret;
}

int Planner::DoPlanS201 (const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool) {
	int ret = 0;
	char buf[10240];
	Json::Value jError;
	std::tr1::unordered_map<std::string, std::tr1::unordered_set<int> > cityIdxListMap;
	if (!req.isMember("list") || !req["list"].isArray() || !req["list"].size() > 0) {
		resp["error"]["error_id"] = 51101;
		resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
		resp["error"]["error_reason"] = "请求格式错误:list";
		return 0;
	}
	if (!req.isMember("referTrip") || !req["referTrip"].isArray()) {
		resp["error"]["error_id"] = 51101;
		resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
		resp["error"]["error_reason"] = "请求格式错误:referTrip";
		return 0;
	}
	//list中city按照cid分组
	Json::Value& jList = req["list"];
	std::string tourId = "";
	for (int i = 0; i < jList.size(); i ++) {
		if(req["localTest"].isInt() and req["localTest"].asInt())
		{
			jList[i]["ridx"]=i;
			jList[i]["cityNum"]=jList.size();
			if(i+1<jList.size()) jList[i]["nextCid"] = jList[i+1]["cid"].asString();
			if (jList[i].isMember("tour") && jList[i]["tour"].isMember("product_id") && jList[i]["tour"]["product_id"].isString() && jList[i]["tour"]["product_id"].asString() != tourId) {
				std::string productId = jList[i]["tour"]["product_id"].asString();
				if (req["product"]["tour"].isMember(productId)) {
					tourId = productId;
					Json::Value& jTourRoute = req["product"]["tour"][productId]["route"];
					int idx = i;
					if (jTourRoute.size() > 0 && jTourRoute[0u]["cid"] == jList[i]["cid"]) {
						for (int j = 0; j < jTourRoute.size(); j ++) {
							jTourRoute[j]["ridx"] = idx;
							idx++;
							_INFO("add city: %s, ridx: %d", jList[idx]["cid"].asString().c_str(), idx);
						}
					}
				}
			}
		}
		Json::Value& jCity = jList[i];
		if (jCity.isMember("cid") && jCity["cid"].isString()) {
			std::string cityID = jCity["cid"].asString();
			cityIdxListMap[cityID].insert(i);
		}
	}

	int ppMode = 0;//非法值被格式化为0
	if (req.isMember("ppMode") && req["ppMode"].isInt()) {
		int tppMode = req["ppMode"].asInt();
		if(tppMode<=2 and tppMode>=0) ppMode=tppMode;
	}
	if (ppMode) {
		req["referTrip"].resize(0);
	}

	int action = 0;//非法值被格式化为0
	if (req.isMember("action") && req["action"].isInt()) {
		int _action = req["action"].asInt();
		if(_action<=1 and _action>=0) action=_action;
	}

	bool hasSuccess = false;
	//按组分组打请求
	std::tr1::unordered_map<int, Json::Value> idx2JsonMap;
	std::tr1::unordered_map<std::string, std::tr1::unordered_set<int> >::iterator cidIt;
	for (cidIt = cityIdxListMap.begin(); cidIt != cityIdxListMap.end(); cidIt ++) {
		_INFO("PlanCity: %s", cidIt->first.c_str());
		std::tr1::unordered_set<int>::iterator idxIt;
		std::tr1::unordered_set<int>& idxSet = cidIt->second;
		Json::Value jLightPlanReq, jTourSetReq;
		jLightPlanReq["list"].resize(0);
		jTourSetReq["list"].resize(0);
		if(action == 1) jLightPlanReq["isChangeTrip"] = 1;
		//根据是否为团游 分请求
		for (idxIt = idxSet.begin(); idxIt != idxSet.end(); idxIt ++) {
			const Json::Value & oneReq = jList[*idxIt];
			bool isTourCity = false;
			Json::Value jtour;
			int rsize = 0;
			if (oneReq.isMember("tour") && oneReq["tour"].isObject() && oneReq["tour"].isMember("product_id") && oneReq["tour"]["product_id"].isString()) {
				std::string proId = oneReq["tour"]["product_id"].asString();
				if (req["product"].isMember("tour") && req["product"]["tour"].isMember(proId)) {
					Json::Value& jTourRoute = req["product"]["tour"][proId]["route"];
					_INFO("%s is tourCity", cidIt->first.c_str());
					for (rsize=0; rsize < jTourRoute.size(); rsize++) {
						std::string cid = cidIt->first;
						if (jTourRoute[rsize]["cid"].asString() == cid && jTourRoute[rsize]["ridx"] == oneReq["ridx"]) {
							isTourCity = true;
							jtour["tourId"] = proId;
							jtour["idx"] = rsize;
							jTourSetReq["list"].append(oneReq);
							jTourSetReq["list"][jTourSetReq["list"].size()-1]["traffic_pass"] = jTourRoute[rsize]["traffic_pass"];
							_INFO("tourSet insert %s", cid.c_str());
							break;
						}
					}
				}
			} else {
				jLightPlanReq["list"].append(oneReq);
				if(action==0) jLightPlanReq["list"][jLightPlanReq["list"].size() - 1]["use17Limit"] = 1; //17点逻辑~
				ReqParser::NewCity2OldCity(param , jLightPlanReq["list"][jLightPlanReq["list"].size() - 1]);
				jLightPlanReq["list"][jLightPlanReq["list"].size() - 1]["traffic_pass"] = GetReferTrafficPass(oneReq, req["referTrip"]);
				MJ::PrintInfo::PrintLog("[%s]Planner::DoPlanS201, idx %d", param.log.c_str(), (*idxIt));
				Json::FastWriter jw;
				std::cerr<<"hyhy idx "<<(*idxIt)<<" refer \n"<<jw.write(jLightPlanReq["list"][jLightPlanReq["list"].size() - 1]["traffic_pass"])<<std::endl;
			}
			if (isTourCity) {
				jLightPlanReq["tourCity"][jtour["tourId"].asString()].append(rsize);
			}
		}
		jLightPlanReq["product"] = req["product"];
		SetReferOriginViews(jLightPlanReq["list"], req["referTrip"], 0);
		SetReferOriginViews(jTourSetReq["list"], req["referTrip"], 1);
		if (req["referTrip"].isArray() && req["referTrip"].size() > 0) {
			Json::Value& jReferTrips = req["referTrip"];
			for (int i = 0; i < jReferTrips.size(); i++) {
				Json::Value& jReferTrip = jReferTrips[i];
				if (jReferTrip.isMember("product") && jReferTrip["product"].isObject() && jReferTrip["product"].isMember("wanle") && jReferTrip["product"]["wanle"].isObject()) {
					Json::Value::Members jMem = jReferTrip["product"]["wanle"].getMemberNames();
					for (Json::Value::Members::iterator it = jMem.begin(); it != jMem.end(); it++) {
						if (!req["product"]["wanle"].isMember(*it)) {
							req["product"]["wanle"][*it] = jReferTrip["product"]["wanle"][*it];
						}
					}
				}
			}
		}
		jLightPlanReq["product"]["wanle"] = req["product"]["wanle"];
		if (req["product"].isObject() && req["product"].isMember("tour") && req["product"]["tour"].isObject()) {
			Json::Value::Members jMem = req["product"]["tour"].getMemberNames();
			for (Json::Value::Members::iterator jmemIt = jMem.begin(); jmemIt != jMem.end(); jmemIt++) {
				Json::Value& jTour = req["product"]["tour"][*jmemIt];
				if (jTour.isMember("product") && jTour["product"].isMember("wanle") && jTour["product"]["wanle"].isObject()) {
					Json::Value& jtourWanle = jTour["product"]["wanle"];
					Json::Value::Members jWanleMem = jtourWanle.getMemberNames();
					for (Json::Value::Members::iterator jwanleIt = jWanleMem.begin(); jwanleIt!= jWanleMem.end(); jwanleIt++) {
						if (!req["product"]["wanle"].isMember(*jwanleIt)) {
							req["product"]["wanle"][*jwanleIt] = jtourWanle[*jwanleIt];
						}
					}
				}
			}
		}
		jLightPlanReq["cityPreferCommon"] = req["cityPreferCommon"];
		jLightPlanReq["pp_mode"] = ppMode;
		//修改/参考行程，打s126
		Json::Value jResp;
		QueryParam queryParam = param;
		queryParam.type = "s126";
		MJ::PrintInfo::PrintLog("[%s]Planner::DoPlanS201, cid = %s", param.log.c_str(), (cidIt->first).c_str());
		if (jLightPlanReq["list"].isArray() && jLightPlanReq["list"].size()>0) {
			ret = DoPlanS126(queryParam, jLightPlanReq, jResp, threadPool);
		}
		if (jResp["data"]["list"].isArray() && jResp["data"]["list"].size()>0) {
			PlanThroughtPois::MakeOutputTrafficPass(jLightPlanReq, queryParam, jResp);
		}
		if (jResp["error"]["error_id"] != 0) {
			for (int i = 0; i < jLightPlanReq["list"].size(); i ++) {
				Json::Value jViewItem;
				jViewItem["error"] = jResp["error"];
				jViewItem["view"] = Json::nullValue;
				jViewItem["traffic_pass"] = Json::nullValue;
				int cidx = jLightPlanReq["list"][i]["ridx"].asInt();
				idx2JsonMap[cidx] = jViewItem;
			}
		} else {
			hasSuccess = true;
			int i = 0;
			for (i = 0; i < jLightPlanReq["list"].size(); i ++) {
				Json::Value jViewItem;
				jViewItem["error"] = jResp["error"];
				jViewItem["view"] = jResp["data"]["list"][i]["view"];
				jViewItem["traffic_pass"] = jResp["data"]["list"][i]["traffic_pass"];
				int cidx = jLightPlanReq["list"][i]["ridx"].asInt();
				idx2JsonMap[cidx] = jViewItem;
			}
		}

		//团游相关打007
		Json::Value jTourResp = Json::Value();
		//jTourTrafficPassReq 专门用来处理途经点
		Json::Value jTourTrafficPassReq = Json::Value();
		jTourTrafficPassReq["list"] = Json::arrayValue;
		jTourTrafficPassReq["product"] = req["product"];
		jTourResp["data"]["list"] = Json::arrayValue;
		QueryParam tourParam = param;
		tourParam.type = "ssv007";
		for (int tourReqSize = 0; tourReqSize < jTourSetReq["list"].size(); tourReqSize++) {
			Json::Value jtourReq = Json::Value();
			Json::Value jtourResp;
			jtourReq["city"] = jTourSetReq["list"][tourReqSize];
			jtourReq["product"] = req["product"];
			_INFO("DoPlanSSV007: %d",tourReqSize);
			int ret = DoPlanSSV007(tourParam, jtourReq, jtourResp);
			//换酒店失败 or 场次不可用
			if (ret) {
				if (!jtourReq["city"]["originView"].isNull()) {
					jtourResp["data"]["view"] = jtourReq["city"]["originView"];
				}
				Json::Value& jView = jtourResp["data"]["view"];
				jView["expire"] = 1;
				Json::Value jWarningList = Json::arrayValue;
				if (jtourResp["data"].isMember("error") && jtourResp["data"]["error"].isArray() && jtourResp["data"]["error"].size()>0 && jtourResp["data"]["error"][0u].isMember("didx") && jtourResp["data"]["error"][0u]["didx"].isInt()) {
					Json::Value jWarning = Json::Value();
					jWarning["vidx"] = -1;
					jWarning["didx"] = jtourResp["data"]["error"][0u]["didx"];
					jWarning["type"] = 11;
					jWarning["desc"] = "当前所选酒店会导致城市内交通时间过长，无法保证晚上合理的休息时间，请重新选择酒店。";
					jWarningList.append(jWarning);

					int didx = jtourResp["data"]["error"][0u]["didx"].asInt();
					if (didx >= jView["summary"]["days"].size()) didx = 0;
					if (jView["summary"]["days"].size()) {
						jView["summary"]["days"][didx]["expire"] = 1;
					}
				}
				jtourResp["data"]["view"]["summary"]["warning"] = jWarningList;
			}
			jTourResp["data"]["list"].append(jtourResp["data"]);
			jTourTrafficPassReq["list"].append(jtourReq["city"]);
		}
		if (jTourResp["data"]["list"].isArray() && jTourResp["data"]["list"].size()>0) {
			PlanThroughtPois::MakeOutputTrafficPass(jTourTrafficPassReq, tourParam, jTourResp);
		}
		for (int tourSize = 0; tourSize < jTourResp["data"]["list"].size(); tourSize++) {
			Json::Value& jresp = jTourResp["data"]["list"][tourSize];
			int cidx = jTourTrafficPassReq["list"][tourSize]["ridx"].asInt();
			hasSuccess = true;
			Json::Value jViewItem;
			Json::Value jError;
			jError["error_id"] = 0;
			jError["error_reason"] = "";
			jViewItem["error"] = jError;
			jViewItem["view"] = jresp["view"];
			jViewItem["traffic_pass"] = jresp["traffic_pass"];
			if (jViewItem["view"].isNull() && jViewItem["traffic_pass"].isNull()) continue;
			idx2JsonMap[cidx] = jViewItem;
		}
	}
	resp["data"]["list"].resize(0);
	std::vector<std::string> failCityList;
	for (int i = 0 ; i < jList.size(); i ++) {
		int cidx = -1;
		if (jList[i].isMember("ridx") && jList[i]["ridx"].isInt()) cidx = jList[i]["ridx"].asInt();
		if (idx2JsonMap.find(cidx) != idx2JsonMap.end()) {
			resp["data"]["list"].append(idx2JsonMap[cidx]);
			Json::Value& jView = resp["data"]["list"][i]["view"];
			if (jView.isMember("summary") && jView["summary"].isMember("missPoi") && jView["summary"]["missPoi"].size()) {
				jView["summary"]["missPoi"].resize(0);
			}
		} else {
			if (!jList[i].isMember("cid") || !jList[i]["cid"].isString()) {
				char buff[1024] = {};
				snprintf(buff, sizeof(buff), "exception_cid_idx_%d", i);
				failCityList.push_back(std::string(buff));
				Json::Value jViewItem;
				jViewItem["view"] = Json::Value();
				jViewItem["traffic_pass"] = Json::nullValue;
				jViewItem["error"]["error_id"] = "50000";
				jViewItem["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				jViewItem["error"]["error_reason"] = "异常的城市cid";
				resp["data"]["list"].append(jViewItem);

			} else {
				std::string cityId = req["list"][i]["cid"].asString();
				const LYPlace* place = LYConstData::GetLYPlace(cityId, "");
				std::string cname = "";
				if (!place) {
					cname = cityId;
				} else {
					cname = place->_name;
				}
				failCityList.push_back(cname);
				Json::Value jViewItem;
				jViewItem["view"] = Json::Value();
				jViewItem["traffic_pass"] = Json::nullValue;
				jViewItem["error"]["error_id"] = "50000";
				jViewItem["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				jViewItem["error"]["error_reason"] = "异常的城市错误";
				resp["data"]["list"].append(jViewItem);
			}
		}
	}

	if (!failCityList.empty()) {
		snprintf(buf, sizeof(buf), "%s", failCityList[0].c_str());
		for (int j = 1; j < failCityList.size(); ++j) {
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ", %s", failCityList[j].c_str());
		}
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "的市内规划出现异常，您可以前往该城市手动进行规划。");
		resp["data"]["warning"] = buf;
	} else {
		resp["data"]["warning"] = Json::Value();
	}

	if (!hasSuccess) {
		resp["error"]["error_id"] = 59002;
		resp["error"]["error_reason"] = "请求失败";
		resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
	} else {
		resp["error"]["error_id"] = 0;
		resp["error"]["error_reason"] = "";
		resp["error"]["error_str"] = "";
	}
	if (req["list"].size() != resp["data"]["list"].size()) {
		resp["error"]["error_id"] = 59004;
		resp["error"]["error_reason"] = "返回list长度与请求不一致";
		resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
	}

	PostProcessor::SetDurNegative1ForCList(resp);
	PostProcessor::MakeOutputProduct(req, resp);

	return ret;
}

int Planner::DoPlanS205(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool) {
	int ret = 0;
	MJ::MyTimer t;

	ErrorInfo error;
	PlanThroughtPois planThroughtPois;// = new PlanThroughtPois;

	t.start();

	ret = planThroughtPois.PlanPath(param, req, resp, error);

	ErrDump::Dump(param, error, req, resp);
	fprintf(stderr, "miojiCost %d\n", t.cost() / 1000);

	Json::FastWriter jw;
	std::cerr<<"shyy res final\n"<<jw.write(resp)<<std::endl;
	PostProcessor::MakeOutputProduct(req, resp);

	return ret;
}

int Planner::DoPlanS203(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool) {
	int ret = 0;
	MJ::MyTimer t;
	t.start();

	int req_avail = 0;
	QueryParam queryParam = param;
	if(req["city"].isObject() and req["city"]["view"].isObject()
			and req["city"]["view"]["expire"].isInt() and req["city"]["view"]["expire"].asInt())
	{
		req_avail = 0;
	}
	else
	{
		//先走ssv006
		Json::Value jReq = req;
		queryParam.type = "ssv006_light";
		BagPlan* bagPlan = new BagPlan;
		DoPlanSSV006(queryParam, jReq, resp, bagPlan);
		if (resp.isMember("data") && resp["data"].isMember("city") and resp["data"]["city"].isMember("view") and resp["data"]["city"]["view"].isMember("expire") and resp["data"]["city"]["view"]["expire"].isInt() and !resp["data"]["city"]["view"]["expire"].asInt()) {
			req["city"] = resp["data"]["city"];
			req_avail = 1;
		}
		if (bagPlan) {
			delete bagPlan;
			bagPlan = NULL;
		}
	}
	std::cerr<<"req_avail:"<<req_avail<<std::endl;

	resp = Json::Value();
	std::vector<std::pair<QueryParam,Json::Value>> workerParams;
	Json::Value s130Req = req;
	s130Req["inner"]["delPoi"] = 1;
	//~(playTimeRange|playOrder|playDur)  线程号与请求对应关系
	//int threadIdx[] = {0}; //可通过设置线程号 调试
	//for (int i:threadIdx) {
	for (int i = 0; i < 8; i++) {
		std::bitset<3> option(i);
		//playTimeRange order keepTime
		//option 从高到低依次为 出发/返回时间 顺序 景点时长
		//为1 表示保留  0 表示未保留(突破)
		if (option.test(0)) {
			s130Req["cityPreferCommon"]["keepTime"] = 1;
		} else {
			s130Req["cityPreferCommon"]["keepTime"] = 0;
		}
		if (option.test(1)) {
			queryParam.type = "ssv006_light";
		} else {
			queryParam.type = "ssv005_s130";
		}
		if (option.test(2)) {
			s130Req["cityPreferCommon"]["keepPlayTimeRange"] = 1;
		} else {
			s130Req["cityPreferCommon"]["keepPlayTimeRange"] = 0;
		}
		workerParams.push_back(std::make_pair(queryParam,s130Req));
	}
	
	Json::Value jRemainWarningList;
	if (req.isMember("city") and req["city"].isMember("view")
			and req["city"]["view"].isMember("summary") and req["city"]["view"]["summary"].isMember("warning")
			and req["city"]["view"]["summary"]["warning"].isArray() and req["city"]["view"]["summary"]["warning"].size() > 0) {
		jRemainWarningList = req["city"]["view"]["summary"]["warning"];
	}

	Json::Value lockDateList;
	ReqParser::ParseLockDay(req["days"], lockDateList, jRemainWarningList);
	req["notPlanDays"] = lockDateList;

	std::vector<MyWorker*> jobs;
	for(auto it= workerParams.begin(); it!=workerParams.end(); it++)
	{
		SmartOptWorker * smartOptWorker = new SmartOptWorker(it->first,it->second);
		jobs.push_back(dynamic_cast<MyWorker*>(smartOptWorker));
		threadPool->add_worker(dynamic_cast<MyWorker*>(smartOptWorker));
	}
	threadPool->wait_worker_done(jobs);

	std::vector<std::pair<int,PathView*>> pathViews;
	for (int i = 0; i < jobs.size(); ++i) {
		SmartOptWorker* smartOptWorker = dynamic_cast<SmartOptWorker*>(jobs[i]);
		if(smartOptWorker->m_ret==0 && (smartOptWorker->m_resp["data"].isMember("city"))
				and smartOptWorker->m_resp["data"]["city"].isMember("view") and smartOptWorker->m_resp["data"]["city"]["view"].isMember("expire")
				and smartOptWorker->m_resp["data"]["city"]["view"]["expire"].isInt() and !smartOptWorker->m_resp["data"]["city"]["view"]["expire"].asInt()) {
			pathViews.push_back(std::make_pair(i, &(smartOptWorker->m_plan->m_PlanList)));
		}
		_INFO("s130 pathview dump idx:%d",i);
		smartOptWorker->m_plan->m_PlanList.Dump(smartOptWorker->m_plan,true);
		std::cerr<<"s130 resp dump idx:" <<i<<std::endl<<smartOptWorker->m_resp<<std::endl;
	}
	if(pathViews.size()==0)
	{
		Json::Value error;
		error["error_id"]=0;
		error["error_reason"]="";
		Json::Value data;
		data["stat"]=0;
		resp["error"]=error;
		resp["data"]=data;

		for (int i = 0; i < jobs.size(); ++i) delete jobs[i];
		return 0;
	}
	_INFO("smartOpt success num: %d",pathViews.size());
	//暂时先选最后一个
	Json::Value jResp;
	BagPlan * plan;
	PathViewOptionCmp pathCmp(req);
	std::stable_sort(pathViews.begin(), pathViews.end(), pathCmp);
	int selectIdx = pathViews.front().first;
	_INFO("smartOpt sort selectIdx:%d",selectIdx);
	{
		//因为keepTime而删点始终不如非keepTime而不删点(或删点更少)
		if(selectIdx & 0x0001)
		{
			SmartOptWorker* smartOptWorker = dynamic_cast<SmartOptWorker*>(jobs[selectIdx]);
			Json::Value& selectResp = smartOptWorker->m_resp;
			int selectMissPoi = 0, tIdxMissPoi = 0;
			if (selectResp.isMember("data") and selectResp["data"].isMember("city") and selectResp["data"]["city"].isMember("view") 
					and selectResp["data"]["city"]["view"].isMember("summary") and selectResp["data"]["city"]["view"]["summary"].isMember("missPoi") and selectResp["data"]["city"]["view"]["summary"]["missPoi"].isArray()) {
				selectMissPoi = selectResp["data"]["city"]["view"]["summary"]["missPoi"].size();
			}
			int tIdx = selectIdx -1;
			for(int i=0; i<pathViews.size(); i++)
			{
				if(pathViews[i].first == tIdx)
				{
					SmartOptWorker* tIdxOptWorker = dynamic_cast<SmartOptWorker*>(jobs[tIdx]);
					Json::Value& tIdxResp = tIdxOptWorker->m_resp;
					if (tIdxResp.isMember("data") and tIdxResp["data"].isMember("city") and tIdxResp["data"]["city"].isMember("view") 
							and tIdxResp["data"]["city"]["view"].isMember("summary") and tIdxResp["data"]["city"]["view"]["summary"].isMember("missPoi") and tIdxResp["data"]["city"]["view"]["summary"]["missPoi"].isArray()) {
						tIdxMissPoi = tIdxResp["data"]["city"]["view"]["summary"]["missPoi"].size();
					}
					if(selectMissPoi > tIdxMissPoi)
					{
						selectIdx = tIdx;
						break;
					}
				}
			}
		}
		SmartOptWorker* selectedWorker = dynamic_cast<SmartOptWorker*>(jobs[selectIdx]);
		_INFO("smartOpt success idx:%d",selectIdx);
		jResp = selectedWorker->m_resp;
		plan = selectedWorker->m_plan;
	}
	resp["data"]["city"] = jResp["data"]["city"];
	resp["error"] = jResp["error"];

	plan->m_SelectedOptMode = selectIdx;
	plan->m_qParam.type = "s130";
	plan->s130reqAvail = req_avail;
	ret = PostProcessor::PostProcess(req, plan, resp);
	PostProcessor::MakeOutputProduct(req, resp);
	std::cerr << "s130 -- resp:" << std::endl << resp << std::endl;
	plan->m_cost.m_postProcess = t.cost();

	//保留锁定天的warning
	if (resp["data"]["city"].isMember("view") and resp["data"]["city"]["view"].isMember("summary")
			and (resp["data"]["city"]["view"]["summary"]["warning"].isNull() or resp["data"]["city"]["view"]["summary"]["warning"].isArray())) {
		for (int i = 0; i < jRemainWarningList.size(); i++) {
			resp["data"]["city"]["view"]["summary"]["warning"].append(jRemainWarningList[i]);
		}
	}
	Json::Value jLog;
	LogDump::Dump(plan->m_qParam, plan->m_error, plan->m_runType, plan->m_cost, plan->m_bagCost, plan->m_stat, plan->m_bagStat, jLog);
	ErrDump::Dump(plan->m_qParam, plan->m_error, req, resp);

	if (plan->m_qParam.uid == "test") {
		resp["kpi"]["logdump"] = jLog;
//		resp["kpi"]["planStats"] = jStats;
	}

	for (int i = 0; i < jobs.size(); ++i) delete jobs[i];

	return ret;
}

int Planner::MakeS131toSSV006 (const QueryParam& param, Json::Value& req, Json::Value& resp) {
	Json::Value jReq = req;
	bool onlyOneCity = false;
	if (req.isMember("route") && req["route"].isArray()) {
		if (req["route"].size() == 1) onlyOneCity = true;
	} else {
		return 1;
	}
	jReq["ridx"] = 0;
	jReq["city"] = req["route"][0u];
	if (onlyOneCity) jReq["city"]["dept_poi"] = Json::Value();
	bool onlyOneDay = false;
	Json::Value jDaysList = Json::arrayValue;
	Json::Value& jDayList = jReq["city"]["view"]["day"];

	PostProcessor::MakejDays(req,jDayList,jDaysList);
	jReq["days"] = jDaysList;

	if (jReq["days"].isArray() && jReq["days"].size() ==1) {
		onlyOneDay = true;
	}
	if (onlyOneCity and onlyOneDay) {
		jReq["needEtime"] = 1;
	}
	//pstime str 内部使用
	if (req.isMember("pstime") and req["pstime"].isString()) {
		if (req.isMember("needView") and req["needView"].isInt() and req["needView"].asInt()) {
			jReq["needView"] = 1;
		}
		std::string stime = req["pstime"].asString();
		if (req["pstime"].asString().length() == 4) {
			stime = "0"+stime;
		}
		std::string date = getDate(jReq);
		jReq["city"]["arv_time"] = date+"_"+stime;
		jReq["city"]["arv_poi"] = Json::Value();
		if (onlyOneCity and onlyOneDay and jReq["city"]["dept_time"].isString()) {
			if (jReq["city"]["dept_time"].asString() < (date + "_" + "23:59")) {
				jReq["city"]["dept_time"] = date + "_" + "23:59";
			}
		}
		int ret = DoPlanSSV006(param, jReq, resp);
		if (resp.isMember("data") and resp["data"].isMember("view")) {
			PostProcessor::MakeOutputProduct(req, resp);
		}
		return ret;
	} else if (req.isMember("pstime") and req["pstime"].isArray()) {
		for (int i = 0; i < req["pstime"].size(); i++) {
			Json::Value jreq = jReq;
			Json::Value jResp = Json::Value();
			if (!req["pstime"][i].isString()) continue;
			std::string stime = req["pstime"][i].asString();
			if (stime.length() == 4) {
				stime = "0"+stime;
			}
			std::string date = getDate(jreq);
			jreq["city"]["arv_time"] = date+"_"+stime;
			jreq["city"]["arv_poi"] = Json::Value();
			if (onlyOneCity and onlyOneDay and jreq["city"]["dept_time"].isString()) {
				if (jreq["city"]["dept_time"].asString() < (date + "_" + "23:59")) {
					jreq["city"]["dept_time"] = date + "_" + "23:59";
				}
			}
			std::string deptTime = "";
			if (jreq.isMember("city") and jreq["city"].isMember("dept_time") and jreq["city"]["dept_time"].isString() and jreq["city"]["dept_time"].asString().length() == 14) {
				deptTime = jreq["city"]["dept_time"].asString().substr(9);
			}

			if (jreq["city"]["arv_time"] > jreq["city"]["dept_time"]) {
				jResp["type"] = 1;
				if (deptTime != "") {
					jResp["content"] = "您选择的开始时间，将导致"+deptTime+"出发的交通不可使用，请重新选择";
				}
			} else {
				Json::Value jresp;
				int ret = DoPlanSSV006(param, jreq, jresp);
				jResp = jresp["data"];
				if (ret) {
					//格式错误亦返回了数据删除报错
					resp = jresp;
					return ret;
				}
			}
			resp["data"]["list"].append(jResp);
		}
	} else {
		return 1;
	}
	//可能arvTIme和 deptTime 还需要修改
	return 0;
}
std::string Planner::getDate (const Json::Value& jReq) {
	std::string date = jReq["city"]["view"]["day"][0u]["date"].asString();
	//取第一个点的游玩日期
	if (jReq["city"]["view"]["day"][0u].isMember("view") and jReq["city"]["view"]["day"][0u]["view"].isArray() and jReq["city"]["view"]["day"][0u]["view"].size()>0
			and jReq["city"]["view"]["day"][0u]["view"][0u].isMember("stime") and jReq["city"]["view"]["day"][0u]["view"][0u]["stime"].isString()
			and jReq["city"]["view"]["day"][0u]["view"][0u]["stime"].asString().length() == 14) {
		date = jReq["city"]["view"]["day"][0u]["view"][0u]["stime"].asString().substr(0,8);
	}
	return date;
}
