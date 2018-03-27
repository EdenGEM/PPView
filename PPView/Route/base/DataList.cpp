#include <iostream>
#include "Route/vlist/ViewRankProcessor.h"
#include "Route/vlist/ShopRankProcessor.h"
#include "Route/vlist/RestaurantRankProcessor.h"
#include "Route/vlist/HotelRankProcessor.h"
#include "Route/vlist/ViewShopRankProcessor.h"
#include "DataList.h"

class TourCmp {
private:
	BasePlan* m_basePlan;
	//按照价格排序
	bool CmpOfPrice(const Tour* plhs, const Tour* prhs) const {
		const TicketsFun* lticket = NULL;
		const TicketsFun* rticket = NULL;
		float lPrice = 0;
		float rPrice = 0;
		LYConstData::GetBottomTicketAndPrice(plhs, lticket, lPrice);
		LYConstData::GetBottomTicketAndPrice(prhs, rticket, rPrice);

		if (lticket == NULL && rticket == NULL) {
			return false;
		} else if (lticket == NULL && rticket != NULL) {
			return false;
		} else if (lticket != NULL && rticket == NULL) {
			return true;
		} else {
			if (lticket->m_ccy != "CNY") {
				lPrice = ToolFunc::RateExchange::curConv(lPrice, lticket->m_ccy);
			}
			if (rticket->m_ccy != "CNY") {
				rPrice = ToolFunc::RateExchange::curConv(rPrice, rticket->m_ccy);
			}
			if (m_basePlan->m_sortMode == 2) {
				return lPrice > rPrice;
			} else {
				return lPrice < rPrice;
			}
		}
	}
	//按照日期排序
	int CmpOfDate(const Tour* plhs, const Tour* prhs) const {
		int lflag = 0, rflag = 0;
		std::string out;
		if (LYConstData::IsTourAvailable(plhs, m_basePlan->m_listDate)) {
			lflag = 1;
		}
		if (LYConstData::IsTourAvailable(prhs, m_basePlan->m_listDate)) {
			rflag = 1;
		}
		if (lflag > rflag) {
			return 1;
		} else if (lflag < rflag) {
			return -1;
		} else {
			return 0;
		}
	}
public:
	bool operator() (const LYPlace* plhs, const LYPlace* prhs) const {
		const Tour* ltour = dynamic_cast<const Tour*>(plhs);
		const Tour* rtour = dynamic_cast<const Tour*>(prhs);
		if (ltour != NULL && rtour == NULL) {
			return true;
		} else if (ltour == NULL && rtour != NULL) {
			return false;
		} else if (ltour == NULL && rtour == NULL) {
			return false;
		} else if (ltour != NULL && rtour != NULL) {
			//如果是默认排序，则先按照日期排序
			if (m_basePlan->m_sortMode == 0) {
				int ret = CmpOfDate(ltour, rtour);
				if (ret) {
					return ret > 0;
				}
			}
			return CmpOfPrice(ltour, rtour);
		}
	}
	TourCmp(BasePlan *basePlan):m_basePlan(basePlan) {}
};
//add by shyy
int DataList::SortListTourDatePrice(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList) {
	TourCmp tourCmp(basePlan);
	std::stable_sort(pList.begin(), pList.end(), tourCmp);

	for (int i = 0; i < pList.size(); i++) {
		std::vector<const TicketsFun* > tickets;
		LYConstData::GetProdTicketsListByPlace(pList[i],tickets);
		if(tickets.size()>0)
		{
			const VarPlace* vPlace = dynamic_cast<const VarPlace*>(pList[i]);
			showItemList.push_back(ShowItem(pList[i], vPlace->_ranking));
		}
	}
	return 0;
}
//shyy end
int DataList::SortListRanking(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList) {
	std::sort(pList.begin(), pList.end(), VPlaceGradeCmp());
	for (int i = 0; i < pList.size(); i++) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(pList[i]);
		if (vPlace == NULL) continue;
		showItemList.push_back(ShowItem(pList[i], vPlace->_ranking));
	}
	return 0;
}

