#ifndef _REQ_CHECKER_H_
#define _REQ_CHECKER_H_

#include <iostream>
#include "Route/base/Utils.h"
#include "MJCommon.h"
#include "LogDump.h"

enum JSON_TYPE {
	JSON_TYPE_NULL,
	JSON_TYPE_OBJECT,
	JSON_TYPE_ARRAY,
	JSON_TYPE_INT,
	JSON_TYPE_STRING,
	JSON_TYPE_DOUBLE
};

enum FORMAT_TYPE {
	STRING_TYPE_TIME_YMD,
	STRING_TYPE_TIME_YMD_HM,
	STRING_TYPE_TIME_HM,
	STRING_TYPE_COORD,
	STRING_TYPE_ID
};

class ReqChecker {
public:
	static int DoCheck(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
private:
	static int CheckBase(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckSSV005(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckSSV006(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckS126(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckPoiList(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckP105(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckB116(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckS128(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckS130(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckS131(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckTour(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckSSV007(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	//smz
	static int CheckCityInfo(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckProductInfo(const QueryParam& param, Json::Value& jProduct, ErrorInfo& errorInfo);
	static int CheckDays(const QueryParam& param, Json::Value& jDays, ErrorInfo& errorInfo);
	static int CheckNotPlanDays(const QueryParam& param, Json::Value& jDays, ErrorInfo& errorInfo);
	static int CheckCostomTraffic(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	//smz_end
	static int CheckProduct(const QueryParam& param, Json::Value& jProduct, ErrorInfo errorInfo);
	static int CheckFilterP104(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckCity(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckRoute(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckSelectPois(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckCityPrefer(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckFilter(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);
	static int CheckTrafficPass(const QueryParam& param, Json::Value& req, ErrorInfo& errorInfo);   // check 途经点
public:
	static int CheckTypeSoft(const QueryParam& param, Json::Value& req, const std::string& memName, int jType, const std::string& errInfo, ErrorInfo& errorInfo);
	static int CheckType(const QueryParam& param, Json::Value& req, const std::string& memName, int jType, const std::string& errInfo, ErrorInfo& errorInfo);
	static int CheckType(const QueryParam& param, Json::Value& jValue, int jType, const std::string& errInfo, ErrorInfo& errorInfo);
	static int CheckStringFormat(const QueryParam& param, const std::string& str, int sType, const std::string& errInfo, ErrorInfo& errorInfo);
	static int CheckId(const std::string& str);
};
#endif

