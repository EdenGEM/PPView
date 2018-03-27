#include "ShopRankProcessor.h"

double ShopRankProcessor::CalcScore(int RankType, void* PoiNode) {
	ScoreNodeS* Point = dynamic_cast<ScoreNodeS*>((ScoreNode*)(PoiNode));
	double ret = 0.0;
	switch (RankType) {
		case SHOP_RANK_DEFAULT: {
			ret = Point->m_hotScore;
			break;
		}
		case SHOP_RANK_DISTANCE: {
			ret = Point->m_distScore;
			break;
		}
		case SHOP_RANK_HOT: {
			ret = Point->m_hotScore;
			break;
		}
		case SHOP_RANK_RECOMMEND: {
			ret = Point->m_beenToScore;
			break;
		}
		default : {
			ret = Point->m_hotScore + Point->m_distScore;
			break;
		}
	}
	Point->m_finalScore = ret;
	return ret;
}

int ShopRankProcessor::SetFiltInfo(RankNode* TheNode, void* PoiNode) {
	return 0;
}
