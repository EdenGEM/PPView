#ifndef __DAYSPLAN__H_
#define __DAYSPLAN__H_

#include "PathGenerator.h"
#include "Route/base/define.h"
#include "Route/base/BasePlan.h"
class DayRoute;
class DaysPlan {
	public:
		DaysPlan() {
		}
		~DaysPlan() {
			for (int i = 0; i < m_dayPlan.size(); i++) {
				delete m_dayPlan[i];
				m_dayPlan[i] = NULL;
			}
			m_dayPlan.clear();
		}

		bool GetPlanResp(BasePlan* basePlan, const Json::Value& req, Json::Value& resp);

	private:
		std::vector<DayRoute *> m_dayPlan;
		std::map<std::string, Json::Value> m_date2pois;
		std::map<std::string, std::pair<int, int>> m_date2playRange;
		std::string m_cityID;
		
		//构造输入
		bool InitDayPlan (BasePlan* basePlan, const Json::Value& req);
		bool InitRouteJ (BasePlan* basePlan);
		//构造输出
		bool MakeResp (BasePlan* basePlan, const Json::Value& req, Json::Value& resp);
		bool MakejSummary (BasePlan* basePlan, const Json::Value& req, const Json::Value& jWarningList, Json::Value& jDayList, Json::Value& jSummary);
		bool ChangeConflict2TourErr(const Json::Value& req, const Json::Value& jWarningList, Json::Value& resp);
		//由于酒店特殊处理，须对早上离开的酒店stime etime dur做处理
		bool FixMorningDeptHotel (Json::Value& jDeptHotel, const Json::Value& jLastNightHotel);
		//warning 按天排序
		bool PerfectWarningList (Json::Value& jWarningList);
		
		const LYPlace* GetHotelByDidx (BasePlan* basePlan, int didx);
		int hasExpire(int didx);
};

class DayRoute {
	public:
		DayRoute(){}
		DayRoute(int didx, std::string date, Json::Value routeJ, int dayMax, Json::Value controls):m_didx(didx),m_date(date),m_inRouteJ(routeJ),m_dayMax(dayMax),m_controls(controls){
			m_expire = 0;
		}
		bool GetDayView (BasePlan* basePlan, Json::Value& jError, Json::Value& jDay, Json::Value& jWarningList);
		int hasExpire();
		bool Show(int timeZone);

	private:
		int m_didx;
		int m_expire;
		std::string m_date;
		Json::Value m_inRouteJ;
		Json::Value m_outRouteJ;
		Json::Value m_controls;
		int m_dayMax;

		//pathGennerator 扩展天
		bool dealDayPois();
		//routeJ转为输出格式view.day
		bool routeJ2ViewDay (BasePlan* basePlan, Json::Value& jError, Json::Value& jDay, Json::Value& jWarningList);
		bool getErrorAndWarnings(BasePlan* basePlan, Json::Value& jError, Json::Value& jWarningList);
		bool SetProduct(BasePlan* basePlan,const LYPlace* place, Json::Value& jView);
		std::string getPlaceName(BasePlan* basePlan, int index);
		bool makeJView (BasePlan* basePlan, const Json::Value& nodeJ, Json::Value& jView);
		bool makeJViewList(BasePlan* basePlan, Json::Value& jViewList);
};

#endif
