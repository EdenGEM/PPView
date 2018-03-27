#ifndef _VIEWRANKPROCESSOR_H_
#define _VIEWRANKPROCESSOR_H_

#include "RankProcessor.h"

enum ViewRankTypeEnum {
	VIEW_RANK_DEFAULT = 0,
	VIEW_RANK_DISTANCE,
	VIEW_RANK_HOT,
	VIEW_RANK_RECOMMEND
};

class ViewRankProcessor : public RankProcessor {
public:
	ViewRankProcessor() {
	}

	virtual ~ViewRankProcessor() {
	}

	virtual double CalcScore(int RankType, void* PoiNode);

	virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode);
};
#endif
