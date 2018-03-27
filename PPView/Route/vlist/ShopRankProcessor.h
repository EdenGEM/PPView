#ifndef _SHOPRANKPROCESSOR_H_
#define _SHOPRANKPROCESSOR_H_

#include "RankProcessor.h"

enum ShopRankTypeEnum {
	SHOP_RANK_DEFAULT = 0,
	SHOP_RANK_DISTANCE,
	SHOP_RANK_HOT,
	SHOP_RANK_RECOMMEND
};

class ShopRankProcessor : public RankProcessor {
public:
	ShopRankProcessor() {
	}

	virtual ~ShopRankProcessor() {
	}

	virtual double CalcScore(int RankType, void* PoiNode);

        virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode);
};
#endif
