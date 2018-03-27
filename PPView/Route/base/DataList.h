#ifndef _DATA_LIST_H_
#define _DATA_LIST_H_

#include <iostream>
#include <vector>
#include "define.h"
#include "BasePlan.h"
#include "Route/vlist/RankProcessor.h"
#include "LYConstData.h"
#include "ToolFunc.h"

class DataList {
public:
	//排序功能
	static int SortList(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList);

	static int SortListRanking(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList);
	static int SortListHot(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList);

	static int SortListTour(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList);
	//转换地点为ScoreNode
	static int GetSNodeList(int nodeType, std::vector<const LYPlace*>& pList, std::vector<const ScoreNode*>& scoreNodeList);
	//获取需要的ScoreNode
	static ScoreNode* GetSNode(int type, const LYPlace* place);
	//计算距离分数
	static int CalDistScoreCenter(std::vector<const ScoreNode*>& scoreNodeList, const LYPlace* &center);
	static int CalDistScoreNear(std::vector<const ScoreNode*>& scoreNodeList, std::vector<const LYPlace*>& nearPlaceList);
	//计算距离的中心 返回中心
	static int CalSphereDistCenter(std::vector < const LYPlace*>& userPlaceList , const LYPlace* center);
	//计算热度分数
	static int CalHotScore(std::vector <const ScoreNode*>& scoreNodeList);
	//计算酒店价格分数
	static int CalPriceScore(std::vector < const ScoreNode* >& scoreNodeList, std::vector < const HotelInfo* >& hotelInfoList);
	//计算餐馆价格分数
	static int CalPriceScore(std::vector<const ScoreNode*>& scoreNodeList);
	//调用排序端口
	static int PlaceRanker(int type, int rankType, std::vector< const ScoreNode*>& scoreNodeList, std::vector<ShowItem>& showItemList);
	//获取景点列表
	static int GetLYPlaceList(BasePlan* basePlan,int placeType,  std::vector<const LYPlace*>& pList);
	//获取城市的所有Place
	static int GetCityPlace(std::string cityID, int type, std::vector<const LYPlace*>& pList, const std::string& ptid, ErrorInfo& errorInfo );
	static int RemoveBlackPlace(std::vector < const LYPlace* >& blackPlaceList, std::vector < const LYPlace* >& pList);
	//add by yangshu
	static int GetTour(const std::string &cid, int type, std::vector<const LYPlace*>& pList, const std::string& ptid, ErrorInfo& errorInfo );
	static int GetTourList(BasePlan* basePlan,int placeType,  std::vector<const LYPlace*>& pList);
	//add end
	//add by shyy
	static int SortListTourDatePrice(BasePlan* basePlan, int reqMode, std::vector<const LYPlace*>& pList, std::vector<ShowItem>& showItemList);
};

struct VPlaceRankCmp {
	bool operator() (const LYPlace* p1, const LYPlace* p2) {
		const VarPlace* v1 = dynamic_cast<const VarPlace*>(p1);
		const VarPlace* v2 = dynamic_cast<const VarPlace*>(p2);
		if (v1 != NULL && v2 != NULL) {
			if (v1->_rec_priority == v2->_rec_priority) return (v1->_ranking < v2->_ranking); // 是否有优先推荐
			if (v1->_rec_priority == 10) return true;
			return false;
		}
	}
};

struct VPlaceHotCmp {
	bool operator() (const LYPlace* p1, const LYPlace* p2) {
		const VarPlace* v1 = dynamic_cast<const VarPlace*>(p1);
		const VarPlace* v2 = dynamic_cast<const VarPlace*>(p2);
		if (v1 != NULL && v2 != NULL) {
			return (v1->_hot_level > v2->_hot_level);
		}
	}
};

struct VPlaceGradeCmp {
	bool operator() (const LYPlace* p1, const LYPlace* p2) {
		const VarPlace* v1 = dynamic_cast<const VarPlace*>(p1);
		const VarPlace* v2 = dynamic_cast<const VarPlace*>(p2);
		if (v1 != NULL && v2 != NULL) {
			// 首先判断是否优先推荐
            if (v1->_rec_priority == 10 && v2->_rec_priority != 10) return true;
            if (v1->_rec_priority != 10 && v2->_rec_priority == 10) return false;

            // m_refer不为空，m_custom 为3， 判断为私有
			int v1_private = 0, v2_private = 0;
            if (v1->m_custom == 3 && v1->m_refer == "") {
                v1_private = 1;
            }
            if (v2->m_custom == 3 && v2->m_refer == "") {
                v2_private = 1;
            }

            if (v1_private != v2_private) {
                return v1_private < v2_private;
            } else if (v1_private == 0 && v2_private == 0) {
				if (v1->_grade != v2->_grade) {
					return v1->_grade > v2->_grade;
				} else if(v1->_ranking != v2->_ranking){
					return v1->_ranking < v2->_ranking;
				} else if (v1->_name != v2->_name) {
					return v1->_name > v2->_name;
				} else {
					return v1->_ID < v2->_ID;
				}
            } else if (v1_private == 1 && v2_private == 1) {
				if (v1->_utime != v2->_utime) {
					return v1->_utime > v2->_utime;
				} else if(v1->_name != v2->_name) {
					return v1->_name > v2->_name;
				} else {
					return v1->_ID < v2->_ID;
				}
            } else {
				return v1->_ID < v2->_ID;
			}
		}
	}
};

#endif
