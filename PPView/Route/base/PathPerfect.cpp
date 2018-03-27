#include <iostream>
#include "PathUtil.h"
#include "PathEval.h"
#include "PathPerfect.h"
#include "PathTraffic.h"
#include "TrafficData.h"
int PathPerfect::DoPerfect(BasePlan* basePlan) {
	MJ::PrintInfo::PrintLog("[%s]PathPerfect::DoPerfect, MergeSame...", basePlan->m_qParam.log.c_str());
	PathUtil::MergeLuggAndSleep(basePlan);
	PathUtil::BackHotelTimeChange(basePlan->m_PlanList);
	RichAttach(basePlan, basePlan->m_PlanList, basePlan->m_RestaurantTimeList);
	basePlan->m_PlanList.Dump(basePlan);

//	MJ::PrintInfo::PrintDbg("[%s]PathPerfect::DoPerfect, DoBanlance...", basePlan->m_qParam.log.c_str());
//	MJ::PrintInfo::PrintDbg("[%s]PathPerfect::DoPerfect, DoNormDur...", basePlan->m_qParam.log.c_str());
//	MJ::PrintInfo::PrintDbg("[%s]PathPerfect::DoPerfect, DoMergeIdle...", basePlan->m_qParam.log.c_str());
//	MJ::PrintInfo::PrintDbg("[%s]PathPerfect::DoPerfect, DoWhat...", basePlan->m_qParam.log.c_str());
//	MJ::PrintInfo::PrintDbg("[%s]PathPerfect::DoPerfect, DoAddIdle...", basePlan->m_qParam.log.c_str());
	if (basePlan->m_useTrafShow) {
		AddTrafShow(basePlan);
	}
	basePlan->m_PlanList.Dump(basePlan);
	basePlan->m_stat.m_placeNum = basePlan->m_PlanList._placeNum;
	basePlan->m_stat.m_hot = basePlan->m_PlanList._hot;
	basePlan->m_stat.m_dist = basePlan->m_PlanList._dist;
	basePlan->m_stat.m_time = basePlan->m_PlanList._time;
	basePlan->m_stat.m_blank = basePlan->m_PlanList._blank;
	basePlan->m_stat.m_score = basePlan->m_PlanList._score;

	RealTimeTrafStat(basePlan);
	return 0;
}

// 统计实时交通占比
int PathPerfect::RealTimeTrafStat(BasePlan* basePlan) {
	if (basePlan->m_realTrafNeed & REAL_TRAF_REPLACE || basePlan->m_realTrafNeed & REAL_TRAF_ADVANCE) {
		int trafNum = 0;
		int realTrafNum = 0;
		PathView& path = basePlan->m_PlanList;
		for (int i = 0; i < path.Length(); ++i) {
			const PlanItem* item = path.GetItemIndex(i);
			if (item && item->_departTraffic) {
				++trafNum;
				if (item->_departTraffic->m_stat == TRAF_STAT_REAL_TIME) {
					++realTrafNum;
				}
			}
		}
		basePlan->m_stat.m_trafNum = trafNum;
		basePlan->m_stat.m_realTrafNum = realTrafNum;
	}
	return 0;
}

int PathPerfect::AddTrafShow(BasePlan* basePlan) {
	std::vector<TrafficItem*> trafficList;
	for (int i = 0; i < basePlan->m_PlanList.Length(); ++i) {
		PlanItem* planItem = basePlan->m_PlanList.GetItemIndex(i);
		if (planItem->_departTraffic != NULL && TrafficData::IsReal(planItem->_departTraffic->_mid)) {
			trafficList.push_back(const_cast<TrafficItem*>(planItem->_departTraffic));
		}
		if (planItem->_arriveTraffic != NULL && TrafficData::IsReal(planItem->_arriveTraffic->_mid)) {
			trafficList.push_back(const_cast<TrafficItem*>(planItem->_arriveTraffic));
		}
	}
	if (basePlan->m_PlanList.Length() == 0) {
		for (auto trafKey:basePlan->m_TrafficMap) {
			trafficList.push_back(trafKey.second);
		}
	}
	if (trafficList.size() == 0) return 1;
	int ret = PathTraffic::GetTrafficSummary(basePlan, trafficList);
	if (ret) return 1;
	return 0;
}

int PathPerfect::RichAttach(BasePlan* basePlan, PathView& path, std::vector<RestaurantTime>& restTimeList) {
	MJ::PrintInfo::PrintLog("[%s]PathPerfect::RichAttach, Dump Ori Path", basePlan->m_qParam.log.c_str());
	path.Dump(basePlan);

	int restNeed = RESTAURANT_TYPE_LUNCH | RESTAURANT_TYPE_SUPPER;
	for(int i=0; i<restTimeList.size(); i++)
	{
		if(not (restTimeList[i]._type & restNeed)) continue;

		PlanItem* firstItem = path.GetItemIndex(0);
		for (int begIdx=0, endIdx = 1; endIdx < path.Length(); ++endIdx) { //从1开始：从第一个hotel开始尝试结束一天
			const PlanItem* begItem = path.GetItemIndex(begIdx);
			PlanItem* endItem = path.GetItemIndex(endIdx);
			int lastHotelIdx;
			PlanItem* lastHotelItem = path.GetItemLastHotel(endIdx, lastHotelIdx);
			if (
					(endItem->_place->_t & LY_PLACE_TYPE_HOTEL) &&
					((lastHotelItem && endItem->_departDate!=lastHotelItem->_departDate) //当前是酒店 存在上一个酒店 当前日期与之前的日期不是一个
					||(!lastHotelItem && firstItem->_departDate!=endItem->_departDate)) //当前是酒店 没有上一个酒店 且不是第一天
					||endItem->_place->_t & LY_PLACE_TYPE_ARRIVE) { //当前点是离开点
				//找到分界点
				MJ::PrintInfo::PrintDbg("SMZ found %d %d", begIdx, endIdx);
				RichAttachNodeMark(basePlan, path, begIdx, endIdx, restTimeList[i]);
				begIdx = endIdx +1 ;
			}
		}
	}

	return 0;
}

