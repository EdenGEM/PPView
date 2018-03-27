#ifndef __LIGHTPLAN_H__
#define __LIGHTPLAN_H__

#include "base/define.h"
#include "base/Utils.h"
#include "MJCommon.h"
#include "base/ReqParser.h"
#include "base/ReqChecker.h"
#include "base/LYConstData.h"
#include "base/PathTraffic.h"
#include "bag/MyThreadPool.h"
#include "bag/ThreadMemPool.h"
#include "PostProcessor.h"
#include <iostream>

const int   CHANGE_NULL	= 0x00000000;
const int   CHANGE_HOTEL_NUM = 0x00000001;	//酒店数目变化
const int   CHANGE_HOTEL_NULL = 0x00000002;	//删除酒店
const int   CHANGE_HOTEL_REPLACE = 0x00000004;	//酒店顺序变化或改酒店
const int   CHANGE_HOTEL_DATE = 0x00000008;	//酒店日期改变
const int   CHANGE_HOTEL_COREHOTEL_ID = 0x00000010; //需要改变市中心酒店id  //兼容老数据 假酒店 返回 真实酒店id
const int   CHANGE_HOTEL = CHANGE_HOTEL_NUM | CHANGE_HOTEL_NULL | CHANGE_HOTEL_REPLACE |  CHANGE_HOTEL_DATE | CHANGE_HOTEL_COREHOTEL_ID;
const int   CHANGE_ADD_DAYS = 0x00000200;
const int   CHANGE_NEW_CITY = 0x00002000;
const int	CHANGE_NEED_MOVE_ROUTE = 0x00004000;	//行程需平移
const int	CHANGE_HAS_CAR_STORE = 0x00008000;	//有租车点就要走ssv006以获取交通

struct DayCmp {
	bool operator() (const std::pair<std::string, int>& l, const std::pair<std::string, int>& r) {
		if (l.second != 0 && r.second != 0) {
			return l.first < r.first;
		} else if (l.second == 0 && r.second == 0){
			return l.first < r.first;
		} else {
			return l.second > r.second;
		}
	}
};

class PoiInfo {
public:
	PoiInfo(std::string id = "", std::string name = "", std::string lname = "", std::string arvTime = "", std::string deptTime = "",
			std::string coord = "", std::string pstime = "", std::string petime = "", int custom = 0, int dur = -1, int type = LY_PLACE_TYPE_NULL, Json::Value product=Json::Value()) :
		m_id(id), m_name(name), m_lname(lname), m_arvTime(arvTime), m_deptTime(deptTime), m_coord(coord), m_pstime(pstime),m_petime(petime), m_custom(custom), m_dur(dur), m_type(type), m_product(product){
	}
public:
	std::string m_id;
	std::string m_name;
	std::string m_lname;
	std::string m_arvTime;
	std::string m_deptTime;
	std::string m_coord;
	std::string m_pstime;
	std::string m_petime;
	//点归属的日期，主要针对凌晨晚的点
	std::string m_belongDate;
	int m_custom;
	int m_dur;
	int m_type;
	Json::Value m_product;
	std::string m_play;

	bool operator < (const PoiInfo& rpoi) const {
		return this->m_name < rpoi.m_name;
	}
	bool operator == (const PoiInfo& rpoi) const {
		return (this->m_id == rpoi.m_id) &&
			(this->m_name == rpoi.m_name) &&
			(this->m_arvTime == rpoi.m_arvTime) &&
			(this->m_deptTime == rpoi.m_deptTime) &&
			(this->m_coord == rpoi.m_coord) &&
			(this->m_pstime == rpoi.m_pstime) &&
			(this->m_petime == rpoi.m_petime) &&
			(this->m_custom == rpoi.m_custom) &&
			(this->m_type == rpoi.m_type);
	}
};

struct CityInfo {
	//请求相关
	std::string m_qCheckInDate;
	std::string m_qCheckOutDate;
	PoiInfo m_qArvPoiInfo;
	PoiInfo m_qDeptPoiInfo;
	Json::Value m_qTrafficIn;
	std::vector<HInfo> m_qHInfoList;
	std::vector<std::string> m_qDateList;
	//旧响应相关
	std::string m_rCheckInDate;
	std::string m_rCheckOutDate;
	PoiInfo m_rArvPoiInfo;
	PoiInfo m_rDeptPoiInfo;
	Json::Value m_rTrafficIn;
	std::vector<HInfo> m_rHInfoList;
	std::string m_rHotelId; //记录原行程酒店id仅原行程酒店为null时 有用
	std::vector<std::pair<std::string, int> > m_dayVCntList;	//每天的景点数
	double m_timeZone;
	//其他
	int m_changeType;	//更改类型
	std::string m_cid; //城市id
	std::tr1::unordered_set<std::string> m_addDateSet;	//新行程较旧行程增加的天
	std::tr1::unordered_set<std::string> m_needPlanDateSet;	//需要规划的天
//	std::tr1::unordered_map<std::string, int> m_rDateSeqMap;	//旧行程各天对应于新行程的第几天
	std::tr1::unordered_map<std::string, std::string> m_reqDate2oriDate;  //新旧行程的日期对应关系
	int m_ridx;//在route中的序号
	int m_translate;	//新旧行程偏移量 old-new (用老行程日期计算新日期时,使用负值)
};

