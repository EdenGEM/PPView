#include "RestaurantRankProcessor.h"

double RestaurantRankProcessor::CalcScore(int RankType, void* PoiNode) {
	ScoreNodeR* Point = dynamic_cast<ScoreNodeR*>((ScoreNode*)(PoiNode));
	double ret = 0.0;
	switch(RankType) {
		case RESTAURANT_RANK_DEFAULT: {
			ret = Point->m_hotScore - Point->m_priceScore + Point->m_distScore * 2;
			break;
		}
		case RESTAURANT_RANK_DISTANCE:{
			ret = Point->m_distScore;
			break;
		}
		case RESTAURANT_RANK_HOT: {
			ret = Point->m_hotScore;
			break;
		}
		case RESTAURANT_RANK_RECOMMEND: {
			ret = Point->m_beenToScore;
			break;
		}
		case RESTAURANT_RANK_PRICE: {
			ret = -Point->m_priceScore;
			break;
		}
		case RESTAURANT_RANK_PRICE_REV: {
			ret = Point->m_priceScore;
			break;
		}
		default : {
			ret = Point->m_hotScore - Point->m_priceScore;
			break;
		}
	}
	Point->m_finalScore = ret;
	return ret;
}

int RestaurantRankProcessor::SetFiltInfo(RankNode* TheNode, void* PoiNode) {
	return 0;
}
