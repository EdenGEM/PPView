#ifndef _BASE_PLAN_H_
#define _BASE_PLAN_H_

#include <tr1/unordered_map>
#include <string>
#include <vector>
#include "define.h"
#include "LYConstData.h"
#include "PathView.h"
#include "PlaceGroup.h"
#include "Utils.h"
#include "Prefer.h"
#include "TrafficData.h"
#include "LogDump.h"
#include "BagParam.h"
#include "ToolFunc.h"

enum PLACE_PUSH_TYPE {
	PLACE_PUSH_NULL,
	PLACE_PUSH_COMP,
	PLACE_PUSH_MANU
};

enum RUN_TYPE {
	RUN_NULL,
	RUN_PROCESSOR,
	RUN_PLANNER_BAG,
	RUN_PLANNER_ACO
};

class BasePlan {
	public:
		BasePlan();
		virtual ~BasePlan() {
			Release();
		}
	private:
		int Release();

	//-------函数相关----------
	public:
	// 整体行程相关
		std::vector<const LYPlace*>& GetPlace();
		TimeBlock* GetBlock(int index) const;
		KeyNode* GetKey(int index) const;
		std::tr1::unordered_map<std::string, int>& GetCrossMap();

		int GetBlockNum() const;
		int ClearBlock();
		int GetKeyNum() const;
		int PushBlock(time_t start, time_t stop, const std::string& trafDate, int avail_dur, uint8_t restNeed, double time_zone);

	public:
	// 读写相关
		const TrafficItem* GetTraffic(const std::string& ida, const std::string& idb);
		const TrafficItem* GetTraffic(const std::string& ida, const std::string& idb, const std::string& date);
		const TrafficItem* GetCustomTraffic(const std::string& trafKey);
		int GetAvgTrafTime(const std::string& id);
		int GetAvgTrafDist(const std::string& id);
		int GetAvgAllocDur(const LYPlace* place);
		int GetMinAllocDur(const LYPlace* place);
		int GetAllocDur(const LYPlace* place);

		const DurS GetDurS(const LYPlace* place);
		const DurS* GetDurSP(const LYPlace* place);
		int GetRcmdDur(const LYPlace* place);
		int GetMinDur(const LYPlace* place);
		int GetMaxDur(const LYPlace* place);
		int GetZipDur(const LYPlace* place);
		int GetExtendDur(const LYPlace* place);
		int SetDurS(const LYPlace* place, const DurS* durS);
		int SetDurS(const LYPlace* place, const DurS durS);
		int SetDurS(const LYPlace* place, int min, int zip, int rcmd, int extend, int max);
		int GetHot(const LYPlace* place);

		int GetMissLevel(const LYPlace* place);
		const HInfo* GetHotelByDidx(int didx);
		const LYPlace* GetLYPlace(const std::string& id);//用id 获取相应点信息
		std::pair<time_t, time_t> GetDayRange(const std::string& date);
		//KeyNodeBuild.cpp
		const int GetEntryTime(const LYPlace* place);
		const int GetExitTime(const LYPlace* place);
		const int GetEntryZipTime(const LYPlace* place);
		const int GetExitZipTime(const LYPlace* place);
		static std::string GetCutId(const std::string& id);//多次游玩的点和自制coreHotel 需要cut id后使用
		std::string Insert2MultiMap(const std::string& id);//添加多次游玩的点
		std::string GetRealID(const std::string& id); //Eden change :可删，因为return 的就是id

		int GetProdTicketsListByPlace(const LYPlace* place, std::vector<const TicketsFun*> &tickets, const std::string& date);//拿门票信息
		int GetAdjacentDaysOpenCloseTime(const std::string& date, const LYPlace* place, std::vector<std::pair<int, int>>& openCloseList);
		const LYPlace* SetCustomPlace(const int& type, const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, const int& customMod);