int DataList::SortListHot(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList) {
	std::sort(pList.begin(), pList.end(), VPlaceHotCmp());
	for (int i = 0; i < pList.size(); i++) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(pList[i]);
		//double score = 1 - i * 1.0 / pList.size();
		showItemList.push_back(ShowItem(pList[i], vPlace->_ranking));
	}
	return 0;
}

int DataList::SortListTour(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList) {
//score具体算法还不清楚，先按照景点rank排序进行算的
	std::sort(pList.begin(), pList.end(), varPlaceCmp());
	for (int i = 0; i < pList.size(); i++) {
		const VarPlace* vPlace = dynamic_cast<const VarPlace*>(pList[i]);
		double score = 1 - i * 1.0 / pList.size();
		showItemList.push_back(ShowItem(pList[i], score));
	}
	return 0;
}

//仅用于补充购物点 （购物狂）
int DataList::SortList(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList)  {
	//4 转换LYPlace -> ScoreNode
	std::vector < const ScoreNode* > scoreNodeList;
	int ret = 0;
	ret = GetSNodeList(reqMode, pList, scoreNodeList);
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s][GetSNodeList] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
		basePlan->m_error.Set(55013,  "sortnode list error !");
		return ret;
	}
	//5 计算距离分数
	if (reqMode != LY_PLACE_TYPE_RESTAURANT) {
		const LYPlace* center = NULL;

		if (basePlan->m_userMustPlaceSet.size() != 0) {
			center = new LYPlace;
			std::vector<const LYPlace*> userMustPlaceList;
			for (auto place: basePlan->m_userMustPlaceSet) {
				userMustPlaceList.push_back(place);
			}
			ret = CalSphereDistCenter(userMustPlaceList, center);
			if (ret) {
				MJ::PrintInfo::PrintErr("[%s][CalSphereDistCenter] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
				basePlan->m_error.Set(55012,  "距离计算异常");
				return ret;
			}
			ret = CalDistScoreCenter(scoreNodeList, center);
			if (ret) {
				MJ::PrintInfo::PrintErr("[%s][CalDistScore] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
				basePlan->m_error.Set(55012,  "距离计算异常");
				return ret;
			}
			delete center;
			center = NULL;
		}
	}
	//6 计算热度分数
	ret = CalHotScore(scoreNodeList);
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s][CalHotScore] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
			basePlan->m_error.Set(55014, "热度计算异常");
		return ret;
	}
	//6.2 计算价格分数
	if (reqMode == LY_PLACE_TYPE_RESTAURANT) {
		ret = CalPriceScore(scoreNodeList);
		if (ret) {
			basePlan->m_error.Set(55016, "餐馆价格计算异常");
			MJ::PrintInfo::PrintErr("[%s][CalPriceScore] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
			return ret;
		}
	}
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		MJ::PrintInfo::PrintDbg("[%s]name %s, id %s, hot = %.2lf, dist = %.2lf, level = %.2lf, price = %.2lf", basePlan->m_qParam.log.c_str(), scoreNodeList[i]->m_lp->_name.c_str(),
		scoreNodeList[i]->m_lp->_ID.c_str(), scoreNodeList[i]->m_hotScore, scoreNodeList[i]->m_distScore, scoreNodeList[i]->m_levelScore, scoreNodeList[i]->m_priceScore);
	}
	//7 排序
	ret = PlaceRanker(reqMode, basePlan->m_sortMode, scoreNodeList, showItemList);
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s][PlaceRanker] error ID = %d", basePlan->m_qParam.log.c_str(), ret);
		basePlan->m_error.Set(55010, "排序异常");
		return ret;
	}
	return 0;
}

int DataList::GetSNodeList(int nodeType, std::vector<const LYPlace*>& pList, std::vector<const ScoreNode*>& scoreNodeList) {
	const ScoreNode* snode = NULL;
	for (int i = 0; i < pList.size(); ++i) {
		snode = DataList::GetSNode(nodeType, pList[i]);
		if (snode == NULL) {
			MJ::PrintInfo::PrintErr("DataList::GetSNodeList, error can`t creat new node");
			return 1;
		}
		scoreNodeList.push_back(snode);
	}
	return 0;
}

