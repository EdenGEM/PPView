#include <iostream>
#include "Route/base/ReqParser.h"
#include "Route/base/TrafficPair.h"
#include "Route/base/KeyNodeBuilder.h"
#include "Route/base/DataList.h"
#include "Route/base/DataChecker.h"
#include "Route/base/PathTraffic.h"
#include "PostProcessor.h"
#include "Processor.h"

int Processor::Process(const QueryParam& param, Json::Value& req, Json::Value& resp) {
	BasePlan* basePlan = new BasePlan;
	MJ::MyTimer t;
	MJ::MyTimer total;
	int ret = 0;
	if (ret == 0) {
		t.start();
		ret = ReqParser::DoParse(param, req, basePlan);
		basePlan->m_cost.m_reqParse = t.cost();
	}
	if (basePlan->m_City == NULL && basePlan->m_key == "") {
		MJ::PrintInfo::PrintErr("[Process]can`t GetCityPlace (city = NULL)!");
		ret = 1;
	}
	if (ret == 0) {
		t.start();
		if (basePlan->m_qParam.type == "p201") {
			ret = DoProcessP201(basePlan);
		} else if (basePlan->m_qParam.type == "p104") {
			ret = DoProcessP104(basePlan);
		} else if (basePlan->m_qParam.type == "p105") {
			ret = DoProcessP105(basePlan);
		} else if (basePlan->m_qParam.type == "p101"
				|| basePlan->m_qParam.type == "p202") {
			ret = DoProcessP202(basePlan);
		} else if (basePlan->m_qParam.type == "b116") {
			ret = DoProcessB116(basePlan);
		}
		basePlan->m_pCost.m_processor = t.cost();
	}

	if (ret == 0) {
		t.start();
		ret = PostProcessor::PostProcess(req, basePlan, resp);
		basePlan->m_cost.m_postProcess = t.cost();
		if (ret != 0) {
			MJ::PrintInfo::PrintErr("[%s][Postprocess] error!!", basePlan->m_qParam.log.c_str());
		}
	} else {
		MJ::PrintInfo::PrintErr("[%s][Processer] error!!", basePlan->m_qParam.log.c_str());
	}

	basePlan->m_cost.m_total = total.cost();
	Json::Value jLog;
	LogDump::Dump(basePlan->m_qParam, basePlan->m_error, 1, basePlan->m_cost, basePlan->m_pCost, basePlan->m_stat, basePlan->m_pStat, jLog);
	ErrDump::Dump(basePlan->m_qParam, basePlan->m_error, req, resp);

	if (basePlan->m_useKpi) {
		resp["kpi"]["logdump"] = jLog;
		//		resp["kpi"]["planStats"] = jStats;
	}

	if (basePlan) {
		delete basePlan;
		basePlan = NULL;
	}
	return ret;
}


int Processor::DoProcessP104(BasePlan* basePlan) {
	int ret = 0;

	std::vector<const LYPlace*> pList;
	std::vector<ShowItem> showItems;
	if (basePlan->m_maxDist == -1) {
		if (basePlan->m_key != "") {
			const LYPlace* place = LYConstData::GetLYPlace(basePlan->m_key, basePlan->m_qParam.ptid);
			if(place) pList.push_back(place);
		} else {
			ret = DataList::GetTourList(basePlan, basePlan->m_reqMode, pList);
		}
	} else {
		ret = FilterDist(basePlan, pList, basePlan->m_maxDist);
	}
	if (ret != 0) return 1;
	ret = FilterPrivate(basePlan->m_qParam, pList, basePlan->m_privateFilter);
	if (ret != 0) return 1;
	ret = FilterTag(pList, basePlan->m_filterTags);
	if (ret != 0) return 1;
	ret = FilterUtime(basePlan, pList, basePlan->m_utime);
	if (ret != 0) return 1;
	ret = FilterId(basePlan->m_qParam, pList, basePlan->m_key);
	if (ret != 0) return 1;
	ret = DataList::SortListTourDatePrice(basePlan, LY_PLACE_TYPE_TOURALL, pList, showItems);
	if (ret != 0) return 1;
	for (int i = 0; i < showItems.size(); ++i) {
		const Tour *tour = dynamic_cast<const Tour*>(showItems[i].GetPlace());
		if (tour) {
			basePlan->m_showItemList.push_back(showItems[i]);
		}
	}
	return 0;
}

int Processor::DoProcessP105(BasePlan* basePlan) {
	return 0;
}

