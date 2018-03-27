#ifndef _POIRANKER_H_
#define _POIRANKER_H_

#include <vector>
#include <string>
#include <tr1/unordered_map>

#include "FiltCons.h"
#include "MJCommon.h"

typedef struct RankNodeS {
	double score;
	void* poiNode;

	std::tr1::unordered_map<int, double> valueMap;
	std::tr1::unordered_map<int, std::string> tagMap;

	static bool NodeComp(struct RankNodeS* NodeA, struct RankNodeS* NodeB) {
		return (NodeA->score > NodeB->score);
	}

	static bool NodeCompR(struct RankNodeS* NodeA, struct RankNodeS* NodeB) {
		return (NodeA->score < NodeB->score);
	}
} RankNode;

class PoiRanker {
public:
	PoiRanker() {
	}

	virtual ~PoiRanker() {
	}

//	int DoRank(FiltCons& FCons, std::vector<RankNode*>& NodeList) const;

	int DoFilt(FiltCons& FCons, std::vector<RankNode*>& NodeList) const;

	int DoRank(std::vector<RankNode*>& NodeList, bool IsReverse) const;
};
#endif