ScoreNode* DataList::GetSNode(int type, const LYPlace* place) {
	if (LY_PLACE_TYPE_VIEWSHOP == type) {
		return (ScoreNode*) (new ScoreNodeVS(place));
	} else if (LY_PLACE_TYPE_SHOP == type) {
		return (ScoreNode*) (new ScoreNodeS(place));
	} else if (LY_PLACE_TYPE_HOTEL == type) {
		return (ScoreNode*) (new ScoreNodeH(place));
	} else if (LY_PLACE_TYPE_RESTAURANT == type) {
		return (ScoreNode*) (new ScoreNodeR(place));
	} else if (LY_PLACE_TYPE_VIEW == type) {
		return (ScoreNode*) (new ScoreNodeV(place));
	}
	return NULL;
}

int DataList::CalDistScoreCenter(std::vector<const ScoreNode*>& scoreNodeList, const LYPlace* &center) {
	std::vector<double> distList;
	double maxDist = 3000;
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		double dist = LYConstData::CaluateSphereDist(scoreNodeList[i]->m_lp, center);
		distList.push_back(dist);
		if (dist > maxDist) {
			maxDist = dist;
		}
	}
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		double dist = distList[i];
		double distN = 0;
		if (maxDist > 0) {
			distN = std::min(3 * (dist - (maxDist + 0) / 2.0) / ((maxDist - 0) / 2.0), 700.0);
		}
		double distS = 1.0 / (1 + exp(distN));
		((ScoreNode*)scoreNodeList[i])->m_distScore = distS;
		MJ::PrintInfo::PrintDbg("DataList::CalDistScore, %s(%s), dist = %.0f, distScore = %.2f", scoreNodeList[i]->m_lp->_ID.c_str(), scoreNodeList[i]->m_lp->_name.c_str(), dist, distS);
	}
	return 0;
}

int DataList::CalDistScoreNear(std::vector<const ScoreNode*>& scoreNodeList, std::vector<const LYPlace*>& nearPlaceList) {
	if (nearPlaceList.empty()) return 0;
	std::vector<double> distList;
	double maxDist = 3000;
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		double dist = 0;
		for (int j = 0; j < nearPlaceList.size(); ++j) {
			dist += LYConstData::CaluateSphereDist(nearPlaceList[j], scoreNodeList[i]->m_lp);
		}
		dist /= nearPlaceList.size();
		distList.push_back(dist);
		if (dist > maxDist) {
			maxDist = dist;
		}
	}
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		double dist = distList[i];
		double distN = 0;
		if (maxDist > 0) {
			distN = std::min(3 * (dist - (maxDist + 0) / 2.0) / ((maxDist - 0) / 2.0), 700.0);
		}
		double distS = 1.0 / (1 + exp(distN));
		((ScoreNode*)scoreNodeList[i])->m_distScore = distS;
		MJ::PrintInfo::PrintDbg("DataList::CalDistScore, %s(%s), dist = %.0f, distScore = %.2f", scoreNodeList[i]->m_lp->_ID.c_str(), scoreNodeList[i]->m_lp->_name.c_str(), dist, distS);
	}
	return 0;
}

int DataList::CalSphereDistCenter(std::vector < const LYPlace*>& userPlaceList , const LYPlace* center) {
	double lon = 0;
	double lat = 0;
	int num = 0;
	for (std::vector <const LYPlace*>::iterator ii = userPlaceList.begin() ; ii != userPlaceList.end();) {
		std::string::size_type pos = (*ii)->_poi.find(",");
		if (pos == std::string::npos)  {
			ii = userPlaceList.erase(ii);
			continue;
		}
		double lon_a = atof((*ii)->_poi.substr(0, pos).c_str());
		double lat_a = atof((*ii)->_poi.substr(pos + 1).c_str());
		lon += lon_a;
		lat += lat_a;
		ii++;
		num++;
	}
	if (num > 0) {
		lon /= num;
		lat /= num;
	}
	char buff[1000] = {0};
	snprintf(buff, sizeof(buff), "%.10lf,%.10lf", lon, lat);
	((LYPlace*)center)->_poi = buff;
	return 0;
}