int Processor::DoProcessP202(BasePlan* basePlan) {
	MJ::MyTimer timer;
	timer.start();

	int ret = 0;

	std::vector<ShowItem> showViewList;
	std::vector<ShowItem> showShopList;
	std::vector<ShowItem> showRestList;

	std::map<int,std::vector<ShowItem>* > _loopParams;
	_loopParams[LY_PLACE_TYPE_VIEW]=&showViewList;
	_loopParams[LY_PLACE_TYPE_SHOP]=&showShopList;
	_loopParams[LY_PLACE_TYPE_RESTAURANT]=&showRestList;
	auto tags = basePlan->m_filterTags;
	for(auto it=_loopParams.begin();it!=_loopParams.end();it++)
	{
		const int type = it->first;
		auto resV = it->second;
		std::vector<const LYPlace*> pList;

		if (basePlan->m_reqMode & type) {
			int maxDist = basePlan->m_maxDist;
			if (maxDist == -1) {
				if (basePlan->m_key != "") {
					const LYPlace* place = LYConstData::GetLYPlace(basePlan->m_key, basePlan->m_qParam.ptid);
					if(place) pList.push_back(place);
				} else {
					ret = DataList::GetLYPlaceList(basePlan, type, pList);
				}
			} else {
				ret = FilterDist(basePlan, pList, maxDist);
			}

			if (ret != 0) return 1;
			ret = FilterId(basePlan->m_qParam, pList, basePlan->m_key);
			if (ret != 0) return 1;
			ret = FilterPtid(basePlan->m_qParam, pList, basePlan->m_qParam.ptid, basePlan->m_privateFilter);
			if (ret != 0) return 1;
			ret = FilterTag(pList, tags);
			if (ret != 0) return 1;
			ret = FilterUtime(basePlan, pList, basePlan->m_utime);
			if (ret != 0) return 1;

			ret = DataList::SortListRanking(basePlan, LY_PLACE_TYPE_VIEW, pList, *resV);
			if (ret != 0) return 1;
		}
		basePlan->m_pStat.m_viewNum = showViewList.size();
		basePlan->m_pStat.m_shopNum = showShopList.size();
		basePlan->m_pStat.m_restNum = showRestList.size();
	}

	std::vector<std::vector<ShowItem> > showList;
	showList.push_back(showViewList);
	showList.push_back(showShopList);
	showList.push_back(showRestList);
	std::vector<int> maxNum;
	std::vector<int> perNum = std::vector<int>(4,2);
	maxNum.push_back(showViewList.size());
	maxNum.push_back(showShopList.size());
	maxNum.push_back(showRestList.size());
	int topNum = showViewList.size() + showShopList.size() + showRestList.size();
	ret = ListResortViewShopRestTour(showList, perNum, maxNum, topNum, basePlan->m_showItemList);
	if (ret != 0) return 1;
	int t = timer.cost();
	std::cerr << "zhangyang p202 cost " <<  t << std::endl;
	return 0;
}

int Processor::DoProcessP201(BasePlan* basePlan) {
	MJ::MyTimer timer;
	timer.start();

	int ret = 0;

	std::vector<ShowItem> showViewList;
	std::vector<ShowItem> showShopList;
	std::vector<ShowItem> showRestList;
	std::vector<ShowItem> showViewTicketList;
	std::vector<ShowItem> showPlayList;
	std::vector<ShowItem> showActList;

	std::map<int,std::vector<ShowItem>* > _loopParams;
	_loopParams[LY_PLACE_TYPE_VIEW]=&showViewList;
	_loopParams[LY_PLACE_TYPE_SHOP]=&showShopList;
	_loopParams[LY_PLACE_TYPE_RESTAURANT]=&showRestList;
	_loopParams[LY_PLACE_TYPE_VIEWTICKET]=&showViewTicketList;
	_loopParams[LY_PLACE_TYPE_PLAY]=&showPlayList;
	_loopParams[LY_PLACE_TYPE_ACTIVITY]=&showActList;
	for(auto it=_loopParams.begin();it!=_loopParams.end();it++) {
		const int type = it->first;
		auto resV = it->second;
		std::vector<const LYPlace*> pList;
		if (type & LY_PLACE_TYPE_TOURALL) {
			ret = DataList::GetTourList(basePlan, type, pList);
		}
		else
			ret = DataList::GetLYPlaceList(basePlan, type, pList);
		if (ret != 0) return 1;
		ret = DataList::SortListRanking(basePlan, LY_PLACE_TYPE_VIEW, pList, *resV);
		if (ret != 0) return 1;
	}

	std::vector<std::vector<ShowItem> > showList;
	showList.push_back(showViewList);
	showList.push_back(showViewTicketList);
	showList.push_back(showPlayList);
	showList.push_back(showActList);
	showList.push_back(showShopList);
	showList.push_back(showRestList);
	std::vector<int> perNum = std::vector<int>(6,1);
	int Num[6] = {8,2,2,2,3,3};
	std::vector<int> maxNum(Num,Num+sizeof(Num)/sizeof(Num[0]));
	std::cerr<<"ViewList: "<<showViewList.size()<<" ViewTicketList: "<<showViewTicketList.size()<<" showPlayList size: "<<showPlayList.size()<<" showActList size: "<<showActList.size()<<" showShopList size: "<<showShopList.size()<<" showRestList size: "<<showRestList.size()<<std::endl;
	ret = ListResortViewShopRestTour(showList, perNum, maxNum, 20, basePlan->m_showItemList);
	if (ret != 0) return 1;
	int t = timer.cost();
	std::cerr << "Eden p201 cost " <<  t << std::endl;
	return 0;
}

