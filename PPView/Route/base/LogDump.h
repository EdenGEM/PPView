#ifndef _LOG_DUMP_H_
#define _LOG_DUMP_H_

#include <iostream>
#include "Utils.h"

// errorID说明
// 格式: 5#@$***
// 说明:
// 第一位5：表示PPView
// 第二位#：出错类别
// 0：默认
// 1：请求有问题
// 2：下游服务有问题
// 3：加载数据有问题
// 4：用户修改有问题
// 5：搜索算法有问题
// 9：其它
// 第三位@：出错位置
// 0：默认
// 1：base目录
// 2：ProcPlan
// 3：ACOPlan
// 4：BagPlan
// 第四位$：预留
// 第五-七位：各出错类别下的子ID，用于定位具体代码位置

class ErrorInfo {
public:
	ErrorInfo() {
		m_errID = 0;
	}
	ErrorInfo(int errID) {
		Set(errID);
	}
	ErrorInfo(int errID, const std::string& errReason) {
		Set(errID, errReason);
	}
	ErrorInfo(int errID, const std::string& errReason, const std::string& errStr) {
		Set(errID, errReason, errStr);
	}
public:
	int Set(int errID) {
		m_errID = errID;
	}
	int Set(int errID, const std::string& errReason) {
		m_errID = errID;
		m_errReason = errReason;
	}
	int Set(int errID, const std::string& errReason, const std::string& errStr) {
		m_errID = errID;
		m_errReason = errReason;
		m_errStr = errStr;
	}
public:
	int m_errID;
	std::string m_errReason;
	std::string m_errStr;
};

// 给debug人员看的自然语言调试信息
class DebugInfo {
public:
//	int SetCost(long long timeCost);
	const std::string& GetDbgStr();

	int SetS108(int errID, const std::string& errReason, int num);
	int SetWidget(int leftTime, double trafRate, int intensity);
	int SetPlanErr(int errID, const std::string& errReason);
	int SetPlaceList(int errID, const std::string& errReason, int placeSum);
	int SetTrafDetail(int errID, const std::string& errReason, int resultSum);
private:
	std::string m_dbgStr;
	char buff[100];
};


class BaseStat {
public:
	BaseStat() {
		m_placeNum = 0;
		m_hot = 0;
		m_dist = 0;
		m_time = 0;
		m_blank = 0;
		m_score = 0;
		m_trafNum = 0;
		m_realTrafNum = 0;
	}
public:
	int m_placeNum;
	int m_hot;
	int m_dist;
	int m_time;
	int m_blank;
	int m_score;

	int m_trafNum;
	int m_realTrafNum;
};

class ProcStat {
public:
	int m_viewNum;
	int m_shopNum;
	int m_restNum;
public:
	ProcStat() {
		m_viewNum = -1;
		m_shopNum = -1;
		m_restNum = -1;
	}
};

class BagStat {
public:
	BagStat() {
		m_rootNum = 0;
		m_richNum = 0;
		m_dfsNum = 0;
		m_rootTimeOut = false;
		m_richTimeOut = false;
		m_dfsTimeOut = false;
		m_routeTimeOut = false;
	}
public:
	int m_rootNum;
	int m_richNum;
	int m_dfsNum;
	bool m_rootTimeOut;
	bool m_richTimeOut;
	bool m_dfsTimeOut;
	bool m_routeTimeOut;
};

class BaseCost {
public:
	BaseCost() {
		m_total = -1;
		m_reqParse = -1;
		m_keyBuild = -1;
		m_blockBuild = -1;
		m_postProcess = -1;
		m_dataCheck = -1;
		m_trafInterface = -1;
		m_traffic8002 = -1;
		m_traffic8003 = -1;
		m_traffic8009 = -1;
		m_pathEnrich = -1;
		m_daysPlan = -1;
	}
public:
	int m_total;
	int m_reqParse;
	int m_keyBuild;
	int m_blockBuild;
	int m_dataCheck;
	int m_postProcess;
	int m_trafInterface;
	int m_traffic8002;
	int m_traffic8003;
	int m_traffic8009;
	int m_pathEnrich;
	int m_daysPlan;
};

class ProcCost {
public:
	ProcCost() {
		m_processor = -1;
	}
public:
	int m_processor;
};

class BagCost {
public:
	BagCost() {
		m_bagSearch = -1;
		m_rootSearch = -1;
		m_richSearch = -1;
		m_dfSearch = -1;
		m_routeSearch = -1;
		m_perfect = -1;
	}
public:
	int m_bagSearch;
	int m_rootSearch;
	int m_richSearch;
	int m_dfSearch;
	int m_routeSearch;
	int m_perfect;
};


class LogDump {
public:
	static int Dump(const QueryParam& qParam, ErrorInfo& errInfo, int runType, BaseCost& baseCost, ProcCost& procCost, BaseStat& baseStat, ProcStat& procStat, Json::Value& jLog);
	static int Dump(const QueryParam& qParam, ErrorInfo& errInfo, int runType, BaseCost& baseCost, BagCost& bagCost, BaseStat& baseStat, BagStat& bagStat, Json::Value& jLog);
};

class ErrDump {
public:
	static int Dump(const QueryParam& qParam, ErrorInfo& errInfo, Json::Value& req, Json::Value& resp);
};

class RealTrafStat {
public:
	static int DoStat(BasePlan* basePlan);
};

#endif