int DataList::CalHotScore(std::vector <const ScoreNode*>& scoreNodeList) {
	for ( int i = 0 ; i < scoreNodeList.size() ; ++ i ) {
		const VarPlace*  vp = dynamic_cast<const VarPlace*>(scoreNodeList[i]->m_lp);
		double hot = 0;
		if (vp != NULL){
			//临时grade 后改回
			if (vp->_t == LY_PLACE_TYPE_RESTAURANT) {
				hot = vp->_grade * 10;
			} else {
				hot = vp->_hot_level;
			}
		} else {
			hot = 0;
		}
		((ScoreNode*)scoreNodeList[i])->m_hotScore = hot;
	}
	return 0;
}

int DataList::CalPriceScore(std::vector<const ScoreNode*>& scoreNodeList) {
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		const VarPlace* vp = dynamic_cast<const VarPlace*>(scoreNodeList[i]->m_lp);
		if (vp != NULL) {
			((ScoreNode*)scoreNodeList[i])->m_priceScore = log10(vp->avgPrice > 1 ? vp->avgPrice : 1);
		} else {
			((ScoreNode*)scoreNodeList[i])->m_priceScore = 0;
		}
	}
	return 0;
}

int DataList::CalPriceScore(std::vector < const ScoreNode* >& scoreNodeList, std::vector < const HotelInfo* >& hotelInfoList) {
	std::tr1::unordered_map < const LYPlace* , const HotelInfo* > lyPlaceToHInfo;
	for (int i = 0; i < hotelInfoList.size(); ++i) {
		lyPlaceToHInfo[hotelInfoList[i]->m_vPlace] = hotelInfoList[i];
	}
	double avgPrice = 0;
	for (int i = 0; i < scoreNodeList.size(); ++i) {
		std::tr1::unordered_map < const LYPlace* , const HotelInfo* >::iterator it;
		if ((it = lyPlaceToHInfo.find(scoreNodeList[i]->m_lp)) != lyPlaceToHInfo.end()) {
			avgPrice += (it->second)->m_cost;
		}
	}
	if (scoreNodeList.size() > 0) {
		avgPrice /= scoreNodeList.size();
	}

	for (int i = 0; i < scoreNodeList.size(); ++i) {
		std::tr1::unordered_map < const LYPlace* , const HotelInfo* >::iterator it;
		if ((it = lyPlaceToHInfo.find(scoreNodeList[i]->m_lp)) != lyPlaceToHInfo.end() && (it->second)->m_cost > 0) {
			((ScoreNode*)scoreNodeList[i])->m_priceScore = log10((it->second)->m_cost > 1 ? (it->second)->m_cost : 1);
		} else {
			((ScoreNode*)scoreNodeList[i])->m_priceScore = 0;
		}
	}
	return 0;
}

int DataList::PlaceRanker(int type, int rankType, std::vector<const ScoreNode*>& scoreNodeList, std::vector<ShowItem>& showItemList) {
	std::vector <void* > voidList;
	for (int i = 0; i < scoreNodeList.size(); i++) {
		voidList.push_back((void*)(scoreNodeList[i]));
	}
	RankProcessor* rankProecesser = NULL;
	if (LY_PLACE_TYPE_VIEWSHOP == type) {
		rankProecesser= new ViewShopRankProcessor();
	} else if (LY_PLACE_TYPE_SHOP == type) {
		rankProecesser= new ShopRankProcessor();
	} else if (LY_PLACE_TYPE_HOTEL == type) {
		rankProecesser= new HotelRankProcessor();
	} else if (LY_PLACE_TYPE_RESTAURANT == type) {
		rankProecesser= new RestaurantRankProcessor();
	} else if (LY_PLACE_TYPE_VIEW == type) {
		rankProecesser= new ViewRankProcessor();
	}
	rankProecesser->Init(rankType,voidList);
	voidList.clear();
	FiltCons* filtCons = new FiltCons();
	rankProecesser->DoRank(voidList,(*filtCons), false);
	delete filtCons;
	delete rankProecesser;
	for (int i = 0; i < voidList.size(); ++i) {
		ScoreNode* sNode = static_cast<ScoreNode*>(voidList[i]);
		if (sNode == NULL || sNode->m_lp == NULL) {
			MJ::PrintInfo::PrintErr("[PlaceRanker] place lost voidlist[%d] ",i);
			continue;
		}
		showItemList.push_back(ShowItem(sNode->m_lp, sNode->m_finalScore));
		delete sNode;
	}
	return 0;
}

