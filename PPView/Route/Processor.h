#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__

#include <iostream>
#include "MJCommon.h"
#include "Route/base/LYConstData.h"
#include "Route/base/BasePlan.h"
#include "Route/base/DataList.h"
#include "Route/vlist/ViewRankProcessor.h"
#include "Route/vlist/ShopRankProcessor.h"
#include "Route/vlist/RestaurantRankProcessor.h"
#include "Route/vlist/HotelRankProcessor.h"
#include "Route/vlist/ViewShopRankProcessor.h"
#include "Route/bag/ThreadMemPool.h"
#include "Planner.h"

class Processor {
	public:
		Processor() {}
		~Processor() {}
	public:
		//分类处理请求
		static int Process(const QueryParam& param, Json::Value& req, Json::Value& resp);
	private:
		// 接口处理
		static int DoProcessP202(BasePlan* basePlan);
		static int DoProcessP201(BasePlan* basePlan);
		static int DoProcessP104(BasePlan* basePlan);
		static int DoProcessP105(BasePlan* basePlan);
		static int DoProcessB116(BasePlan* basePlan);

		//过滤Tag
		static int FilterTag(std::vector<const LYPlace*>& pList, std::vector<std::string> tags);
		//过滤共有私有
		static int FilterPrivate(const QueryParam& qParam, std::vector<const LYPlace*>& pList, int privateOnly);
		//过滤指定企业私有
		static int FilterPtid(const QueryParam& qParam, std::vector<const LYPlace*>& pList, const std::string& ptid, int privateOnly);
		// 按距离过滤
		static int FilterDist(BasePlan* basePlan, std::vector<const LYPlace*>& pList, long dist);
		// 按编辑时间过滤
		static int FilterUtime(const BasePlan* basePlan, std::vector<const LYPlace*>& pList, int utime);
        // 按id过滤
		static int FilterId(const QueryParam& qParam, std::vector<const LYPlace*>& pList, const std::string& id);

		static int MarkPlaceList(BasePlan* basePlan);
		static int getCandidatePoisUnderDist(BasePlan* basePlan, std::vector<const LYPlace*>& inputList, std::vector<const LYPlace*>& outputList, const std::string& coord, long dist);
		static int ListResortViewShopRestTour(std::vector<std::vector<ShowItem> > showList, std::vector<int> perNum, std::vector<int> maxNum, int topNum, std::vector<ShowItem>& pList);
};
#endif