	public:
	// placeinfo相关
		bool IsVarPlaceOpen(const VarPlace* varplace);
		bool IsVarPlaceOpen(const VarPlace* varplace, const std::string& date, std::vector<std::string>& time_rule);
		bool IsTourAvailable(const Tour* tour, const std::string& date);//玩乐是否可用并且有票
		int VarPlace2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int GetViewInfoFromTkt(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int View2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int Tour2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int Restaurant2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int Shop2PlaceInfo(const VarPlace* vPlace, const std::string& date, PlaceInfo*& pInfo);
		int GetOpenCloseTime(const std::string& date, const VarPlace* vPlace, std::vector<std::pair<int, int>>& openCloseList);

	public:
	// 其它函数
		int RemoveDupPlaceByCids(std::vector<const LYPlace*>& place_list);//从低优先级城市规划里去除同时位于多个城市place
		int RemoveNotPlanPlace(std::vector<const LYPlace*>& placeList);
		int DumpPath(bool log = false);

		int GetRestType(const LYPlace* place, time_t arrive, time_t depart);//获取餐厅类型（早中晚或空餐厅）
		int DelPlace(const LYPlace* dPlace);
		bool IsRestaurant(const LYPlace* place);
		bool IsDeletable(const LYPlace* dPlace);

		bool IsTimeOut();
		bool IsLFH(const LYPlace* place);
		int IsUserDateAvail(const LYPlace* place, const std::string& date);// 在指定日期是否可拓展 
		int AvailDurLevel(); //可用时长的档次（6小时为1档）

	//------------变量相关--------------
	public:
	// 成员变量
	// //通用信息 主要是一些默认值
		// 预设时长 todo: 不同city不同value
		int m_ATRTimeCost;               // 平均交通时间
		int m_ATRDist;               // 平均交通距离
		//机场
		int m_EntryAirportTimeCost;  // 机场入境耗时
		int m_ExitAirportTimeCost;  // 机场出境耗时
		int m_EntryZipAirportTimeCost;  // 机场极限入境耗时
		int m_ExitZipAirportTimeCost;  // 机场极限出境耗时
		//火车站
		int m_EntryStationTimeCost;  // 车站入境耗时
		int m_ExitStationTimeCost;  // 车站出境耗时
		int m_EntryZipStationTimeCost;  // 车站极限入境耗时
		int m_ExitZipStationTimeCost;  // 车站极限出境耗时
		//汽车站
		int m_EntryBusStationTimeCost;  // 长途汽车站
		int m_ExitBusStationTimeCost;
		int m_EntryZipBusStationTimeCost;  // 极限长途汽车站
		int m_ExitZipBusStationTimeCost;
		//港口
		int m_EntrySailStationTimeCost;  // 港口
		int m_ExitSailStationTimeCost;
		int m_EntryZipSailStationTimeCost;  // 极限港口
		int m_ExitZipSailStationTimeCost;
		//租车点
		int m_EntryCarStoreTimeCost;  // 租车
		int m_ExitCarStoreTimeCost;
		int m_EntryZipCarStoreTimeCost;  // 极限租车
		int m_ExitZipCarStoreTimeCost;
		//coreHotel //以酒店虚拟的到达点 //时间为0
		int m_ExitHotelTimeCost;	//到达该城市的第一个点为酒店
		int m_EntryHotelTimeCost;	//离开该城市的第一个点为酒店

		int m_MinSleepTimeCost;  // hotel休息最少时间，少于该时间不住酒店
		int m_MaxSleepTimeCost;
		int m_AvgSleepTimeCost;
		int m_sleepCut;  // 睡觉时间可被压缩
		int m_LeaveLuggageTimeCost;  // 第一天先到hotel放行李时间
		int m_ReclaimLuggageTimeCost;  // 最后一天去hotel取行李时间
		int m_Day0LeastViewTimeCost;  // 第一天所需最少游玩时间，小于该时间不玩了直接呆hotel
		int m_MaxWaitOpenTimeCost;  // 最大等开门时间
		int m_MinViewDur;//最小景点游玩时间

	public:
	// 规划请求相关变量
		// 基本属性
		QueryParam m_qParam;
		bool m_faultTolerant;  // 是否容错(开关门不OK也出Path)
		bool m_richPlace;  // 是否自动补充POI

		const City* m_City;
		const LYPlace* m_arvNullPlace;
		const LYPlace* m_deptNullPlace;
		int m_AdultCount; //请求人数 与玩乐门票张数相关
		const LYPlace* m_ArrivePlace;
		const LYPlace* m_DepartPlace;
		int m_ArriveTime;
		int m_DepartTime;
		double m_TimeZone;
		int m_QuerySource;  // 请求来源 andorid ios web
		// 偏好
		double m_scalePrefer;//游玩强度偏好
		int m_HotelOpenTime;
		int m_HotelCloseTime; //酒店出发/返回时间
		std::tr1::unordered_map<std::string, std::pair<time_t, time_t> > m_dayRangeMap;  // todo: 暂不填值 现有设计每天都相同范围
	public:
	//规划相关
		bool m_useDay17Limit;//17点后不规划
		bool m_dayOneLast3hNotPlan;//首尾天<3h不规划逻辑
		bool m_isPlan;  //判断该城市是否需要规划
		bool m_notPlanCityPark;//跳过国家公园的规划

		int m_runType;			//算法类型 详见RUN_TYPE
		std::vector<RouteBlock*> m_RouteBlockList;	//城市

		std::vector<KeyNode*> m_KeyNode;		//keynode
		// keynode缩放比例
		double m_KeyNodeFixRatio;
		bool m_planAfterZip;//在压缩站点时间之后 是否做规划

		std::vector<TimeBlock*> m_BlockList;	//时间段
		int m_availDur;  		// block时长和
		std::tr1::unordered_map<std::string, int> m_allocDurMap;//景点的估计时长
		std::tr1::unordered_map<std::string, int> m_crossMap;  //路线交叉

		PathView m_PlanList;//规划结果

		CityBParam m_cbP;
	public:
	//规划选点
		std::vector<const LYPlace*> m_CityViewList;  					// city内所有景点
		std::tr1::unordered_map<std::string, DurS*> m_durSMap;			//城市对应各点游玩时长map
		std::vector<const PlaceInfo*> m_varPlaceInfoList;				//景点、餐厅、商场信息
		std::tr1::unordered_map<const LYPlace*, int> m_missLevelMap; 	//必去点miss值 用于选取路线
		std::tr1::unordered_map<std::string, int> m_hotMap;				//place_Id 对应hot值
		std::tr1::unordered_set<const LYPlace*> m_systemOptSet;			//系统推荐点集合(来源于不可删点或长远热点)
		std::tr1::unordered_set<const LYPlace*> m_userOptSet;			//用户推荐点集合
		std::tr1::unordered_set<const LYPlace*> m_lookSet;				//必去点不满足开关门 通过缩短游玩时长 规划进行程 (外观游览)
		std::tr1::unordered_set<const LYPlace*> m_failSet;				//waitList中无法规划的点(不满足开关门要求等)

		std::vector<const LYPlace*> m_waitPlaceList;//待规划点

		//聚类矩阵 距离初始化
		double GetDist (const LYPlace* placeA, const LYPlace* placeB);

		//必去点
		std::tr1::unordered_set<const LYPlace*> m_userMustPlaceSet;	//用户必去点 (waitList中会排序，不需要使用vector)
		std::tr1::unordered_set<const LYPlace*> m_userDelSet;		//未规划的用户必去点 报错

		std::tr1::unordered_map<std::string, std::pair<int, int> > m_vPlaceOpenMap; //开关门
		std::tr1::unordered_map<std::string, int> m_UserDurMap; 					//游玩时长
		std::tr1::unordered_map<std::string, LYPlace*> m_customPlaceMap;			//自定义点集
		std::tr1::unordered_map<std::string, std::tr1::unordered_map<int, int> > m_pid2ticketIdAndNum;	//pid ticketId ticketnum 玩乐票种及张数
		std::map<std::string,std::string> m_poiPlays;		//poi的游玩信息，如:入内，外观
		std::map<std::string,int> m_poiFunc;		//poi的功能信息，如:到达/离开站点 放/取行李点

		//景点多次游玩 临时方案
		std::tr1::unordered_map<std::string, int> m_placeNumberMap;
		std::tr1::unordered_map<std::string, const LYPlace*> m_multiPlaceMap;	//新id id#次数
	// Place相关
		// Restaurant相关
		int m_RestNeedNum;
		int m_DefaultRestaurantComp;  // 默认每天吃几顿饭
		std::vector<RestaurantTime> m_RestaurantTimeList;  // 早中晚、下午茶饭点时间（类型，开始时间，结束时间，花费时间, 最小花费时间）
		std::tr1::unordered_map<std::string, int> m_RestTypeMap;  // 某个饭店被设置成什么类型
		static const RestaurantTime m_breakfastTime;
		static const RestaurantTime m_lunchTime;
		static const RestaurantTime m_afternoonTeaTime;
		static const RestaurantTime m_supperTime;

		// Shop相关
		int m_shopIntensity;  // 购物强度
		int m_ShopNeedNum;

		//玩乐poi接送相关
		int AddTourJieSongPoi (const Tour* tour, std::tr1::unordered_set<std::string>& PoiSet);
		int AddTourGatherOrLeftPoi (const Tour* tour, std::tr1::unordered_set<const LYPlace*>& PoiSet, bool isDismiss = false);
		int AddTourJieSongHotel (std::tr1::unordered_set<const LYPlace*>& PoiSet);
		double SelectTransPois (const LYPlace* placeA, const LYPlace* placeB, std::vector<const LYPlace*>& transList);

	public:
	//交通相关
		int m_realTrafNeed;
		int m_travalType;//行程类型（常规／包车）
		int m_trafPrefer; //整体交通偏好
		std::tr1::unordered_map<std::string, int> m_date2trafPrefer;//某天对应的交通偏好
		std::tr1::unordered_map<std::string, TrafficItem*> m_TrafficMap;	// 交通信息
		std::tr1::unordered_map<std::string, TrafficItem*> m_customTrafMap;  // SS006 用户自定义交通
		std::tr1::unordered_map<std::string, TrafficItem*> m_lastTripTrafMap;  //保留用户前一次行程的交通 此map中不包括自定义交通
		std::tr1::unordered_map<std::string, int> m_AvgTrafTimeMap;
		std::tr1::unordered_map<std::string, int> m_AvgTrafDistMap;
		std::tr1::unordered_map<std::string, int> m_maxTrafTimeMap;
		std::tr1::unordered_map<std::string, int> m_maxTrafDistMap;

	public:
	//ssv007
		//报错 目前仅对团游
		typedef	std::multimap<int, std::pair<int, std::string> >::iterator TryErrMapIt;// 仅for 团游
		std::multimap<int, std::pair<int, std::string> > m_tryErrorMap;	//error <dayIdx, <errorId, content> >
		std::tr1::unordered_set<int> m_failedHotelDidx;//报错酒店点的date下标 
	//s125
		bool isChangeTraffic;  //是否为修改交通 action字段
		bool m_isImmediately; //控制是否直接游玩，不放行李，为true表示不放行李
	//s127 参考行程 已规划点不再规划
		std::tr1::unordered_set<std::string> m_notPlanSet;//不做规划的点集 适用于ssv005的接口
		std::tr1::unordered_set<std::string> m_notPlanDateSet;//不规划的日期

	//s130
		//s130智能优化选项 2.游玩时间范围|1.顺序|0.时长
		std::bitset<3> m_OptionMode;
		std::bitset<3> m_SelectedOptMode; //s130选择结果
		int s130reqAvail;//130请求行程是否可用
		//通过 请求005/006 及以下两个变量控制实现智能优化8种可能
		bool m_keepTime;	//是否保持游玩时长
		bool m_keepPlayRange;//是否保持每日出发返回时间

		bool clashDelPoi; //控制是否删点
		//m_PlaceDateMap :点不能在日期集合外的时间放,但在日期集合中最终能不能放是不确定的
		//就如我确定要在11月份领结证,但11月份民政局并不是在这个范围内每天每刻都上班的
		//用来保证 相同的点 高级编辑时不在同一天， 智能优化后 仍在不同天 //其实实现了所在天未变
		std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> > m_PlaceDateMap;  //景点规划所在天 key: id, value: set<date>
	//其他
		std::vector<std::string> m_invalidPois;//异常点报错 目前无该逻辑
	public:
	//列表相关
		int m_reqMode;  // 列表页的请求类型 餐馆/购物/景点/玩乐
		int m_pageIndex;//第几页
		int m_numEachPage;//每页展示个数
		int m_sortMode;//排序选项规则 正序or逆序
		int m_privateFilter;//私有数据筛选(纯私有) 玩乐没有公转私
		int m_utime;	//更新时间过滤
		long m_maxDist; //列表页过滤时的最大距离
		std::string m_key;//关键词检索时用的关键词（点id）
		std::string m_listDate;//玩乐列表的日期
		std::vector<std::string> m_listCity;//列表的城市串
		std::vector<std::string> m_listCenter;//按距离搜索 中心点
		std::vector<std::string> m_filterTags; //存储过滤tag
		std::vector<POIDetail> m_POIList;	//需要获取详情的POI p105
		std::vector<ProductTicket> m_productList;	//需要验证的数据(解析请求中的玩乐信息)
		// 列表页结果
		std::vector<ShowItem> m_showItemList;//view,rest,shop,tour

	public:
	// 性能统计
		MJ::MyTimer m_CutTimer;  // 卡时
		int m_CutThres;  // 卡时
		int m_CutNullThres;  // 无结果卡时

		PlanStats m_planStats;//以block为单位，记录leftTime（剩余时间），trafTime（交通花费时间），throwNum（扔点数量）,place dur，扔点原因（eg：Greedy,dfs）
		BaseCost m_cost;//DoPlan 过程中各阶段花费的时间
		BaseStat m_stat;
		ProcCost m_pCost;
		ProcStat m_pStat;
		ErrorInfo m_error;
		DebugInfo m_dbg;

		pthread_mutex_t m_durLocker; //进程锁

		//内部测试用
		bool m_useCBP;
		bool m_useRealTraf;
		bool m_useTrafShow;
		bool m_useKpi;
		bool m_useStaticTraf;
};
#endif

