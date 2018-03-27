#include "ViewRankProcessor.h"

double ViewRankProcessor::CalcScore(int RankType, void* PoiNode) {
	ScoreNodeV* Point = dynamic_cast<ScoreNodeV*>((ScoreNode*)(PoiNode));
	double ret = 0.0;
	ScoreNodeV* scoreNodev = (ScoreNodeV*)PoiNode;
	switch (RankType) {
		case VIEW_RANK_DEFAULT: {
		 	ret = Point->m_hotScore;
			break;
		}
		case VIEW_RANK_DISTANCE: {
			ret = Point->m_distScore;
			break;
		}
		case VIEW_RANK_HOT: {
			ret = Point->m_hotScore;
			break;
		}
		case VIEW_RANK_RECOMMEND: {
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

int ViewRankProcessor::SetFiltInfo(RankNode* TheNode, void* PoiNode) {
	return 0;
}
