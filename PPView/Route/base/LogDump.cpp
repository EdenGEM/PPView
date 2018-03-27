#include <iostream>
#include "MJCommon.h"
#include "ToolFunc.h"
#include "LogDump.h"

int LogDump::Dump(const QueryParam& qParam, ErrorInfo& errInfo, int runType, BaseCost& baseCost, BagCost& bagCost, BaseStat& baseStat, BagStat& bagStat, Json::Value& jLog) {
	Json::Value jCost;
	jCost["tot"] = baseCost.m_total / 1000;
	jCost["reqParse"]["tot"] = baseCost.m_reqParse / 1000;
	jCost["keyBuild"]["tot"] = baseCost.m_keyBuild / 1000;
	jCost["blockBuild"]["tot"] = baseCost.m_blockBuild / 1000;
	jCost["dataCheck"]["tot"] = baseCost.m_dataCheck / 1000;
	jCost["postProcess"]["tot"] = baseCost.m_postProcess / 1000;
	jCost["pathEnrich"]["tot"] = baseCost.m_pathEnrich / 1000;
	jCost["traffic"]["8002"] = baseCost.m_traffic8002 / 1000;
	jCost["traffic"]["8003"] = baseCost.m_traffic8003 / 1000;
	jCost["daysPlan"]["tot"] = baseCost.m_daysPlan / 1000;

	jCost["bagSearch"]["tot"] = bagCost.m_bagSearch / 1000;
	jCost["bagSearch"]["root"] = bagCost.m_rootSearch / 1000;
	jCost["bagSearch"]["rich"] = bagCost.m_richSearch / 1000;
	jCost["bagSearch"]["dfs"] = bagCost.m_dfSearch / 1000;
	jCost["bagSearch"]["route"] = bagCost.m_routeSearch / 1000;
	jCost["bagSearch"]["perfect"] = bagCost.m_perfect / 1000;

	Json::Value jStat;
	jStat["path"]["num"] = baseStat.m_placeNum;
	jStat["path"]["hot"] = baseStat.m_hot;
	jStat["path"]["dist"] = baseStat.m_dist;
	jStat["path"]["time"] = baseStat.m_time;
	jStat["path"]["blank"] = ToolFunc::NormSeconds(baseStat.m_blank);
	jStat["path"]["score"] = baseStat.m_score;
	if (baseStat.m_trafNum > 0) {
		jStat["realTraf"]["tot"] = baseStat.m_trafNum;
		jStat["realTraf"]["hit"] = baseStat.m_realTrafNum;
		jStat["realTraf"]["ratio"] = static_cast<int>(baseStat.m_realTrafNum * 100 / baseStat.m_trafNum);
	}
	jStat["bag"]["rootNum"] = bagStat.m_rootNum;
	jStat["bag"]["richNum"] = bagStat.m_richNum;
	jStat["bag"]["dfsNum"] = bagStat.m_dfsNum;
	jStat["bag"]["rootTimeOut"] = bagStat.m_rootTimeOut ? "true" : "false";
	jStat["bag"]["richTimeOut"] = bagStat.m_richTimeOut ? "true" : "false";
	jStat["bag"]["dfsTimeOut"] = bagStat.m_dfsTimeOut ? "true" : "false";
	jStat["bag"]["routeTimeOut"] = bagStat.m_routeTimeOut ? "true" : "false";

//	Json::Value jLog;
	jLog["type"] = qParam.type;
	jLog["run"] = runType;
	jLog["cost"] = jCost;
	jLog["stat"] = jStat;
	jLog["uid"] = qParam.uid;
	jLog["qid"] = qParam.qid;
	jLog["error"]["error_id"] = errInfo.m_errID;
	jLog["error"]["error_reason"] = errInfo.m_errReason;

	Json::FastWriter jw;
	std::string logStr = jw.write(jLog);
	ToolFunc::rtrim(logStr);
	MJ::PrintInfo::PrintLog("[%s]LogDump::Dump, log=%s", qParam.log.c_str(), logStr.c_str());
	return 0;
}

