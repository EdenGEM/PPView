#include <iostream>
#include <algorithm>
#include <limits>
#include "PathTraffic.h"
#include "MJCommon.h"
#include "PlaceInfoAlloc.h"
#include "DataList.h"
#include "DataChecker.h"

// DoCheck准备相关数据
int DataChecker::DoCheck(BasePlan* basePlan) {
	int ret = 0;

	// 1 选择饭店,高级编辑还是需要
	ret = GetRestaurant(basePlan);
	if (ret != 0) return 1;

	// 2 选择购物
	ret = GetShop(basePlan);
	if (ret != 0) return 1;

	// 3 选择view(偏多，非最终景点)
	ret = GetView(basePlan);
	if (ret != 0) return 1;

	// 4 选择Tour
	// 并没有补充tour 不主动规划玩乐
	ret = GetTour(basePlan);
	if (ret != 0) return 1;

	//5 置HotMap
	ret = SetHotMap(basePlan);
	if (ret != 0) return 1;

	//5.1 先排序限制点
	ret = SortAllPlace(basePlan);
	if (ret != 0) return 1;

	// 6 获取交通
		ret = GetTraffic(basePlan);
		if (ret != 0) return 1;

	// 7 判断是否可用于规划
	ret = CheckInfo(basePlan);
	if (ret != 0) return 1;

	// 9 必去点miss 值 用于路线选择
	ret = CalMiss(basePlan);
	if (ret != 0) return 1;

	// 10 所有点排序
	//ret = SortAllPlace(basePlan);
	//if (ret != 0) return 1;

	return 0;
}

// 选择饭店
int DataChecker::GetRestaurant(BasePlan* basePlan) {
	// 用户指定
	basePlan->m_RestNeedNum = 0;
	std::tr1::unordered_set<std::string> addSet;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (!(place->_t & LY_PLACE_TYPE_RESTAURANT)) continue;
		if (addSet.find(place->_ID) != addSet.end()) continue;
		++basePlan->m_RestNeedNum;
		addSet.insert(place->_ID);
	}

	if (basePlan->m_richPlace) {
		int restRichNum = 0;
		int dayStay = 0;
		for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
			dayStay += basePlan->m_RouteBlockList[i]->_day_limit;
		}
		if (dayStay <= 2) {
			restRichNum = 0;
		} else {
			restRichNum = std::min(static_cast<int>(dayStay / 2), 4);
		}
		if (restRichNum > basePlan->m_RestNeedNum) {
			basePlan->m_RestNeedNum = restRichNum;
			restRichNum -= basePlan->m_RestNeedNum;
			std::vector<const LYPlace*> cRestList;
			if (LYConstData::getRestaurantLocal(basePlan->m_City->_ID, cRestList, basePlan->m_qParam.ptid)) {
				MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetRestaurant, LYConstData::getRestaurantLocal fail", basePlan->m_qParam.log.c_str());
			}
			basePlan->RemoveNotPlanPlace(cRestList);

			int restCnt = 0;
			for (int i = 0; i < cRestList.size(); ++i) {
				const Restaurant* restaurant = dynamic_cast<const Restaurant*>(cRestList[i]);
				if (restCnt >= 2 * restRichNum) break;
				if (!basePlan->IsVarPlaceOpen(restaurant)) continue;
				if (addSet.find(restaurant->_ID) != addSet.end()) continue;
				basePlan->m_waitPlaceList.push_back(restaurant);
				basePlan->m_RestTypeMap[restaurant->_ID] = RESTAURANT_TYPE_LUNCH | RESTAURANT_TYPE_SUPPER;
				addSet.insert(restaurant->_ID);
				MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetRestaurant, restaurant: %s(%s), hot_level: %.0f", basePlan->m_qParam.log.c_str(), restaurant->_ID.c_str(), restaurant->_name.c_str(), restaurant->_hot_level);
				++restCnt;
			}
			MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetRestaurant, restaurant size:%d, recommend size:%d, need: %d", basePlan->m_qParam.log.c_str(), cRestList.size(), restCnt, basePlan->m_RestNeedNum);
		}
	}
	return 0;
}

