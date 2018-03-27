#include <iostream>
#include "Route/base/TrafRoute.h"
#include "Route/base/PathView.h"
#include "Route/base/BagParam.h"
#include "Route/base/PathEval.h"
#include "Route/base/PathUtil.h"
#include "RouteWorker.h"
#include "MyTime.h"
#include "RouteSearch.h"

struct PathViewScoreCmp {
	bool operator() (const PathView* pa, const PathView* pb) {
		if (pa->m_missLevel != pb->m_missLevel) {
			return (pa->m_missLevel < pb->m_missLevel);
		} else {
			return (pa->_score > pb->_score);
		}
	}
};

int RouteSearch::DoSearch(BagPlan* bagPlan, std::vector<const SPath*>& dfsList, MyThreadPool* threadPool) {
	int ret = 0;

	// 0 跑RouteWorker
	std::vector<MyWorker*> jobs;
	jobs.reserve(dfsList.size());
	for (int i = 0; i < dfsList.size(); ++i) {
		const SPath* dfsPath = dfsList[i];
		RouteWorker* routeWorker = new RouteWorker(bagPlan, dfsPath);
		jobs.push_back(dynamic_cast<MyWorker*>(routeWorker));
		threadPool->add_worker(dynamic_cast<MyWorker*>(routeWorker));
	}
	threadPool->wait_worker_done(jobs);

	// 1 取路径
	std::vector<PathView*> routeList;
	routeList.reserve(jobs.size());
	std::tr1::unordered_set<std::string> pathStrSet;
	for (int i = 0; i < jobs.size(); ++i) {
		RouteWorker* routeWorker = dynamic_cast<RouteWorker*>(jobs[i]);
		PathView* path = routeWorker->m_path;
		if (!path->m_isValid) continue;
		std::string idStr = path->GetIDStr();
		if (pathStrSet.find(idStr) == pathStrSet.end()) {
			routeList.push_back(path);
			pathStrSet.insert(idStr);
		}
	}

	// 2 评价
	DoEval(bagPlan, routeList);

	// 3 取最优
	std::sort(routeList.begin(), routeList.end(), PathViewScoreCmp());
	if (!routeList.empty()) {
		bagPlan->m_PlanList.Copy(routeList.front());
		MJ::MyTimer t;
		t.start();
		AdjustTight(bagPlan, &(bagPlan->m_PlanList));
		MJ::PrintInfo::PrintDbg("[%s]RouteSearch::DoSearch, AdjustTight cost %d ms", bagPlan->m_qParam.log.c_str(),  t.cost() / 1000);
	}

	MJ::PrintInfo::PrintLog("[%s]RouteSearch::DoSearch, PathLen_route: %d", bagPlan->m_qParam.log.c_str(), routeList.size());
	for (int i = 0; i < routeList.size(); ++i) {
		if (!bagPlan->m_track && i > 100) break;
		MJ::PrintInfo::PrintLog("[%s]RouteSearch::DoSearch, route[%d]", bagPlan->m_qParam.log.c_str(), i);
		fprintf(stderr, "jjj %d\t%.4f\thotValue: %.2f\tmiss: %d\ttrafDist: %d\n", i, routeList[i]->_score, routeList[i]->m_hotValue, routeList[i]->m_missLevel, routeList[i]->m_trafDist);
		routeList[i]->Dump(bagPlan, false);
	}

	// 4 析构
	for (int i = 0; i < jobs.size(); ++i) {
		RouteWorker* routeWorker = dynamic_cast<RouteWorker*>(jobs[i]);
		delete routeWorker;
	}
	jobs.clear();

	return 0;
}

