#include "ViewShopRankProcessor.h"

double ViewShopRankProcessor::CalcScore(int RankType, void* PoiNode) {
	ScoreNodeVS* Point = dynamic_cast<ScoreNodeVS*>((ScoreNode*)(PoiNode));
	double ret = 0.0;
	switch (RankType) {
		case VIEW_SHOP_RANK_DEFAULT: {
			ret = Point->m_hotScore;
			break;
		}
		case VIEW_SHOP_RANK_DISTANCE: {
			ret = Point->m_distScore;
			break;
		}
		case VIEW_SHOP_RANK_HOT: {
			ret = Point->m_hotScore;
			break;
		}
		case VIEW_SHOP_RANK_RECOMMEND: {
			ret = Point->m_beenToScore;
			break;
		}
		default: {
			ret = Point->m_hotScore + Point->m_distScore;
			break;	
		}
	}
	Point->m_finalScore = ret;
	return ret;
}

int ViewShopRankProcessor::SetFiltInfo(RankNode* TheNode, void* PoiNode) {
	return 0;
}