class LightPlan {
public:
	int PlanS126(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error);
private:
/*获取行程信息*/

	//获取新的请求和之前的响应信息
	int GetReqRespInfo(const QueryParam& param, Json::Value& req, ErrorInfo& error);

/*检测城市间的改变*/

	//检查城市间的改变
	int CheckChanges(const QueryParam& param, Json::Value& req, MyThreadPool* threadPool);
	//获取酒店的改变类型
	int GetHotelChangeType(const QueryParam& param, int cIdx, const CityInfo& cityInfo, int& changeType);
	//获取城市间的交通、时间等的改变类型
	int GetCityTrafTimeChangeType(const QueryParam& param, Json::Value& req, int cIdx, const CityInfo& cityInfo, int& changeType);
	//判断是否为新增城市
	bool IsNewCity(const QueryParam& param, const Json::Value& req, int cIdx);
	//判断手否有租车门店
	bool HasCarStore(const Json::Value & req,int cIdx);


/*规划或改变行程*/

	//处理单个城市
	int PlanCity(const QueryParam& param, Json::Value& req, Json::Value& jCityResp, MyThreadPool* threadPool,  ErrorInfo& error, int cIdx, CityInfo& cityInfo);
	//对于新城市，够造请求并规划
	int PlanRoute(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error, int cIdx, int changeType);
	//已有行程，刷新行程
	int RefreshRoute(const QueryParam& param, Json::Value& req, Json::Value& resp, ErrorInfo& error, int cIdx, CityInfo& cityInfo);
	//根据product改变view;
	int ChangeViewAboutCarStore(Json::Value& req);


/*构造请求相关*/

	//获取请求包含的日期
	int GetReqDateList(CityInfo& cityInfo);
	//根据checkIn获取酒店
	int getNewHotelId(const CityInfo& cityInfo, std::string checkInDate, std::string& hotelId);
	//构造jDays
	int MakeJDays(Json::Value& jReq, CityInfo& cityInfo);
	//构造ssv005_rich请求
	int MakeReqSSV005_rich(const Json::Value& req, Json::Value& jReq, QueryParam& queryParam, int cIdx);
	//构造ssv006请求
	int MakeReqSSV006(const Json::Value& req, Json::Value& jReq, QueryParam& queryParam, int cIdx, CityInfo& cityInfo);


/*对新行程或空白天做规划*/

	//对增加的天或增加的城市进行按城市分组规划
	int DoGroupPlan(const QueryParam& param, Json::Value& req, Json::Value& resp, MyThreadPool* threadPool,  ErrorInfo& error);
public:
	std::tr1::unordered_map<std::string,std::tr1::unordered_map<std::string,Json::Value> > m_missCarStores;//存储解析到的租车数据
	Json::Value m_preferTimeRange;
private:
	std::map<int,int> m_cityDaysTranslate;  //旧日期到新日期的偏移 old-new
	std::tr1::unordered_set<int> m_changeCIdxSet;
	std::tr1::unordered_map<int, CityInfo> m_cityInfoMap;
	Json::Value m_plannedPois; //团游中已规划的点集
};

inline bool LightPlan::IsNewCity(const QueryParam& param, const Json::Value& req, int cIdx) {
	if (cIdx < req["list"].size() && req["list"][cIdx]["originView"].isNull()) {
		MJ::PrintInfo::PrintDbg("[%s]LightPlan::IsNewCity, cIdx %d is a new city, ", param.log.c_str(), cIdx);
		return true;
	}
	return false;
}

inline bool LightPlan::HasCarStore(const Json::Value & req,int cIdx)
{
	if(cIdx < req["list"].size()
			and req["list"][cIdx]["originView"].isObject()
			and req["list"][cIdx]["originView"]["day"].isArray()
	  )
	{
		const Json::Value & day = req["list"][cIdx]["originView"]["day"];
		for(int i=0; i< day.size(); i++)
		{
			const Json::Value & oneDayView = day[i]["view"];
			if(oneDayView.isArray())
			{
				for(int j=0; j< oneDayView.size(); j++)
				{
					if(oneDayView[j]["type"].isInt() and oneDayView[j]["type"].asInt() & LY_PLACE_TYPE_CAR_STORE)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

#endif
