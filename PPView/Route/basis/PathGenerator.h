#ifndef __PATHGENERATOR__H_
#define __PATHGENERATOR__H_

#include <string>
#include "NodeJ.h"
#include "json/json.h"
#include "Route/base/BagParam.h"

class PathGenerator{
	//设置天的左右边界
	public:
		//crotrols 中包含是否允许客观冲突删点等标志位,允许从外部控制路线生成的表现;比如指定优化顺序等
		PathGenerator(const Json::Value& routeJ, const Json::Value& crotrols = Json::nullValue);
		PathGenerator(){};
		bool DayPathExpand();
		bool DayPathExpandOpt();
		bool DayPathSearch();
		bool TimeEnrich();
		Json::Value GetResult() {
			return m_outRouteJ;
		}
		//选取合适的open close 保证开关门尽可能多
        bool selectOpenClose(Json::Value& routeJ);
        bool m_isSearch;
	private:
		std::pair<int,int> m_playBorders;
		Json::Value m_controls;
		Json::Value m_inRouteJ;
		Json::Value m_outRouteJ;

		//选取fixed点合适的场次
		bool selectFixedPoiTimes();
		bool setFixedPoiTimes(Json::Value& nodeJ);
		//选取合适的open close 保证开关门尽可能多
        bool isOpen(Json::Value nodeJ,int pos);
		std::bitset<32> countOpenClose(Json::Value routeJ);
        Json::Value selectRouteJ(std::vector<Json::Value> ResultRouteJ);
        int ErrorCount(Json::Value routeJ);


		//在点append之前，判断会出现的error
		bool setNodeError(Json::Value& nodeJ);
		//初始化左右边界
		bool initPlayBorders();
		//判断天的边界本身是否有冲突
		bool isBordersConflict();
		//判断fixed客观冲突，并置error
		bool isBackFixedConflict(Json::Value& nodeJ);
		//判断是否与右边界冲突
		bool isConflictWithBorders(const Json::Value& nodeJ);

		bool append(const Json::Value& nodeJ);
		Json::Value popBack();

	//search 相关
	private:
		MJ::MyTimer m_searchTime;
		MJ::MyTimer m_screeningsSearchTime; //分场次搜索
		int m_sCnt;
		Json::Value m_poiList;
		bool m_IdxVisited[BagParam::m_dayNodeLimit][BagParam::m_dayNodeLimit]; //当前层已选点
		bool m_visited[BagParam::m_dayNodeLimit]; //当前路线已选点
		int m_fixedNum;
		int m_freeNum;
		int m_trafTime;
		std::multimap <float, Json::Value> m_pathList;  //score - path
	private:
		bool GetNextNodeByGreedy(const Json::Value& nodeJ, int idx, int& nextIdx);
		bool GetIdxByIdstr(std::string id, int& index);
		bool InitSearch();
		bool Search();
		bool DoSearch(int depth);
		bool IsTimeOut();
		bool IsScreeningsTimeOut();
		bool SelectBestPath();
};
#endif
