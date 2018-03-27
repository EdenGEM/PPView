#include <string>
#include "NodeJ.h"
#include "stdio.h"
#include "stdlib.h"
#include <iostream>
#include "../base/ToolFunc.h"
using namespace std;

//检查除arrange之外字段的合法性,关于每个字段的检查范围参照docs;
//从下面接口定义中知道,"toNext"这个key一定存在
bool NodeJ::checkNodeJ(const Json::Value& nodeJ)
{
    bool ret=true;
    if(!(nodeJ.isMember("id")&&nodeJ["id"].isString()))
        return false;
    if(nodeJ.isMember("locations")){
        if(!(nodeJ["locations"].isMember("in")&&nodeJ["locations"].isMember("out"))) {
			_INFO("id: %s, locations has no in/out", nodeJ["id"].asString().c_str());
           return false;
		}
    }
    if(!nodeJ.isMember("toNext")) {
		_INFO("id:%s, no member toNext", nodeJ["id"].asString().c_str());
        return false;
	}
    if(!(isFree(nodeJ)^isFixed(nodeJ))) {
		_INFO("id:%s, no member free/fixed", nodeJ["id"].asString().c_str());
        return false;
	}
    if(nodeJ.isMember("free")){
        if(!nodeJ["free"].isMember("openClose") || !nodeJ["free"]["openClose"].isArray()) {
			_INFO("id:%s, openclose is Err", nodeJ["id"].asString().c_str());
            return false;
		}
        if(!(nodeJ["free"].isMember("durs")&&nodeJ["free"]["durs"].isArray())) {
			_INFO("id:%s, durs is Err", nodeJ["id"].asString().c_str());
            return false;
		}
    }
    else if(nodeJ.isMember("fixed")){
        if(!nodeJ["fixed"].isMember("times") || !nodeJ["fixed"]["times"].isArray() || nodeJ["fixed"]["times"].size() <= 0 || !nodeJ["fixed"]["times"][0u].isArray()) {
			_INFO("id:%s, fixed times is Err", nodeJ["id"].asString().c_str());
            return false;
		}
        if(!(nodeJ["fixed"].isMember("canDel")&&nodeJ["fixed"]["canDel"].isInt())) {
			_INFO("id:%s, fixed canDel is Err", nodeJ["id"].asString().c_str());
            return false;
		}
    }
    return ret;
}

//循环检查每个nodeJ
bool NodeJ::checkRouteJ(const Json::Value& routeJ)
{
    bool ret=true;
	int len=routeJ.size();
	if (len < 2) {
		_INFO("routeJ size < 2");
		return false;
	}
    for(int i=0;i<len;i++)
    {
        ret=checkNodeJ(routeJ[i]);
        if (!ret) return false;
    }
    return true;
}
//设置非空的id名,否则返回false
bool NodeJ::setId(Json::Value& nodeJ,std::string id){
    if (id!="")
    {
        nodeJ["id"]=Json::Value(id);
        return true;
    }
    return false;
}
//设置点的出入位置;检查出入口的经纬度格式
bool NodeJ::setLocations(Json::Value& nodeJ,std::string inLoc,std::string outLoc){
    int flagi=0,flago=0;
    Json::Value temp;
    std::vector<std::string> output;
	ToolFunc::sepString(inLoc,",",output);
	if (output.size() != 2) return false;
    float Ilit=atof(output[0].c_str());
    float Ialt=atof(output[1].c_str());
    output.clear();
	ToolFunc::sepString(inLoc,",",output);
	if (output.size() != 2) return false;
    float Olit=atof(output[0].c_str());
    float Oalt=atof(output[1].c_str());
    if(Ilit>=0&&Ilit<=180&&Ialt>=0&&Ialt<=90)
        flagi=1;
    if(Olit>=0&&Olit<=180&&Oalt>=0&&Oalt<=90)
        flago=1;
    if(flagi==1&&flago==1)
    {
        temp["in"]=Json::Value(inLoc);
        temp["out"]=Json::Value(outLoc);
        nodeJ["locations"]=temp;
        return true;
    }
    return false;
}
//此函数用于添加后向的交通,交通时长要大于等于0
bool NodeJ::addTraffic(Json::Value& nodeJ,std::string id,int seconds){
    if(seconds>=0&&id!="")
    {
        nodeJ["toNext"][id]=Json::Value(seconds);
        return true;
    }
    return false;
}
//将"toNext"的值置为null
bool NodeJ::setEndFlag(Json::Value& nodeJ){
    nodeJ["toNext"]=Json::Value();
    return true;
}

