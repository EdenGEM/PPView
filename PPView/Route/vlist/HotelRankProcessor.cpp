#include "HotelRankProcessor.h"

double HotelRankProcessor::CalcScore(int RankType, void* PoiNode) {
	ScoreNodeH* Point = dynamic_cast<ScoreNodeH*>((ScoreNode*)(PoiNode));
	double ret = 0.0;
	switch (RankType) {
		case HOTEL_RANK_DEFAULT: {
//			ret = -Point->m_priceScore + Point->m_hotScore + -Point->m_distScore;
			ret = Point->m_distScore;
			break;
		}
		case HOTEL_RANK_DISTANCE: {
			ret = Point->m_distScore;
			break;
		}
		case HOTEL_RANK_HOT: {
			ret = Point->m_hotScore;
			break;
		}
		case HOTEL_RANK_RECOMMEND: {
			ret = Point->m_beenToScore;
			break;
		}
		default: {
			ret = -Point->m_priceScore + Point->m_hotScore + Point->m_distScore;
			break;	
		}
	}
	Point->m_finalScore = ret;
	return ret;
}

int HotelRankProcessor::SetFiltInfo(RankNode* TheNode, void* PoiNode) {
	return 0;
}