int Processor::FilterTag(std::vector<const LYPlace*>& pList, std::vector<std::string> tags) {
	if (tags.size() == 0) {
		return 0;
	}
	std::tr1::unordered_set<const LYPlace*> placeSet;
	for (int i = 0;i < tags.size(); ++i) {
		for (std::vector<const LYPlace*>::iterator it = pList.begin(); it != pList.end();) {
			if ((*it)->m_tag.count(tags[i])){
				placeSet.insert(*it);
				it ++;
			} else {
				//it = pList.erase(it);
				it ++;
			}
		}
	}
	pList.clear();
	std::tr1::unordered_set<const LYPlace*>::iterator it;
	for (it = placeSet.begin(); it != placeSet.end(); it ++) {
		MJ::PrintInfo::PrintLog("Processor::FilterTag, %s(%s)", (*it)->_ID.c_str(), (*it)->_name.c_str());
		//		std::cerr << "id = " << (*it)->_ID << std::endl;
		pList.push_back(*it);
	}
	return 0;
}

int Processor::FilterPrivate(const QueryParam& qParam, std::vector<const LYPlace*>& pList, int privateOnly){
	for (std::vector<const LYPlace*>::iterator it = pList.begin(); it != pList.end();) {
		if(not privateOnly or privateOnly and not(*it)->m_ptid.empty()){
			it ++;
		} else {
			it = pList.erase(it);
		}
	}
	return 0;
}

int Processor::FilterId(const QueryParam& qParam, std::vector<const LYPlace*>& pList, const std::string& id) {
	if (id == "") return 0;
	const LYPlace* place = LYConstData::GetLYPlace(id, qParam.ptid);
	if(place == NULL) {
		pList.clear();
		return 0;
	}
	for (std::vector<const LYPlace*>::iterator it = pList.begin(); it != pList.end();) {
		if (*it == place) {
			it++;
		} else {
			it = pList.erase(it);
		}
	}
	return 0;
}

int Processor::FilterPtid(const QueryParam& qParam, std::vector<const LYPlace*>& pList, const std::string& ptid, int privateOnly) {
	for (std::vector<const LYPlace*>::iterator it = pList.begin(); it != pList.end();) {
		if ((*it)->m_custom != 0 and (*it)->m_ptid != ptid) {
			it = pList.erase(it);
			continue;
		}
		//非仅私有数据时 展示 公有和私有
		//仅私有数据时 展 纯私有
		if (!privateOnly) {
			//			MJ::PrintInfo::PrintLog("FilterPtid, success %s(%s) ptid (%s)", (*it)->_ID.c_str(), (*it)->_name.c_str(), (*it)->m_ptid.c_str());
			it ++;
		} else if ((*it)->m_ptid == ptid && (*it)->m_refer == "") {
			//			MJ::PrintInfo::PrintLog("FilterPtid, success %s(%s) ptid (%s)", (*it)->_ID.c_str(), (*it)->_name.c_str(), (*it)->m_ptid.c_str());
			it ++;
		} else {
			it = pList.erase(it);
		}
	}
	return 0;
}

