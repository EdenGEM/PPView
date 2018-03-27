#ifndef _NODEJ_H_
#define _NODEJ_H_

#include <string>
#include <json/json.h>
#include "../base/define.h"

//NodeJ这个类名暂时只起到命名空间的效果
class NodeJ{
	public:
		//检查除arrange之外字段的合法性,关于每个字段的检查范围参照docs;
		//从下面接口定义中知道,"toNext"这个key一定存在
		static bool checkNodeJ(const Json::Value& nodeJ);
		//循环检查每个nodeJ
		static bool checkRouteJ(const Json::Value& routeJ);
		//设置非空的id名,否则返回false
		static bool setId(Json::Value& nodeJ,std::string id);
		//设置点的出入位置;检查出入口的经纬度格式
		static bool setLocations(Json::Value& nodeJ,std::string inLoc,std::string outLoc);
		//此函数用于添加后向的交通,交通时长要大于等于0
		static bool addTraffic(Json::Value& nodeJ,std::string id,int seconds);
		//将"toNext"的值置为null
		static bool setEndFlag(Json::Value& nodeJ);
		//判断free字段是否存在
		static bool isFree(const Json::Value& nodeJ);
		//设置游玩时长
		static bool setDurs(Json::Value& nodeJ,int min,int rcmd,int max);
		//增加开关门
		static bool addOpenClose(Json::Value& nodeJ,int open,int close);
		//判断fixed字段是否存在
		static bool isFixed(const Json::Value& nodeJ);
		//增加场次
		static bool addTime(Json::Value& nodeJ,int left,int right);
		//清空场次信息
		static bool delTimes(Json::Value& nodeJ);
		//设置fixed是否可删
		static bool setFixedCanDel(Json::Value& nodeJ,int canDel);
		//判断是否可删除
		static bool isNodeCanDel(const Json::Value& nodeJ);
		//判断是否被删除
		static bool isDeleted(const Json::Value& nodeJ);
		//删除arrange
		static bool reset(Json::Value& nodeJ);
		//判断添加该点时是否出错
		static bool hasError(const Json::Value& nodeJ);
		//判断是否出现客观错误
		static bool hasFixedConflictError(const Json::Value& nodeJ);
		//设置error标志
		static bool setError(Json::Value& nodeJ,int type);
		//设置默认错误码
		static bool setDefaultError(Json::Value& nodeJ);
		//设置安排结果
		static bool setPlayRange(Json::Value& nodeJ,int left,int right);
		//获取游玩时长
		static bool getPlayRange(const Json::Value& nodeJ);
		//设置当前遍历所使用的场次或开关门idx
		static bool setRangeIdx(Json::Value& nodeJ,int idx);
		//判断是否和右边界冲突
        static bool isRightConflict(const Json::Value& nodeJ);
		//判断是否可以在锁定时刻/或之前到达
		static bool isFixedArvOnTime(const Json::Value& nodeJ);
};
#endif
