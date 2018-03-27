#ifndef __POST_PROCESSOR_H__
#define __POST_PROCESSOR_H__

#include "Route/base/BasePlan.h"
#include "Route/base/PathTraffic.h"
#include "Route/base/define.h"

class PostProcessor {
public:
	static int PostProcess(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	//置S122中休息、出发游玩、退房的dur为-1
	static int SetDurNegative1ForCList(Json::Value& resp);
	static bool IsSleepCost(BasePlan* basePlan, Json::Value& jView);
	//玩乐product 输出
	static int MakeOutputProduct(Json::Value& req, Json::Value& resp);
	static int AddDoWhat(Json::Value& jDayList, BasePlan* basePlan);
	//增加交通报错
	static int AddTrafficWarning(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList);
	//增加高级编辑报错
	static int AddErrors(BasePlan* basePlan, Json::Value& jData);
	//根据view 生成 poi list
	static int MakejDays (const Json::Value& req, const Json::Value& jViewDayList, Json::Value& jSummaryDaysList);
	static int MakeJSummary(BasePlan* basePlan, const Json::Value& req, const Json::Value& jWarningList,  Json::Value& jDayList, Json::Value& jSummary);
	//根据viewList 生成day.stime etime
	static int SetDayPlayRange(Json::Value& jDay, std::string sTime = "", std::string eTime = "");
	//生成view中玩乐的product
	static int SetProduct(BasePlan* basePlan, const LYPlace* place, Json::Value& jView);
	static Json::Value AddResult(const Json::Value jBaseDayList, const Json::Value jDayList, std::tr1::unordered_set<std::string> DateSet);//拼结果
	static bool FixHotelTime (Json::Value& jDeptHotel, Json::Value& jDeptHotelPoi, Json::Value& jLastNightHotel, Json::Value& jLastNightHotelPoi);//修改入住和第二天离开酒店的时间
private:
	static int MakeOutputP104(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeOutputP105(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeOutputB116(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeOutputSSV005(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeOutputP202(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeOutputP201(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	//智能优化预判
	static int MakeOutputS129(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
    // 途经点平移行程
    static int MakeOutputS126(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int MakeTrafficJDetail(BasePlan* basePlan, const TrafficDetail* tDetail, Json::Value& jItem);
	static int MakeJView(const PlanItem* item, const PlanItem* nextItem, BasePlan* basePlan, Json::Value& jView, Json::Value& jWarningList, int dIdx, int vIdx);
	static int MakeJDay(BasePlan* basePlan, Json::Value& jViewList, Json::Value& jDay, bool isLastDay, bool isFirstDay);
	static int MakeJDayList(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList);
	static int PerfectHotelDoWhat(Json::Value& jDayList, BasePlan* basePlan);

	static std::string GetDate(Json::Value& jViewList, bool isLastDay = false, bool isFirstDay = false);
	static int MakeMissPoi(BasePlan* basePlan, Json::Value& jMissPoiList);

	static int MakeServInfo(BasePlan* basePlan, Json::Value& servInfo);

	// arvPoi和deptPoi为空时，去除行程首尾的作为arvPoi和deptPoi的Hotel
	static int RemoveArvDeptHotel(BasePlan* basePlan, Json::Value& jDayList, Json::Value& jWarningList);

	static int SetDurNegative1(Json::Value& jDays);
	static int MergeDay1Hotel(BasePlan* basePlan, Json::Value& jViewList, Json::Value& jSecondDay);
	//删除必去点的报错
	static int DelUseMustPoi(BasePlan* basePlan, Json::Value& jWarningList);
	static int MakeP104Node(const LYPlace *place, Json::Value &jPlaceInfo);
	//s130
	static int MakeOutputS130(Json::Value& req, BasePlan* basePlan, Json::Value& resp);
	static int AddDiffAfterOption(Json::Value& req, BasePlan* basePlan, Json::Value& jData);
	static int GetAllItemFromView(Json::Value& jView, Json::Value& jReqView, Json::Value& closePois);
	static int InsertToPoisSet(std::string id, std::tr1::unordered_map<std::string, int>& placeNumMap, std::tr1::unordered_set<std::string>& poiIdSet);
	//product
	static int MakeProduct(Json::Value& req, Json::Value& jView, Json::Value& jWanle, int ridx, int inCity);
	static int DelOriProduct(Json::Value& jWanle, int ridx, int inCity);
	//判断是否为锁定点
	static bool IsLockTimePoi(BasePlan* basePlan, const Json::Value& jView,int index);
};

#endif