int LogDump::Dump(const QueryParam& qParam, ErrorInfo& errInfo, int runType, BaseCost& baseCost, ProcCost& procCost, BaseStat& baseStat, ProcStat& procStat, Json::Value& jLog) {
	Json::Value jCost;
	jCost["tot"] = baseCost.m_total / 1000;
	jCost["reqParse"]["tot"] = baseCost.m_reqParse / 1000;
	jCost["keyBuild"]["tot"] = baseCost.m_keyBuild / 1000;
	jCost["blockBuild"]["tot"] = baseCost.m_blockBuild / 1000;
	jCost["dataCheck"]["tot"] = baseCost.m_dataCheck / 1000;
	jCost["postProcess"]["tot"] = baseCost.m_postProcess / 1000;
	jCost["pathEnrich"]["tot"] = baseCost.m_pathEnrich / 1000;
	jCost["traffic"]["8002"] = baseCost.m_traffic8002 / 1000;
	jCost["traffic"]["8003"] = baseCost.m_traffic8003 / 1000;

	jCost["processor"]["tot"] = procCost.m_processor / 1000;

	Json::Value jStat;
	jStat["viewNum"]["tot"] = procStat.m_viewNum;
	jStat["shopNum"]["tot"] = procStat.m_shopNum;
	jStat["restNum"]["tot"] = procStat.m_restNum;

//	Json::Value jLog;
	jLog["type"] = qParam.type;
	jLog["run"] = runType;
	jLog["cost"] = jCost;
	jLog["stat"] = jStat;
	jLog["uid"] = qParam.uid;
	jLog["qid"] = qParam.qid;
	jLog["error"]["error_id"] = errInfo.m_errID;
	jLog["error"]["error_reason"] = errInfo.m_errReason;

	Json::FastWriter jw;
	std::string logStr = jw.write(jLog);
	ToolFunc::rtrim(logStr);
	MJ::PrintInfo::PrintLog("[%s]LogDump::Dump, log=%s", qParam.log.c_str(), logStr.c_str());
	return 0;
}

