#ifndef _PLANNER_H_
#define _PLANNER_H_

#include <iostream>
#include "Route/base/Utils.h"
#include "MJCommon.h"
#include "Route/bag/BagPlan.h"
#include "Route/bag/MyThreadPool.h"
#include "Route/bag/ThreadMemPool.h"
#include "Route/basis/DaysPlan.h"

class Planner {
public:
	Planner() {
	}
	~Planner() {
	}
public:
	static int DoPlan(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  BagPlan * bagPlan = NULL);
	static int AvailDurLevel(const QueryParam& param, Json::Value& req);
public:
	static int DoPlanSSV005(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  BagPlan * bagPlan=NULL);
	static int DoPlanSSV006(const QueryParam& param, Json::Value& req, Json::Value& resp, BagPlan* bagPlan=NULL);
	static int DoPlanS125_SSV006(const QueryParam& param, Json::Value& req, Json::Value& resp, BagPlan* bagPlan=NULL);
	static int DoPlanSSV007(const QueryParam& param, Json::Value& req, Json::Value& resp);
	static int DoPlanS126(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool);
	//
	static int DoPlanS201(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool);
	static int DoPlanS202(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool);
	static int DoPlanS203 (const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool);
	static int DoPlanS205(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool);
	static int DoPlanS131 (const QueryParam& param, Json::Value& req, Json::Value& resp);

	static int DoPlanBag(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool, BagPlan* bagPlan=NULL);
	static int GetIntervalDay(Json::Value& req);
	static Json::Value GetReferTrafficPass(const Json::Value& req, const Json::Value& referTrips);
	static void SetReferOriginViews(Json::Value& reqList, const Json::Value& referTrips, int isTourCity);
	static int MakeS131toSSV006 (const QueryParam& param, Json::Value& req, Json::Value& resp);
private:
	//获取s131首点游玩日期
	static std::string getDate (const Json::Value& jReq);
public:
	static const int kBagDayLimit = 365; //要支持120天，超过一定天数会bagSearch中途会失败，直接使用贪心算法
};

class SmartOptWorker : public MyWorker {
	public:
		SmartOptWorker(const QueryParam param,Json::Value req):m_param(param), m_req(req){
			m_plan = new BagPlan;
			m_threadPool = new MyThreadPool;
			m_threadPool->open(BagParam::m_threadNum, 204800000);
			m_threadPool->activate();
		}
		virtual ~SmartOptWorker(){
			delete m_plan;
			delete m_threadPool;
		}
	public:
		int doWork(ThreadMemPool* tmPool){
			LYConstData::SetQueryParam(&m_param);
			m_ret = Planner::DoPlan(m_param, m_req, m_resp, m_threadPool, m_plan);
		}
	private:
		const QueryParam m_param;
		Json::Value m_req;
		MyThreadPool * m_threadPool;
	public:
		Json::Value m_resp;
		BagPlan* m_plan;
		int m_ret;
};

#endif