// 选择购物
int DataChecker::GetShop(BasePlan* basePlan) {
	basePlan->m_ShopNeedNum = 0;
	std::tr1::unordered_set<std::string> addSet;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (!(place->_t & LY_PLACE_TYPE_SHOP)) continue;
		if (addSet.find(place->_ID) != addSet.end()) continue;
		++basePlan->m_ShopNeedNum;
		addSet.insert(place->_ID);
	}

	if (basePlan->m_richPlace) {
		int shopRichNum = 0;
		int dayStay = 0;
		for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
			dayStay += basePlan->m_RouteBlockList[i]->_day_limit;
		}
		if (basePlan->m_shopIntensity == SHOP_INTENSITY_LOOK_AROUND) {
			if (dayStay < 2) {
				shopRichNum = 0;
			} else {
				shopRichNum = std::min(static_cast<int>(dayStay / 2), 4);
			}
		} else if (basePlan->m_shopIntensity == SHOP_INTENSITY_SHOPAHOLIC) {
			shopRichNum = dayStay;
		}

		if (shopRichNum > basePlan->m_ShopNeedNum) {
			basePlan->m_ShopNeedNum = shopRichNum;
			std::vector<const LYPlace*> pListShop;
			std::vector<ShowItem> showShopList;
			DataList::GetLYPlaceList(basePlan, LY_PLACE_TYPE_SHOP, pListShop);
			DataList::SortList(basePlan, LY_PLACE_TYPE_SHOP, pListShop, showShopList);

			int shopCnt = 0;
			for (int i = 0; i < showShopList.size(); i++) {
				ShowItem& showItem = showShopList[i];
				const Shop* shop = dynamic_cast<const Shop*>(showItem.GetPlace());
				if (shopCnt >= 2 * shopRichNum) break;
				if (!basePlan->IsVarPlaceOpen(shop)) continue;
				if (addSet.find(shop->_ID) != addSet.end()) continue;
				basePlan->m_waitPlaceList.push_back(shop);
				addSet.insert(shop->_ID);
				MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetShop, shop: %s(%s), _hot_level: %.0f", basePlan->m_qParam.log.c_str(), shop->_ID.c_str(), shop->_name.c_str(), shop->_hot_level);
				++shopCnt;
			}
			MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetShop, shop size: %d, recommend size: %d, need: %d", basePlan->m_qParam.log.c_str(), showShopList.size(), shopCnt, basePlan->m_ShopNeedNum);
		}
	}
	return 0;
}

// 选择景点
int DataChecker::GetView(BasePlan* basePlan) {
	std::tr1::unordered_set<std::string> addSet;
	for (auto place: basePlan->m_userMustPlaceSet) {
		if (!(place->_t & LY_PLACE_TYPE_VIEW)) continue;
		if (addSet.find(place->_ID) != addSet.end()) continue;
		addSet.insert(place->_ID);
	}
	if (basePlan->m_richPlace && basePlan->m_isPlan && (basePlan->m_notPlanCityPark ? !LYConstData::IsParkCity(basePlan->m_City->_ID): true) ) {//后续先判断是不是跳过国家公园的规划 不跳过直接补点 跳过则判断是否为国家公园
		std::vector<const LYPlace*> cityViewList;
		int ret = LYConstData::getViewLocal(basePlan->m_City->_ID, cityViewList, basePlan->m_qParam.ptid);
		if (ret != 0) {
			char buff[100];
			MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetView, Get city view failed", basePlan->m_qParam.log.c_str());
			if (basePlan->m_qParam.lang != "en") {
				snprintf(buff, sizeof(buff), "%s 目前缺失景点，妙计正在补充", basePlan->m_City->_name.c_str());
			} else {
				snprintf(buff, sizeof(buff), "Sorry, %s is currently not on record, Mioji is in the process of making this addition to our system.", basePlan->m_City->_enname.c_str());
			}
			basePlan->m_error.Set(53107, "景点获取失败", std::string(buff));
			return 1;
		}
		basePlan->RemoveNotPlanPlace(cityViewList);
		basePlan->m_CityViewList.assign(cityViewList.begin(), cityViewList.end());

		for (int i = 0; i < cityViewList.size(); i++) {
			const View* view = dynamic_cast<const View*>(cityViewList[i]);
			if (!basePlan->IsVarPlaceOpen(view)) continue;
			if (addSet.find(view->_ID) != addSet.end()) continue;
			basePlan->m_waitPlaceList.push_back(view);
			addSet.insert(view->_ID);
		}
	}

	return 0;
}

