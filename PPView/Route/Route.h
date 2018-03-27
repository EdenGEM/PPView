#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <iostream>
#include <pthread.h>
#include <tr1/unordered_map>
#include "Route/base/LYConstData.h"
#include "Route/base/Utils.h"
#include "MJCommon.h"
#include "Route/bag/MyThreadPool.h"

class Route {
public:
	Route() {
		_threadPool = NULL;
		Init();
	}
	~Route() {
		Release();
	}
public:
	int DoRoute(const QueryParam& param, Json::Value& req, Json::Value& resp);
	int AvailDurLevel(const QueryParam& param, Json::Value& req);
	static int LoadStatic(const std::string& data_path, const std::string& conf_path);
	static int ReleaseStatic();
private:
	int Init();
	int Release();
public:
	MyThreadPool* _threadPool;
};

#endif