//判断free字段是否存在
bool NodeJ::isFree(const Json::Value& nodeJ){
    if (nodeJ.isMember("free"))
        return true;
    else
        return false;

}
//设置游玩时长
bool NodeJ::setDurs(Json::Value& nodeJ,int min,int rcmd,int max){
    if (min<=rcmd&&rcmd<=max){
        Json::Value temp;
        temp["durs"].append(min);
        temp["durs"].append(rcmd);
        temp["durs"].append(max);
        nodeJ["free"]["durs"]=temp["durs"];
        return true;
    }
    else
        return false;
}
//增加开关门
bool NodeJ::addOpenClose(Json::Value& nodeJ,int open,int close){
	if (open == close) {
		nodeJ["free"]["openClose"] = Json::arrayValue;
		return true;
	}
	Json::Value temp;
	temp["openClose"].append(open);
	temp["openClose"].append(close);
	nodeJ["free"]["openClose"].append(temp["openClose"]);
	return true;
}
//判断fixed字段是否存在
bool NodeJ::isFixed(const Json::Value& nodeJ){
    if(nodeJ.isMember("fixed"))
        return true;
    else
        return false;
}

//增加场次
bool NodeJ::addTime(Json::Value& nodeJ,int left,int right){
    if(left<=right){
        Json::Value times;
        times.append(left);
        times.append(right);
        nodeJ["fixed"]["times"].append(times);
        return true;
    }
    return false;
}
//清空场次
bool NodeJ::delTimes(Json::Value& nodeJ) {
	if (nodeJ.isMember("fixed")) nodeJ["fixed"]["times"] = Json::arrayValue;
	return true;
}

//设置fixed是否可删
bool NodeJ::setFixedCanDel(Json::Value& nodeJ,int canDel){
    nodeJ["fixed"]["canDel"]=Json::Value(canDel);
    return true;
}

//判断是否可删除
bool NodeJ::isNodeCanDel(const Json::Value& nodeJ) {
    if (isFree(nodeJ)) return true;
	if (nodeJ["fixed"].isMember("canDel") && nodeJ["fixed"]["canDel"].isInt() && nodeJ["fixed"]["canDel"].asInt()) {
		return true;
	}
	return false;
}

//判断是否被删除
bool NodeJ::isDeleted(const Json::Value& nodeJ){
    if (nodeJ["arrange"]["time"][0]==-1&&nodeJ["arrange"]["time"][1]==-1)
        return true;
    else
        return false;
}

//判断是否按时到达
bool NodeJ::isFixedArvOnTime(const Json::Value& nodeJ) {
	if(nodeJ["arrange"]["error"].isArray() && nodeJ["arrange"]["error"][5].asInt()) return false;
	return true;
}

//删除arrange
bool NodeJ::reset(Json::Value& nodeJ){
    nodeJ["arrange"]["time"].append(-1);
    nodeJ["arrange"]["time"].append(-1);

}
//判断添加该点时是否出错
bool NodeJ::hasError(const Json::Value& nodeJ){
    for(int i=0;i<7;i++){
        if(nodeJ["arrange"]["error"][i].asInt())
            return true;
    }
    return false;
}
bool NodeJ::hasFixedConflictError(const Json::Value& nodeJ){
    for(int i=0;i<=2;i++){
        if(nodeJ["arrange"]["error"][i].asInt())
            return true;
    }
    return false;
}
bool NodeJ::setDefaultError(Json::Value& nodeJ){
	nodeJ["arrange"]["error"] = Json::arrayValue;
    for(int i=0;i<7;i++)
        nodeJ["arrange"]["error"].append(0);
    return true;
}
//设置error标志
bool NodeJ::setError(Json::Value& nodeJ,int type){
    if(type>=0&&type<7){
        nodeJ["arrange"]["error"][type]=1;
        return true;
    }
    return false;
}
//设置安排结果
bool NodeJ::setPlayRange(Json::Value& nodeJ,int left,int right){
    Json::Value temp;
    temp["time"].append(left);
    temp["time"].append(right);
    nodeJ["arrange"]["time"]=temp["time"];
    return true;
}
//设置当前遍历所使用的场次或开关门idx
bool NodeJ::setRangeIdx(Json::Value& nodeJ,int idx){
    nodeJ["arrange"]["controls"]["rangeIdx"]=Json::Value(idx);
    return true;
}

bool NodeJ::isRightConflict(const Json::Value& nodeJ){
    if(nodeJ.isMember("arrange") && nodeJ["arrange"].isMember("error") && nodeJ["arrange"]["error"][6].asInt()) return true;
    return false;
}