int DataChecker::GetTour(BasePlan* basePlan) {
	return 0;
}

int DataChecker::SetHotMap(BasePlan* basePlan) {
	for (int i = 0; i < basePlan->m_waitPlaceList.size(); ++i) {
		int hot = 0;
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(basePlan->m_waitPlaceList[i]);
		hot = vPlace->GetHotLevel();
		//用户必去点+10
		if (basePlan->m_userMustPlaceSet.find(vPlace) != basePlan->m_userMustPlaceSet.end()) {
			hot += 10;
//			hot = std::max(hot, 5);
		}
		// 对私有平台优先推荐数据加分
		// 优先推荐+50
        if (vPlace->_rec_priority == 10 and vPlace->m_ptid == basePlan->m_qParam.ptid) {
            hot += 50;
        }

		//排名前3的景点热度加权，以便路线排序的时候能有优先选中
		// grade 大于 8，小城市无热门景点时就不加权
		// 1st +50
		// 2nd +25
		// 3rd +10
		if (vPlace->_ranking == 1 && vPlace->_grade > 8.0) {
			hot += 50;
		}
		if (vPlace->_ranking == 2 && vPlace->_grade > 8.0) {
			hot += 25;
		}
		if (vPlace->_ranking == 3 && vPlace->_grade > 8.0) {
			hot += 10;
		}

		basePlan->m_hotMap[vPlace->_ID] = hot;
	}

	return 0;
}
// 获取候选点间两两交通
int DataChecker::GetTraffic(BasePlan* basePlan) {
	int ret = 0;

	std::tr1::unordered_set<std::string> idSet;
	for (int i = 0; i < basePlan->GetKeyNum(); ++i) {
		if (basePlan->GetKey(i)->_place == basePlan->m_arvNullPlace 
				|| basePlan->GetKey(i)->_place == basePlan->m_deptNullPlace) continue;
		idSet.insert(basePlan->GetKey(i)->_place->_ID);
	}
	for (int i = 0; i < basePlan->m_waitPlaceList.size(); ++i) {
		idSet.insert(basePlan->m_waitPlaceList[i]->_ID);
	}

	ret = PathTraffic::GetTrafID(basePlan, idSet);
	if (ret != 0) return 1;

	ret = CheckTraf(basePlan, idSet);
	if (ret != 0) return 1;

	ret = GetAvgTraf(basePlan);
	if (ret != 0) return 1;

	return 0;
}


// 选择placeinfo
int DataChecker::CheckInfo(BasePlan* basePlan) {
	int ret = 0;

	PlaceInfoAlloc place_info_alloc;
	ret = place_info_alloc.DoAlloc(basePlan);
	if (ret != 0) return 1;

	return 0;
}


int DataChecker::GetAvgTraf(BasePlan* basePlan) {
	std::vector<const LYPlace*> keyList;
	for (int i = 0; i < basePlan->GetKeyNum(); ++i) {
		const LYPlace* place = basePlan->GetKey(i)->_place;
		if (place && LYConstData::IsRealID(place->_ID)) {
			keyList.push_back(place);
		}
	}
	GetAvgTraf(basePlan, keyList);

	std::vector<const LYPlace*> vPlaceList;
	for (int i = 0; i < basePlan->m_waitPlaceList.size(); ++i) {
		const LYPlace* place = basePlan->m_waitPlaceList[i];
		if (place && LYConstData::IsRealID(place->_ID)) {
			vPlaceList.push_back(place);
		}
	}
	GetAvgTraf(basePlan, vPlaceList);
	return 0;
}