int PathPerfect::RichAttach(BasePlan* basePlan, Json::Value& jViewDayList) {
	std::vector<RestaurantTime>& restTimeList = basePlan->m_RestaurantTimeList;
	for (int didx = 0; didx < jViewDayList.size(); didx++) {
		Json::Value& jViewList = jViewDayList[didx]["view"];
		int restNeed = RESTAURANT_TYPE_LUNCH | RESTAURANT_TYPE_SUPPER;
		for(int i=0; i<restTimeList.size(); i++)
		{
			if(not (restTimeList[i]._type & restNeed)) continue;

			int dinnerIdx = -1;
			std::map<int, int> overlapTime2vidx;
			for (int vidx=0; vidx < jViewList.size(); vidx++) { //从1开始：从第一个hotel开始尝试结束一天
				if (didx != 0 and vidx == 0) continue; //中间天早上出发酒店不就餐
				Json::Value& jView = jViewList[vidx];
				int stime = MJ::MyTime::toTime(jView["stime"].asString(), basePlan->m_TimeZone);
				int etime = MJ::MyTime::toTime(jView["etime"].asString(), basePlan->m_TimeZone);
				int type = jView["type"].asInt();
				int overlapTime = overlapTimeWithRestSlot(basePlan->m_TimeZone, stime, etime, restTimeList[i], type);
				if (overlapTime > 0) {
					overlapTime2vidx[overlapTime] = vidx;
				}
			}
			if (!overlapTime2vidx.empty()) {
				dinnerIdx = overlapTime2vidx.rbegin()->second;
			}
			if (dinnerIdx != -1 and dinnerIdx < jViewList.size() and jViewList[dinnerIdx]["type"] != LY_PLACE_TYPE_RESTAURANT) {
				jViewList[dinnerIdx]["dining_nearby"] = 1;
			}
		}
	}

	return 0;
}
//与就餐槽位的重叠时间 type为饭店时，范围槽位时长
int PathPerfect::overlapTimeWithRestSlot(int timeZone, int stime, int etime, RestaurantTime& restTime, int type) {
	std::string date = MJ::MyTime::toString(stime, timeZone).substr(0,8);
	time_t restOpen = MJ::MyTime::toTime(date + "_00:00", timeZone) + restTime._begin;
	time_t restClose = MJ::MyTime::toTime(date + "_00:00", timeZone) + restTime._end;
	if (ToolFunc::HasUnionPeriod(restOpen,restClose,stime,etime)){
		if(type & LY_PLACE_TYPE_RESTAURANT)
		{
			return restClose-restOpen;
		}
		else {
			int restStime = stime>restOpen ? stime:restOpen;
			int restEtime = etime<restClose ? etime:restClose;
			return restEtime-restStime;
		}
	}
	return 0;
}

int PathPerfect::RichAttachNodeMark(BasePlan* basePlan, PathView& path, int begIdx, int endIdx, RestaurantTime& restTime) {
	std::string date = path.GetItemIndex(begIdx)->_departDate;
	double timeZone = path.GetItemIndex(begIdx)->_timeZone;
	time_t restOpen = MJ::MyTime::toTime(date + "_00:00", timeZone) + restTime._begin;
	time_t restClose = MJ::MyTime::toTime(date + "_00:00", timeZone) + restTime._end;

	std::vector<PlanItem*> possibleAttachItemList;
	std::map<int,int> rawIdxMap;
	for (int i = begIdx; i <= endIdx; ++i) {
		PlanItem* item = path.GetItemIndex(i);
		if (ToolFunc::HasUnionPeriod(restOpen,restClose,item->_arriveTime,item->_departTime)){
			if(item->_place->_t & LY_PLACE_TYPE_RESTAURANT || item->m_hasAttach)
			{
				MJ::PrintInfo::PrintErr("[%s]PathPerfect::RichAttachNodeMark no need to mark", basePlan->m_qParam.log.c_str());
				return 0;
			}
			else{
				MJ::PrintInfo::PrintDbg("place %s(%s) push to mark", item->_place->_ID.c_str(), item->_place->_name.c_str());
				rawIdxMap[possibleAttachItemList.size()]=i;
				possibleAttachItemList.push_back(item);
			}
		}
	}
	if(possibleAttachItemList.size() > 0) {
		int maxStayIdx = 0;
		int maxNodeStay = -1;
		for (int i = 0; i < possibleAttachItemList.size(); i++) {//选一个重合时间最长的点
			int start = std::max(restOpen,possibleAttachItemList[i]->_arriveTime);
			int end = std::min(restClose,possibleAttachItemList[i]->_departTime);
			int stay = std::max(0,end-start);
			if (stay > maxNodeStay) {
				maxNodeStay = stay;
				maxStayIdx = i;
			}
		}
		MJ::PrintInfo::PrintDbg("place %s(%s) final mark id:%d", possibleAttachItemList[maxStayIdx]->_place->_ID.c_str(), possibleAttachItemList[maxStayIdx]->_place->_name.c_str(),rawIdxMap[maxStayIdx]);
		int & AttachType = possibleAttachItemList[maxStayIdx]->m_hasAttach;
		AttachType |= restTime._type;
		return 0;
	} else {
		//无位置可以mark
		MJ::PrintInfo::PrintErr("[%s]PathPerfect::RichAttachNodeMark no item to mark", basePlan->m_qParam.log.c_str());
		return 1;
	}
}
