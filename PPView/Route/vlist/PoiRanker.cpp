#include <algorithm>
#include "PoiRanker.h"

/*int DoRank(FiltCons& FCons, std::vector<RankNode*>& NodeList) const {
	DoFilt(FCons, NodeList);
	DoSort(NodeList);
	return 0;
}*/

int PoiRanker::DoFilt(FiltCons& FCons, std::vector<RankNode*>& NodeList) const {
	bool isHit = false;
	if ((FCons.m_tagList.size() == 0) && (FCons.m_rangeList.size() == 0)) return 0;

	for (int i = 0; i < NodeList.size(); i ++) {
		isHit = true;
		for (int j = 0; j < FCons.m_tagList.size(); j ++) {
			if (FCons.m_tagList[i].second.find(NodeList[i]->tagMap[FCons.m_tagList[i].first]) == FCons.m_tagList[i].second.end()) {
				isHit = false;
				break;
			}
		}

		for (int j = 0; j < FCons.m_rangeList.size(); j ++) {
			if ((NodeList[i]->valueMap[FCons.m_rangeList[i].first] < FCons.m_rangeList[i].second.first) || 
				(NodeList[i]->valueMap[FCons.m_rangeList[i].first] > FCons.m_rangeList[i].second.second)) {
				isHit = false;
				break;
			}
		}

		if (!isHit) {
			delete NodeList[i];
			NodeList.erase(NodeList.begin() + i);
		}
	}
	return 0;
}

int PoiRanker::DoRank(std::vector<RankNode*>& NodeList, bool IsReverse = false) const {
	MJ::PrintInfo::PrintDbg("[RankProcessor][Init] size = %d",NodeList.size());
	if (NodeList.size() > 0) {
		if (IsReverse) {
			std::sort(NodeList.begin(), NodeList.end(), RankNode::NodeCompR);
		} else {
			std::sort(NodeList.begin(), NodeList.end(), RankNode::NodeComp);
		}
	}
	return 0;
}