int DataChecker::GetAvgTraf(BasePlan* basePlan, std::vector<const LYPlace*>& placeList) {
	std::tr1::unordered_set<const LYPlace*> placeSet;
	for (int i = 0; i < placeList.size(); ++i) {
		const LYPlace* place = placeList[i];
		if (place && LYConstData::IsRealID(place->_ID)) {
			placeSet.insert(place);
		}
	}
	std::tr1::unordered_set<const LYPlace*> unionSet(placeSet);
	for (int i = 0; i < basePlan->GetKeyNum(); ++i) {
		const LYPlace* place = basePlan->GetKey(i)->_place;
		if (place && LYConstData::IsRealID(place->_ID) && place->_t & LY_PLACE_TYPE_HOTEL) {
			unionSet.insert(place);
		}
	}

	std::vector<const LYPlace*> unionList(unionSet.begin(), unionSet.end());
	std::sort(unionList.begin(), unionList.end(), PlaceIDCmp());
	for (std::vector<const LYPlace*>::iterator it = unionList.begin(); it != unionList.end(); ++it) {
		const LYPlace* pa = *it;
		if (placeSet.find(pa) == placeSet.end()) continue;

		std::vector<std::pair<int, int> > timeDistList;
		int timeSum = 0;
		int distSum = 0;
		int cnt = 0;
		for (std::vector<const LYPlace*>::iterator ii = unionList.begin(); ii != unionList.end(); ++ii) {
			const LYPlace* pb = *ii;
			if (pa == pb) continue;
			const TrafficItem* trafItem = basePlan->GetTraffic(pa->_ID, pb->_ID);
			if (trafItem) {
				timeDistList.push_back(std::pair<int, int>(trafItem->_time, trafItem->_dist));
				timeSum += trafItem->_time;
				distSum += trafItem->_dist;
				++cnt;
			}
		}
		if (cnt > 0) {
			basePlan->m_maxTrafTimeMap[pa->_ID] = timeSum * 1.0 / cnt;
			basePlan->m_maxTrafDistMap[pa->_ID] = distSum * 1.0 / cnt;
		} else {
			basePlan->m_maxTrafTimeMap[pa->_ID] = basePlan->m_ATRTimeCost;
			basePlan->m_maxTrafDistMap[pa->_ID] = basePlan->m_ATRDist;
		}
		std::stable_sort(timeDistList.begin(), timeDistList.end(), sort_pair_second<int, int, std::less<int> >());
		timeSum = 0;
		distSum = 0;
		cnt = 0;
		for (int i = 0; i < timeDistList.size() && i < 4; ++i) {
			timeSum += timeDistList[i].first;
			distSum += timeDistList[i].second;
			++cnt;
		}
		if (cnt > 0) {
			basePlan->m_AvgTrafTimeMap[pa->_ID] = timeSum * 1.0 / cnt;
			basePlan->m_AvgTrafDistMap[pa->_ID] = distSum * 1.0 / cnt;
		} else {
			basePlan->m_AvgTrafTimeMap[pa->_ID] = basePlan->m_ATRTimeCost;
			basePlan->m_AvgTrafDistMap[pa->_ID] = basePlan->m_ATRDist;
		}
		MJ::PrintInfo::PrintDbg("[%s]DataChecker::GetAvgTraf, avg_traf: %s-->([%d, %d], %d)", basePlan->m_qParam.log.c_str(), pa->_ID.c_str(), basePlan->m_AvgTrafDistMap[pa->_ID], basePlan->m_maxTrafDistMap[pa->_ID], basePlan->m_AvgTrafTimeMap[pa->_ID]);
	}
	return 0;
}