int DataList::GetLYPlaceList(BasePlan* basePlan,int placeType,  std::vector<const LYPlace*>& pList) {
	int ret = 0;
	//std::cerr << "cityList:" << std::endl;
	//for (int i = 0; i < basePlan->m_listCity.size(); ++i) {
	//	std::cerr << basePlan->m_listCity[i] << std::endl;
	//}
	for (int i = 0; i < basePlan->m_listCity.size(); ++i) {
		ret = GetCityPlace(basePlan->m_listCity[i], placeType, pList, basePlan->m_qParam.ptid, basePlan->m_error);
	}

	if (ret) {
		return ret;
	}
	//2 移除多城市重复景点
	ret = basePlan->RemoveNotPlanPlace(pList);
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s]DataList::GetLYPlaceList, error  = %d", basePlan->m_qParam.log.c_str(),ret);
		return ret;
	}

	return 0;
}

int DataList::GetCityPlace(std::string cityID, int type, std::vector<const LYPlace*>& pList, const std::string& ptid, ErrorInfo& errorInfo ) {
	int ret = 0;
	//MJ::PrintInfo::PrintDbg("[GetCityPlace] type = %d ", type);
	std::vector<const LYPlace*> thePlaceList;
	if (type & LY_PLACE_TYPE_VIEW) {
		ret = LYConstData::getViewLocal(cityID, thePlaceList, ptid);
		pList.insert(pList.end(), thePlaceList.begin(), thePlaceList.end());
	}
	if (type & LY_PLACE_TYPE_SHOP) {
		ret = LYConstData::getShopLocal(cityID, thePlaceList, ptid);
		pList.insert(pList.end(), thePlaceList.begin(), thePlaceList.end());
	}
	if (type & LY_PLACE_TYPE_RESTAURANT) {
		ret = LYConstData::getRestaurantLocal(cityID, thePlaceList, ptid);
		pList.insert(pList.end(), thePlaceList.begin(), thePlaceList.end());
	}
	return 0;
}

int DataList::GetTourList(BasePlan* basePlan,int placeType,  std::vector<const LYPlace*>& pList) {
	int ret = 0;
	//1 取出所有景点
	for (int i = 0; i < basePlan->m_listCity.size(); ++i) {
		GetTour(basePlan->m_listCity[i], placeType, pList, basePlan->m_qParam.ptid, basePlan->m_error);
	}
	if (ret) {
//		MJ::PrintInfo::PrintErr("[%s]DataList::GetLYPlaceList, cityId %s, error ID = %d", basePlan->m_qParam.log.c_str(), basePlan->m_City->_ID.c_str(), ret);
		return ret;
	}
	//for (int i = 0 ; i < pList.size(); i ++) {
	//	MJ::PrintInfo::PrintLog("GetLYPlaceList,  %d, id %s(%s), ptid %s, ", i, pList[i]->_ID.c_str(), pList[i]->_name.c_str(), pList[i]->m_ptid.c_str());
	//}
	//2 移除多城市重复景点
	//ret = basePlan->RemoveNotPlanPlace(pList);
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s]DataList::GetLYPlaceList, error  = %d", basePlan->m_qParam.log.c_str(),ret);
		return ret;
	}

	return 0;
}

int DataList::GetTour(const std::string &cid, int type, std::vector<const LYPlace*>& pList, const std::string& ptid, ErrorInfo& errorInfo ) {
	int ret = 0;
	//MJ::PrintInfo::PrintDbg("[GetCityPlace] type = %d ", type);
	std::vector<const LYPlace*> thePlaceList;
	ret = LYConstData::GetTourListByType(cid, pList, type, ptid);
	return 0;
}

