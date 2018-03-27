#include <iostream>
#include <algorithm>
#include <limits>
#include "Route/base/define.h"
#include "Route/base/BasePlan.h"
#include "Route/base/BagParam.h"
#include "Route/base/ToolFunc.h"
#include "MJCommon.h"
#include "Processor.h"
#include "Planner.h"
#include "Route.h"
//zt
#include "Route/bag/StaticRand.h"
//zt

int Route::LoadStatic(const std::string& data_path, const std::string& conf_path) {
	if (!RouteConfig::init(data_path,conf_path)) {
		MJ::PrintInfo::PrintErr("Route::LoadStatic, Route Config init failed!");
		return 1;
	}

	if (BagParam::Init(data_path) != 0) {
		MJ::PrintInfo::PrintErr("Route::LoadStatic, BagParam Init failed!");
		return 1;
	}

	MJ::PrintInfo::PrintLog("Route::LoadStatic, initial CONST DATA...");
	if (!LYConstData::init()) {
		MJ::PrintInfo::PrintErr("Route::LoadStatic, initial CONST DATA failed!");
		return 1;
	}
	MJ::PrintInfo::PrintLog("Route::LoadStatic, initial CONST DATA OK");


//    if (MJ::StaticRand::Init() != 0) {
//		MJ::PrintInfo::PrintErr("Route::loadStatic, initial static rand failed!");
//		return 1;
//	}
	MJ::PrintInfo::PrintLog("Route::loadStatic, initial static rand OK");

	StaticRand::Init();
	return 0;
}

int Route::ReleaseStatic() {
//zt
	StaticRand::Release();
//zt
	LYConstData::destroy();
	return 0;
}

int Route::DoRoute(const QueryParam& param, Json::Value& req, Json::Value& resp) {
	LYConstData::SetQueryParam(&param);
	int ret = 1;
	std::string type = param.type;
	Json::FastWriter fw;
	fw.omitEndingLineFeed();
	if (type.empty()) {
		std::string reqStr = fw.write(req);
		ToolFunc::rtrim(reqStr);
		MJ::PrintInfo::PrintErr("[%s]Route::DoRoute, no type in request: %s", param.log.c_str(), reqStr.c_str());
		return 1;
	}

	if (type == "p201" || type == "p202" || 
			type == "p104" || type == "p105" || type == "b116" || type == "p101") {
		MJ::MyTimer timer;
		timer.start();
		ret = Processor::Process(param, req, resp);
		int cost = timer.cost();
		MJ::PrintInfo::PrintLog("[%s][STATS]query-type=%s,cost=%d, query=%s,,,ret=%s", param.log.c_str(), param.type.c_str(), cost, fw.write(req).c_str(), fw.write(resp).c_str());
	} else if (RouteConfig::needPlanner && (
				type == "s125" || type == "s129" || type == "s130" || type == "s128" || type == "s131" || 
				type == "s201" || type == "s202" || type == "s203" || type == "s204" || type == "s205")) {
		MJ::MyTimer timer;
		timer.start();
		ret = Planner::DoPlan(param, req, resp, _threadPool);
		int cost = timer.cost();
		MJ::PrintInfo::PrintLog("[%s][STATS]query-type=%s,cost=%d, query=%s,,,ret=%s", param.log.c_str(), param.type.c_str(), cost, fw.write(req).c_str(), fw.write(resp).c_str());
	} else {
		ret = 1;
		resp["error"]["error_id"] = 1;
		if (param.lang != "en") {
			resp["error"]["error_str"] = "抱歉，服务器没有响应，请稍后再试。";
			resp["error"]["error_reason"] = "暂不支持请求类型";
		} else {
			resp["error"]["error_str"] = "Sorry, Mioji is busy now, please try again later.";
		}
	}
	return ret;
}


int Route::Init() {
	if (RouteConfig::needPlanner) {
		_threadPool = new MyThreadPool;
		_threadPool->open(BagParam::m_threadNum, 204800000);
		_threadPool->activate();
	} else {
		_threadPool = NULL;
	}
	return 0;
}

int Route::Release() {
	if (_threadPool) {
		delete _threadPool;
		_threadPool = NULL;
	}
	return 0;
}

int Route::AvailDurLevel(const QueryParam& param, Json::Value& req) {
	return Planner::AvailDurLevel(param, req);
}