int DataChecker::CheckTraf(BasePlan* basePlan, std::tr1::unordered_set<std::string>& idSet) {
	char buff[256];
	std::tr1::unordered_set<std::string> delSet;
	// 1 数据缺失
	for (std::tr1::unordered_set<std::string>::iterator it = idSet.begin(); it != idSet.end(); ++it) {
		std::string ida = *it;
		const LYPlace* placeA = basePlan->GetLYPlace(ida);
		if (!placeA) continue;
		if (placeA->_t & LY_PLACE_TYPE_TOURALL) {
			const Tour* tour = dynamic_cast<const Tour*>(placeA);
			//if (tour->m_deptPoi == "HOTEL") continue;
		}
		bool isLack = false;
		for (std::tr1::unordered_set<std::string>::iterator ii = idSet.begin(); ii != idSet.end(); ++ii) {
			if (it == ii) continue;
			std::string idb = *ii;
			const LYPlace* placeB = basePlan->GetLYPlace(idb);
			if (!placeB) continue;
			if (placeB->_t & LY_PLACE_TYPE_TOURALL) {
				const Tour* tour = dynamic_cast<const Tour*>(placeB);
				//if (tour->m_arvPoi == "HOTEL") continue;
			}
			const TrafficItem* trafItem = basePlan->GetTraffic(ida, idb);
			if (!trafItem) {
				snprintf(buff, sizeof(buff), "交通数据缺失: %s|%s", ida.c_str(), idb.c_str());
				MJ::PrintInfo::PrintErr("[%s]DataChecker::CheckTraf, %s", basePlan->m_qParam.log.c_str(), buff);
				isLack = true;
				break;
			}
		}
		if (isLack && delSet.find(ida) == delSet.end()) {
			const LYPlace* place = basePlan->GetLYPlace(ida);
			int ret = basePlan->DelPlace(place);
			if (ret != 0) {
				basePlan->m_error.Set(55103, std::string(buff));
				return 1;
			} else {
				delSet.insert(ida);
			}
		}
	}

	// 2 数据异常
	// 2.0 获取基准点
	const LYPlace* corePlace = NULL;
	const LYPlace* firstHotel = NULL;
	const LYPlace* firstArrive = NULL;
	for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
		const RouteBlock* routeBlock = basePlan->m_RouteBlockList[i];
		if (routeBlock && !routeBlock->_hotel_list.empty() && firstHotel == NULL) {
			firstHotel = routeBlock->_hotel_list[0];
		}
		if (routeBlock && routeBlock->_arrive_place && firstArrive == NULL) {
			firstArrive = routeBlock->_arrive_place;
		}
	}
	if (firstHotel) {
		corePlace = firstHotel;
	} else {
		corePlace = firstArrive;
	}
	if (corePlace == NULL) {
		basePlan->m_error.Set(55102, "未收录arrviePlace信息");
		return 1;
	}

	// 2.1 检查异常
	for (std::tr1::unordered_map<std::string, TrafficItem*>::iterator it = basePlan->m_TrafficMap.begin(); it != basePlan->m_TrafficMap.end(); ++it) {
		std::string tKey = it->first;
		const TrafficItem* trafItem = it->second;
		if (!IsBadTraf(trafItem)) continue;

		snprintf(buff, sizeof(buff), "交通数据异常: %s, time: %s, dist: %d", tKey.c_str(), ToolFunc::NormSeconds(trafItem->_time).c_str(), trafItem->_dist);
		MJ::PrintInfo::PrintErr("[%s]DataChecker::CheckTraf, %s", basePlan->m_qParam.log.c_str(), buff);

		std::string::size_type pos = tKey.find("_");
		if (pos == std::string::npos) {
			pos = tKey.find("|");
		}
		if (pos == std::string::npos) continue;

		std::string ida = tKey.substr(0, pos);
		std::string idb = tKey.substr(pos + 1);
		const LYPlace* pa = basePlan->GetLYPlace(ida);
		const LYPlace* pb = basePlan->GetLYPlace(idb);

		const TrafficItem* a2core = basePlan->GetTraffic(ida, corePlace->_ID);
		if (delSet.find(ida) == delSet.end() && a2core && IsBadTraf(a2core)) {
			int ret = basePlan->DelPlace(pa);
			if (ret != 0) {
				basePlan->m_error.Set(55105, std::string(buff));
				return 1;
			} else {
				delSet.insert(ida);
			}
		}
		const TrafficItem* b2core = basePlan->GetTraffic(idb, corePlace->_ID);
		if (delSet.find(idb) == delSet.end() && b2core && IsBadTraf(b2core)) {
			int ret = basePlan->DelPlace(pb);
			if (ret != 0) {
				basePlan->m_error.Set(55105, std::string(buff));
				return 1;
			} else {
				delSet.insert(idb);
			}
		}
	}
	return 0;
}

