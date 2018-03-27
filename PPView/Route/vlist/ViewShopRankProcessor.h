#ifndef _VIEWSHOPRANKPROCESSOR_H_
#define _VIEWSHOPRANKPROCESSOR_H_

#include "RankProcessor.h"

enum ViewShopRankTypeEnum {
	VIEW_SHOP_RANK_DEFAULT = 0,
	VIEW_SHOP_RANK_DISTANCE,
	VIEW_SHOP_RANK_HOT,
	VIEW_SHOP_RANK_RECOMMEND
};

class ViewShopRankProcessor : public RankProcessor {
public:
	ViewShopRankProcessor() {
	}

	virtual ~ViewShopRankProcessor() {
	}

	virtual double CalcScore(int RankType, void* PoiNode);

	virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode);
};
#endif