int ErrDump::Dump(const QueryParam& qParam, ErrorInfo& errInfo, Json::Value& req, Json::Value& resp) {
	if (resp.isMember("error") && resp["error"].isMember("error_id") && resp["error"].isMember("error_str")) {
//	   	&& resp["error"]["error_id"].isInt() && resp["error"]["error_id"].asInt() != 0 ) {
		return 0;
	}
	Json::FastWriter jw;
	std::string reqStr = jw.write(req);
	ToolFunc::rtrim(reqStr);
	MJ::PrintInfo::PrintLog("[%s]ErrDump::Dump, errID=%d,errReason=%s,querystring @%s%s@", qParam.log.c_str(), errInfo.m_errID, errInfo.m_errReason.c_str(), qParam.ReqHead().c_str(), reqStr.c_str());
	resp["error"]["error_id"] = errInfo.m_errID;
	resp["error"]["error_reason"] = errInfo.m_errReason;
	if (errInfo.m_errID == 0) return 0;
	if (!errInfo.m_errStr.empty()) {
		resp["error"]["error_str"] = errInfo.m_errStr;
		return 0;
	}
	/*
	if (errInfo.m_errID == 55301
			|| errInfo.m_errID == 55302
			|| errInfo.m_errID == 55303
			|| errInfo.m_errID == 55304
			|| errInfo.m_errID == 55401
			|| errInfo.m_errID == 55402) {
		resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
	} else if (0) {
		resp["error"]["error_str"] = "当前行程的景点开关门时间有冲突";
	} else if (errInfo.m_errID == 55305) {
		resp["error"]["error_str"] = "景点过多，安排不开了，请尝试删除部分景点。";
	} else if (0) {
		resp["error"]["error_str"] = "按当前顺序，部分景点不在开关门时间内，无法进入游览哦~";
	}
	*/
	if (qParam.lang != "en") {
		switch (errInfo.m_errID) {
			case 50000: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55301: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55302: {
				resp["error"]["error_str"] = "回酒店时间太晚。";
				break;
			};
			case 55303: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55304: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55401: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55402: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55403: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55305: {
				resp["error"]["error_str"] = "景点过多，安排不开了，请尝试删除部分景点。";
				break;
			};
			case 53107: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53106: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53110: {	//hy
				resp["error"]["error_str"] = "当日安排过于紧张，不宜安排任何游玩。";
				break;
			};
			case 53111: {	//hy
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55103: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55102: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55105: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 54102: {
				resp["error"]["error_str"] = "该城市停留时间太短，无法规划行程。";
				break;
			};
			case 54101: {
				resp["error"]["error_str"] = "该城市停留时间太短，无法规划行程。";
				break;
			};
			case 54104: {//hyhy
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55104: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53105: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 52101: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 52102: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 51101: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53101: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53102: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53103: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 53104: {
				resp["error"]["error_str"] = "暂时无交通可用。";
				break;
			};
			case 53108: {
				resp["error"]["error_str"] = "验票服务不可用。";
				break;
			};
			case 53109: {
				resp["error"]["error_str"] = "验票服务不可用。";
				break;
			};
			case 52103: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55013: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55012: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55014: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55015: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55016: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55010: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55098: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55099: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 51001: {	//hy
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 55101: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 59001: {
				resp["error"]["error_str"] = "目前的景点方案已是最优";
				break;
			};
			case 59002: {	//hy
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 59003: {	//hy
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
			case 51105: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				//resp["error"]["error_str"] = "到达时间大于离开时间，请修改后重新规划。";
			};
			default: {
				resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
				break;
			};
		}
	} else {
		switch (errInfo.m_errID) {
			case 50000: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55301: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55302: {
				resp["error"]["error_str"] = "Too late to return to hotel.";
				break;
			};
			case 55303: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55304: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55401: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55402: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55403: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55305: {
				resp["error"]["error_str"] = "Too many places to visit. Please attempt to delete some of them before continuing.";
				break;
			};
			case 53107: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53106: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53110: {	//hy
				resp["error"]["error_str"] = "Your stay is too short in this city, Mioji cannot plan your travel.";
				break;
			};
			case 53111: {	//hy
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55103: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55102: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55105: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 54102: {
				resp["error"]["error_str"] = "Your stay is too short in this city, Mioji cannot plan your travel.";
				break;
			};
			case 54101: {
				resp["error"]["error_str"] = "Your stay is too short in this city, Mioji cannot plan your travel.";
				break;
			};
			case 54104: {	//hy
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55104: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53105: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 52101: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 52102: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 51001: {	//hy
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 51101: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53101: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55101: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53102: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53103: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 53104: {
				resp["error"]["error_str"] = "Sorry, there is no route available.";
				break;
			};
			case 52103: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55013: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55012: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55014: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55015: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55016: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55010: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55098: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 55099: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 59001: {
				resp["error"]["error_str"] = "You have the most optimized now";
				break;
			};
			case 59002: {	//hy
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			case 59003: {	//hy
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
			default: {
				resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
				break;
			};
		}
	}
	// jjj todo: offline环境的临时策略
	//resp["error"]["error_str"] = "[PPView]" + resp["error"]["error_reason"].asString();
	return 0;
}

// 规划结果
int DebugInfo::SetPlanErr(int errID, const std::string& errReason) {
	if (errID == 0) {
		snprintf(buff, sizeof(buff), "生成城市内景点路线, 成功");
	} else {
		snprintf(buff, sizeof(buff), "生成城市内景点路线, 失败\t原因: %s", errReason.c_str());
	}
	m_dbgStr += std::string(buff);
	return 0;
}

int DebugInfo::SetS108(int errID, const std::string& errReason, int num) {
	if (errID == 0) {
		snprintf(buff, sizeof(buff), "点击补全景点, 成功补充%d个景点", num);
	} else {
		snprintf(buff, sizeof(buff), "点击补全景点, 失败\t原因: %s", errReason.c_str());
	}
	m_dbgStr += std::string(buff);
}

int DebugInfo::SetWidget(int leftTime, double trafRate, int intensity) {
	snprintf(buff, sizeof(buff), "widget状态: 剩余时间%s, 交通时间占比%d%%, 强度为%s", ToolFunc::NormSeconds(leftTime).c_str(), static_cast<int>(trafRate), (intensity == 3?"紧张":(intensity == 2?"适中":"轻松")));
	if (!m_dbgStr.empty()) {
		m_dbgStr += "\t";
	}
	m_dbgStr += std::string(buff);
	return 0;
}


int DebugInfo::SetPlaceList(int errID, const std::string& errReason, int placeSum) {
	if (errID == 0) {
		snprintf(buff, sizeof(buff), "加载景点/餐馆列表页，可用数量%d条", placeSum);
	} else {
		snprintf(buff, sizeof(buff), "列表生成失败\t原因%s", errReason.c_str());
	}
	m_dbgStr += std::string(buff);
	return 0;
}

int DebugInfo::SetTrafDetail(int errID, const std::string& errReason, int resultSum){
	if (errID == 0) {
		snprintf(buff, sizeof(buff), "加载交通详情页，可用数量%d条", resultSum);
	} else {
		snprintf(buff, sizeof(buff), "列表生成失败\t原因%s", errReason.c_str());
	}
	m_dbgStr += std::string(buff);
	return 0;
}
//int DebugInfo::SetCost(long long timeCost) {
//	snprintf(buff, sizeof(buff), ", 耗时%.1fs", timeCost / 1000.0 / 1000);
//	m_dbgStr += std::string(buff);
//}

const std::string& DebugInfo::GetDbgStr() {
	return m_dbgStr;
}