//perNum 每遍允许插入个数；hasNum 已经插入个数（即list下标）；maxNum 每类最多允许插入个数；topNum 允许插入pList的个数
int Processor::ListResortViewShopRestTour(std::vector<std::vector<ShowItem> > showList, std::vector<int> perNum, std::vector<int> maxNum, int topNum, std::vector<ShowItem>& pList) {

	std::cerr<<"topNum: "<<topNum<<std::endl;
	int totalNum = 0;// 已pList的放入的总数
	int totalListSize = 0;// 所有列表中所有点的总数
	bool flag = true;
	bool needAdd = false;
	int len = showList.size();
	std::vector<int> hasNum;
	for (int i = 0; i < len; i++) {
		hasNum.push_back(0);
	}
	for (int i = 0; i < len; i++) {
		totalListSize += showList[i].size();
	}
	while (flag){
		for (int i = 0; i < len; i++){
			std::vector<ShowItem> list = showList[i];
			int per = 0;
			while (per < perNum[i] && hasNum[i] < list.size() && (hasNum[i] < maxNum[i] || needAdd)) { 
					pList.push_back(list[hasNum[i]]);
					++hasNum[i];
					++per;
					++totalNum;
					if (totalNum == topNum) {
						flag = false;
						break;
					}
			}
			if (maxNum[i] >= list.size() && hasNum[i] == list.size())
				hasNum[i] = maxNum[i];
			if (hasNum == maxNum && totalNum < topNum)
				needAdd = true;
			if (totalNum == totalListSize) //预防已push所有点但数量还未满足topNum
				flag = false;
			if (flag == false)
				break;
		}
	}
	std::cerr<<"P201 pList size: "<<pList.size()<<std::endl;
	return 0;
}

int Processor::FilterDist(BasePlan* basePlan, std::vector<const LYPlace*>& pList, long maxDist) {
	MJ::MyTimer timer;
	timer.start();
	int ret = 0;
	std::set<std::pair<std::string, std::string> > gridIdList;
	for (int i = 0; i < basePlan->m_listCenter.size(); ++i) {
		const LYPlace* city = LYConstData::GetLYPlace(basePlan->m_listCenter[i], basePlan->m_qParam.ptid);
		if (city != NULL) {
			LYConstData::GetRangePlaceId(city->_poi, maxDist, gridIdList);
			std::set<std::string> filterGridIdList;
			for(auto it=gridIdList.begin();it != gridIdList.end(); it++) {
				if ((*it).second == "" or (*it).second !="" && (*it).second  == basePlan->m_qParam.ptid) {
					filterGridIdList.insert((*it).first);
				}
			}
			for(auto it=filterGridIdList.begin();it != filterGridIdList.end(); it++) {
				const LYPlace* place = LYConstData::GetLYPlace(*it,basePlan->m_qParam.ptid);
				if(place == NULL or !(place->_t & basePlan->m_reqMode)) {
					continue;
				}
				pList.push_back(place);
			}
		}
	}
	std::cerr << "zhangyang, in FilterDist, pList size is " << pList.size() << std::endl;
	if (ret) {
		MJ::PrintInfo::PrintErr("[%s] Processor::FilterDist, Remove place error", basePlan->m_qParam.log.c_str());
	}
	std::cerr<<"FilterDist cost time: " << timer.cost() << std::endl << std::endl;
	return ret;
}

int Processor::getCandidatePoisUnderDist(BasePlan* basePlan,  std::vector<const LYPlace*>& inputList, std::vector<const LYPlace*>& outputList, const std::string& coord, long maxDist) {
	for (auto it = inputList.begin(); it != inputList.end(); ++it) {
		const LYPlace* place = *it;
		if (place == NULL)
			continue;
		//int pos = place->_poi.find(",");
		//if (pos == std::string::npos) {
		//	continue;
		//}
		//double lon = atof(place->_poi.substr(0, pos).c_str());
		//double lat = atof(place->_poi.substr(pos + 1).c_str());
		//std::cerr<< "lon " << lon << " lat " << lat << std::endl;
		//if( lat < rightLat && lat > leftLat && lon < rightLon && lon > leftLon) {
		//	outputList.push_back(place);
		//}
		outputList.push_back(place);
	}
	return 0;
}

int Processor::FilterUtime(const BasePlan* basePlan, std::vector<const LYPlace*>& pList, int utime) {
	int ret = 0;
	if (utime == -1) {
		return ret;
	}

	time_t now = MJ::MyTime::getNow();
	time_t placeTime;
	for (auto it = pList.begin(); it != pList.end();) {
		const LYPlace *place = *it;
		if (place->m_custom != 3) {
			it = pList.erase(it);
			continue;
		}
		placeTime = MJ::MyTime::toTime(place->_utime, "%Y-%m-%d %H:%M:%S");
		int offset = (now - placeTime) / 86400;
		if (offset > utime) {
			it = pList.erase(it);
		} else {
			it++;
		}
	}
	return ret;
}

int Processor::DoProcessB116(BasePlan* basePlan) {
	return 0;
}
