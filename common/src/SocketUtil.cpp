#include "SocketClient.h"
#include "MJLog.h"
#include "SocketUtil.h"


using namespace MJ;
// http请求，返回值
// 0: 正常返回
// -1: 超时
// 1: 结果异常
int SocketUtil::GetHttpData(std::string& Addr, int TimeOut, std::string& Query, nlohmann::json& Ret, int& TimeCost) {
	SocketClient cnn;
	ServerRst ret;

    cnn.init(Addr, TimeOut);
    cnn.getRstFromHost(Query, ret, 2);

	PrintInfo::PrintDbg("SocketUtil::GetHttpData, HTTP Time Cost[%s]: %dus", Query.c_str(), TimeCost);

	if (ret.ret_str.length() == 0){
		PrintInfo::PrintErr("SocketUtil::GetHttpData, req:[%s], no return data!", Query.c_str());
		return -1;
	}
	if (ret.ret_str.size() >= 4 && ret.ret_str.substr(0, 4) == "null") {
		PrintInfo::PrintErr("SocketUtil::GetHttpData, req:[%s], return null!", Query.c_str());
		return 1;
	}
	Ret = nlohmann::json::parse(ret.ret_str);

	PrintInfo::PrintLog("SocketUtil::GetHttpData, Query:%s\nRData:%s", Query.c_str(), ret.ret_str.c_str());
	return 0;
}