//修改了必去点的排序规则
//    必去点加入了推荐点逻辑
template <typename T>
int DataChecker::SortPlaceList(const T& t, BasePlan* basePlan, std::vector<const LYPlace*>& sortList, bool isShopSortWithView) {
	std::vector<const View*> viewList;
	std::vector<const Shop*> shopList;
	std::vector<const Restaurant*> restList;
	std::vector<const Tour*> tourList;
	std::vector<const LYPlace*> otherList;
	std::vector<const LYPlace*> recList; //yc 优先推荐列表
	std::vector<const LYPlace*> shopAndViewList; //购物和景点混排
	for (auto place: t) {
		if (place->_rec_priority == 10) {
			basePlan->m_userOptSet.insert(place);
			recList.push_back(place);
		} else if (place->_t & LY_PLACE_TYPE_VIEW) {
			viewList.push_back(dynamic_cast<const View*>(place));
			shopAndViewList.push_back(place);
		} else if (place->_t & LY_PLACE_TYPE_SHOP) {
			shopList.push_back(dynamic_cast<const Shop*>(place));
			shopAndViewList.push_back(place);
		} else if (place->_t & LY_PLACE_TYPE_RESTAURANT) {
			restList.push_back(dynamic_cast<const Restaurant*>(place));
		} else if (place->_t & LY_PLACE_TYPE_TOURALL) {
			tourList.push_back(dynamic_cast<const Tour*>(place));
		} else {
			otherList.push_back(place);
		}
	}
	std::stable_sort(recList.begin(), recList.end(), varPlaceCmp()); //yc
	sortList.insert(sortList.end(), recList.begin(), recList.end()); //yc

	if (isShopSortWithView) {
		std::stable_sort(shopAndViewList.begin(), shopAndViewList.end(), varPlaceCmp());
		sortList.insert(sortList.end(), shopAndViewList.begin(), shopAndViewList.end());
	} else {
		std::stable_sort(viewList.begin(), viewList.end(), varPlaceCmp());
		std::stable_sort(shopList.begin(), shopList.end(), varPlaceCmp());
		sortList.insert(sortList.end(), viewList.begin(), viewList.end());
		sortList.insert(sortList.end(), shopList.begin(), shopList.end());
	}

	std::stable_sort(restList.begin(), restList.end(), varPlaceCmp());
	sortList.insert(sortList.end(), restList.begin(), restList.end());

	std::stable_sort(tourList.begin(), tourList.end(), varPlaceCmp());
	sortList.insert(sortList.end(), tourList.begin(), tourList.end());
	return 0;

}

//对waitPlaceList中的点排序: 先排m_userMustPlaceSet 加入到waitList, 然后将其余的点排序，再置于其后
int DataChecker::SortAllPlace(BasePlan* basePlan) {
	std::vector<const LYPlace*> userMustPlaceList;
	SortPlaceList(basePlan->m_userMustPlaceSet, basePlan, userMustPlaceList, false);

	std::vector<const LYPlace*> sortList;
	SortPlaceList(basePlan->m_waitPlaceList, basePlan, sortList, true);
	basePlan->m_waitPlaceList.clear();
	basePlan->m_waitPlaceList = userMustPlaceList;
	basePlan->m_waitPlaceList.insert(basePlan->m_waitPlaceList.end(), sortList.begin(), sortList.end());
	if (basePlan->m_waitPlaceList.size() > 100) {
		MJ::PrintInfo::PrintLog("[%s]DataChecker::SortAllPlace, wait Place size %d", basePlan->m_qParam.log.c_str(), basePlan->m_waitPlaceList.size());
		int delPos = 100;
		if (delPos < basePlan->m_userMustPlaceSet.size()) {
			delPos = basePlan->m_userMustPlaceSet.size();
		}
		basePlan->m_waitPlaceList.erase(basePlan->m_waitPlaceList.begin() + delPos, basePlan->m_waitPlaceList.end());
	}
	return 0;
}


bool DataChecker::IsBadTraf(const TrafficItem* trafItem) {
	if (trafItem->_time > 6 * 3600 || trafItem->_time < 0) return true;
	return false;
}

int DataChecker::CalMiss(BasePlan* basePlan) {
	//misslevel
	//必去点	1000
	for (auto place: basePlan->m_userMustPlaceSet) {
		basePlan->m_missLevelMap[place] = 1000;
	}
	return 0;
}