int RouteSearch::AdjustTight(BagPlan* bagPlan, PathView* path) {
	MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight, start to AdjustTight", bagPlan->m_qParam.log.c_str());
	int ret = 0;
	bool needAdjust = false;
	PathView* newPath = NULL;
	std::vector<DayItem*> dayList;
	std::vector<std::string> dateList;
	ret = CalUsedDurPerDay(bagPlan, path, dayList);
	if (!ret) {
		//获取每一天的日期
		for (int k = 0; k < dayList.size(); k++) {
			MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight, day %d, date %s, dur %d", bagPlan->m_qParam.log.c_str(), k, dayList[k]->m_date.c_str(), dayList[k]->m_usedDur);
			dateList.push_back(dayList[k]->m_date);
		}
	}
	if (dayList.size() > 8) goto clean;
	if (!ret && dayList.size() > 1) {
		int startDay = 0;
		int endDay = dayList.size();
		std::vector<DayItem*>::iterator startIt = dayList.begin();
		std::vector<DayItem*>::iterator endIt = dayList.end();
        //一个城市的首尾两天不参与"前紧后松调整"
		{
			startDay++;
			startIt++;
			endDay--;
			endIt--;
		}
		while (endDay > 0 && dayList[endDay - 1]->m_usedDur == 0) {
			endDay--;
			endIt--;
		}
		//按每天的景点使用时长递减排序
		if (endDay > startDay) std::sort(startIt, endIt, Compare);
		for (int k = 0; k < dayList.size(); k++) {
			MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight, after sort day %d, date %s, dur %d", bagPlan->m_qParam.log.c_str(), k, dayList[k]->m_date.c_str(), dayList[k]->m_usedDur);
			if (dayList[k]->m_date != dateList[k]) needAdjust = true;
		}
		if (!needAdjust) {
			MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight don't need adjust", bagPlan->m_qParam.log.c_str());
			goto clean;
		}
		newPath = new PathView(path->Length());
		ret = Perm(bagPlan, newPath, dayList, startDay, endDay, dateList, path->Length());
		if (!ret) {
			MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight adjust sucess", bagPlan->m_qParam.log.c_str());
			newPath->m_hashFrom = path->m_hashFrom;
			path->Copy(newPath);
			if (ret == 0) {
				ret = PathUtil::TimeEnrich(bagPlan, *path);
			}
			ret = CalUsedDurPerDay(bagPlan, path, dayList);
			for (int k = 0; k < dayList.size(); k++) {
				MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight, after adjust day %d, date %s, dur %d", bagPlan->m_qParam.log.c_str(),  k, dayList[k]->m_date.c_str(), dayList[k]->m_usedDur);
			}
			path->Dump(bagPlan);
		} else {
			MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight adjust failed", bagPlan->m_qParam.log.c_str());
		}
		delete newPath;
		newPath = NULL;
	} else if (dayList.size() <= 1){
		MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight don't need adjust cause dayList.size() <= 1 ", bagPlan->m_qParam.log.c_str());
	} else {
		MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight don't need adjust cause CalUsedDurPerDay error", bagPlan->m_qParam.log.c_str());
	}
clean:
	for (int i = 0; i < dayList.size(); i++) {
		if (dayList[i])	delete dayList[i];
		dayList[i] = NULL;
	}
	dayList.clear();
	MJ::PrintInfo::PrintDbg("[%s]RouteSearch::AdjustTight, end to AdjustTight", bagPlan->m_qParam.log.c_str());
	return 0;
}

