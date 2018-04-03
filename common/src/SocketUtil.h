#ifndef __SOCKET_UTIL_H__
#define __SOCKET_UTIL_H__

#include <iostream>
#include <string>
#include "json/json.hpp"

namespace MJ
{

class SocketUtil {
public:
	// Http请求获取信息
	static int GetHttpData(std::string& Addr, int TimeOut, std::string& Query, nlohmann::json& Ret, int& TimeCost);
};

}
#endif
