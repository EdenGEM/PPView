#ifndef _HOTELRANKPROCESSOR_H_
#define _HOTELRANKPROCESSOR_H_

#include "RankProcessor.h"

enum HotelRankTypeEnum {
	HOTEL_RANK_DEFAULT = 0,
	HOTEL_RANK_DISTANCE,
	HOTEL_RANK_HOT,
	HOTEL_RANK_RECOMMEND
};

class HotelRankProcessor : public RankProcessor {
public:
	HotelRankProcessor() {
	}

	virtual ~HotelRankProcessor() {
	}

	virtual double CalcScore(int RankType, void* PoiNode);

	virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode);
};
#endif