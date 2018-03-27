#ifndef _RESTAURANTRANKPROCESSOR_H_
#define _RESTAURANTRANKPROCESSOR_H_

#include "RankProcessor.h"

enum RestaurantRankTypeEnum {
	RESTAURANT_RANK_DEFAULT = 0,
	RESTAURANT_RANK_DISTANCE,
	RESTAURANT_RANK_HOT,
	RESTAURANT_RANK_RECOMMEND,
	RESTAURANT_RANK_PRICE,
	RESTAURANT_RANK_PRICE_REV
};

class RestaurantRankProcessor : public RankProcessor {
public:
	RestaurantRankProcessor() {
	}

	virtual ~RestaurantRankProcessor() {
	}

	virtual double CalcScore(int RankType, void* PoiNode);

	virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode);
};
#endif