int RouteSearch::CalUsedDurPerDay(BagPlan *bagPlan, const PathView* path, std::vector<DayItem*>& dayList) {
	int day = -1;
	int usedTime = 0;
	std::string date = "";
	std::vector<PlaceOrder> orderList;

	for (int i = 0; i < dayList.size(); i++) {
		if (dayList[i]) delete dayList[i];
		dayList[i] = NULL;
	}
	dayList.clear();
	for (int i = 1; i < path->Length(); i++) {
		int placeUsed = 0;
		const PlanItem* pItem = path->GetItemIndex(i);
		const PlanItem* lItem = path->GetItemIndex(i - 1);
		if (date != lItem->_departDate) { //若换天，使用的时长归于这一天
			if (day != -1) {
				dayList.push_back(new DayItem(date, usedTime, orderList));
				usedTime = 0;
			}
			orderList.clear();
			date = lItem->_departDate;
			day++;
		}
		int trafT = 0;
		if (lItem->_departTraffic) {
			trafT = lItem->_departTraffic->_time;
		}
		if (pItem->_arriveTraffic) {
			trafT = pItem->_arriveTraffic->_time;
		}
		if (pItem->_type & NODE_FUNC_PLACE) placeUsed = pItem->_departTime - pItem->_arriveTime;
		if (pItem->_type & NODE_FUNC_PLACE || lItem->_type & NODE_FUNC_PLACE) {
			usedTime += placeUsed + trafT;
		}
		if (0) {
			if (pItem->_place) {
				MJ::PrintInfo::PrintDbg("[%s]RouteSearch::CalUsedDurPerDay, place %s(%s), day %d, date %s, used %d, lItem->_departTraffic %d, pItem->_arriveTraffic %d, traf %d", bagPlan->m_qParam.log.c_str(),  pItem->_place->_ID.c_str(), pItem->_place->_name.c_str(), day, date.c_str(), usedTime, lItem->_departTraffic !=NULL, pItem->_arriveTraffic != NULL, trafT);
			} else {
				MJ::PrintInfo::PrintDbg("[%s]RouteSearch::CalUsedDurPerDay, day %d, date %s, used %d, lItem->_departTraffic %d, pItem->_arriveTraffic %d, traf %d", bagPlan->m_qParam.log.c_str(),
						day, date.c_str(), usedTime, lItem->_departTraffic !=NULL, pItem->_arriveTraffic != NULL, trafT);
			}
		}
		if (!(pItem->_type & NODE_FUNC_KEY) && pItem->_place != LYConstData::m_attachRest) {
			//注：此处会去掉附近就餐，所以前松后紧调整成功之后需重新添加附近就餐
			unsigned char nodeType = NODE_FUNC_PLACE_VIEW_SHOP;
			if (pItem->_type == NODE_FUNC_PLACE_REST_LUNCH) {
				nodeType = NODE_FUNC_PLACE_REST_LUNCH;
			} else if (pItem->_type == NODE_FUNC_PLACE_REST_SUPPER) {
				nodeType = NODE_FUNC_PLACE_REST_SUPPER;
			}
			orderList.push_back(PlaceOrder(pItem->_place, date, orderList.size(), nodeType));
		}
	}
	if (dayList.size() < day + 1) dayList.push_back(new DayItem(date, usedTime, orderList));
	return 0;
}

bool RouteSearch::Compare(const DayItem* lItem, const DayItem* rItem) {
	return lItem->m_usedDur > rItem->m_usedDur;
}

int RouteSearch::SwapTwoDays(std::vector<DayItem*>& dayList, int lDay, int rDay) {
	if (lDay == rDay) return 0;
	DayItem* tmpDay = dayList[lDay];
	dayList[lDay] = dayList[rDay];
	dayList[rDay] = tmpDay;

	return 0;
}

int RouteSearch::Perm(BagPlan* bagPlan, PathView* newPath, std::vector<DayItem*>& dayList, int dayStart, int dayEnd, const std::vector<std::string>& dateList, int len) {
	int ret = 0;
	if (dayStart >= dayEnd) {
		if (0) {
			MJ::PrintInfo::PrintLog("[%s]RouteSearch::Perm, try to adjust", bagPlan->m_qParam.log.c_str());
			for (int k = 0; k < dayList.size(); k++) MJ::PrintInfo::PrintDbg("[%s]RouteSearch::Perm, try to %d, dur %d", bagPlan->m_qParam.log.c_str(),  k, dayList[k]->m_usedDur);
		}
		std::vector<PlaceOrder> orderList;
		for (int i = 0; i < dayList.size(); i++) {
			for (int j = 0; j < dayList[i]->m_orderList.size(); j++) {
				PlaceOrder placeOrder = dayList[i]->m_orderList[j];
				placeOrder._date = dateList[i];
				orderList.push_back(placeOrder);
			}
		}
		TrafRoute tRoute;
		ret = tRoute.DoRoute(bagPlan, orderList, newPath, false);
		if (!ret && newPath->m_isValid) return 0;
		newPath->Reset();
	}
	for (int i = dayStart; i < dayEnd; i++) {
		if (i != dayStart) SwapTwoDays(dayList, dayStart, i);
		ret = Perm(bagPlan, newPath, dayList, dayStart + 1, dayEnd, dateList, len);
		if (!ret) return 0;
		if (i != dayStart) SwapTwoDays(dayList, i, dayStart);
	}
	return 1;
}

int RouteSearch::DoEval(BagPlan* bagPlan, std::vector<PathView*>& routeList) {
	if (routeList.empty()) return 0;
	std::set<double> hotValueList;
	std::set<int> trafDistList;
	for (int i = 0; i < routeList.size(); ++i) {
		PathView* routePath = routeList[i];
		EvalObj(bagPlan,routeList[i]);
		hotValueList.insert(routePath->m_hotValue);
		trafDistList.insert(routePath->m_trafDist);
	}

	double hotRatio = 0.8;
	double distBottom = 5 *1000;
	for (int i = 0; i < routeList.size(); ++i) {
		PathView* path = routeList[i];
		double hotValueS = (path->m_hotValue+1)*1.0 / (*hotValueList.rbegin()+1);
		double trafDistS = 1-(path->m_trafDist+distBottom)*1.0 / (*trafDistList.rbegin()+distBottom);
		trafDistS = std::max(0.0,trafDistS);
		path->_score = hotRatio * hotValueS + (1-hotRatio) * trafDistS;

		fprintf(stderr, "jjj eval"
				"\t%.4f"
				"\t%.2f\t%.4f\t%d"
				"\t%d\t%.4f\n",
				path->_score,
				path->m_hotValue, hotValueS, path->m_missLevel,
				path->m_trafDist, trafDistS
			   );
	}

	return 0;
}

// 客观指标统计
int RouteSearch::EvalObj(BasePlan* basePlan, PathView* path) {
	// 0 游玩价值
	double hotSum = 0;
	std::tr1::unordered_set<const LYPlace*> plannedSet;
	//	path->Dump(basePlan);
	for (int i = 0; i < path->Length(); ++i) {
		const PlanItem* item = path->GetItemIndex(i);
		const LYPlace* place = item->_place;
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
		if (vPlace) {
			plannedSet.insert(vPlace);

			int dur = item->_departTime - item->_arriveTime;
			int rcmdDur = item->_durS->m_rcmd;
			double hotScale =1.0;
			if(dur<rcmdDur) hotScale = dur * 1.0 /(rcmdDur+1); //防止rcmdDur为0的情况
			if (hotScale < 0.3) hotScale = 0.0;
			hotSum += hotScale * basePlan->GetHot(vPlace) * dur/3600.0;
		}
	}
	int missLevel = 0;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (!plannedSet.count(place)) {
			missLevel += basePlan->GetMissLevel(place);
		}
	}
	//	std::cerr << "jjj hotSum " << hotSum << std::endl;

	// 1 交通距离与时间
	int trafDistSum = 0;
	int trafDurSum = 0;
	int trafBusCnt = 0;
	for (int i = 0; i < path->Length(); ++i) {
		const PlanItem* item = path->GetItemIndex(i);
		if (item->_departTraffic) {
			trafDistSum += item->_departTraffic->_dist;
			trafDurSum += item->_departTraffic->_time;
		}
		if (item->_departTraffic && item->_departTraffic->_mode != TRAF_MODE_WALKING) {
			trafBusCnt += 1;
		}
	}

	// 2 步行距离超出的最大值
	int walkDistPlus = 0;
	int walkDistBlock = 0;
	for (int keyIdx = 0; keyIdx < path->Length(); ++keyIdx) {
		const PlanItem* item = path->GetItemIndex(keyIdx);
		if (path->GetItemIndex(keyIdx)->_type & NODE_FUNC_KEY) {
			if (walkDistBlock > m_maxWalkDay) {
				if (walkDistBlock - m_maxWalkDay > walkDistPlus) {
					walkDistPlus = walkDistBlock - m_maxWalkDay;
				}
			}
			walkDistBlock = 0;
		}
		if (item->_departTraffic && item->_departTraffic->_mode == TRAF_MODE_WALKING) {
			walkDistBlock += item->_departTraffic->_dist;
		}
	}

	path->m_hotValue = hotSum;
	path->m_missLevel = missLevel;
	path->m_trafDist = trafDistSum;
	path->m_trafDur = trafDurSum;
	path->m_trafBusCnt = trafBusCnt;
	path->m_walkDistPlus = walkDistPlus;

	return 0;
}
