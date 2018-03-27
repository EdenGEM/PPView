#ifndef _RANKPROCESSOR_H_
#define _RANKPROCESSOR_H_

#include "PoiRanker.h"

class LYPlace;

class RankProcessor {
public:
	RankProcessor() {
	}

	virtual ~RankProcessor() {
		for (int i = 0; i < m_nodeList.size(); i ++) {
			delete m_nodeList[i];
		}
		m_nodeList.clear();
	}

	int Init(int RankType, std::vector<void*>& PoiNodeList) {
		RankNode* tNode = NULL;
		MJ::PrintInfo::PrintDbg("[RankProcessor][Init] size = %d",PoiNodeList.size());
		for (int i = 0; i < PoiNodeList.size(); i ++) {
			tNode = new RankNode;
			tNode->score = CalcScore(RankType, PoiNodeList[i]);
			tNode->poiNode = PoiNodeList[i]; 
			SetFiltInfo(tNode, PoiNodeList[i]);
			m_nodeList.push_back(tNode);
		}
		return 0;
	}

	int DoRank(std::vector<void*>& RankRet, FiltCons& FCons, int CountPerPage, int PageCount, bool IsReverse = false) {
		PoiRanker ranker;
		ranker.DoFilt(FCons, m_nodeList);
		ranker.DoRank(m_nodeList, IsReverse);

		int size = (PageCount + 1)*CountPerPage;
		if (size > m_nodeList.size()) size = m_nodeList.size();
		for (int i = PageCount*CountPerPage; i < size; i ++) {
			RankRet.push_back(m_nodeList[i]->poiNode);
		}
		return 0;
	}

	int DoRank(std::vector<void*>& RankRet, FiltCons& FCons, bool IsReverse = false) {
		PoiRanker ranker;
		ranker.DoFilt(FCons, m_nodeList);
		ranker.DoRank(m_nodeList, IsReverse);

		//int size = (PageCount + 1)*CountPerPage;
		//if (size > m_nodeList.size()) size = m_nodeList.size();
		for (int i = 0; i < m_nodeList.size(); i ++) {
			RankRet.push_back(m_nodeList[i]->poiNode);
		}
		return 0;
	}

	virtual double CalcScore(int RankType, void* PoiNode) = 0;

	virtual int SetFiltInfo(RankNode* TheNode, void* PoiNode) = 0;

	std::vector<RankNode*> m_nodeList;
};

class ScoreNode{
public:
	ScoreNode() {	
		m_hotScore = 0 ;
		m_distScore = 0 ;
		m_levelScore = 0;
		m_priceScore = 0;
		m_beenToScore = 0;
		m_gradeScore = 0;
		m_finalScore = 0;
	}
	ScoreNode(const LYPlace* LyPlace) {
		m_lp = LyPlace;
		m_hotScore = 0 ;
		m_distScore = 0 ;
		m_levelScore = 0;
		m_priceScore = 0;
		m_beenToScore = 0;
		m_gradeScore = 0;
		m_finalScore = 0;
	}
	virtual ~ScoreNode() {
	}
	int init(const LYPlace* LyPlace, double hotScore, double distScore, double levelScore, double priceScore) {
		m_lp = LyPlace;
		m_hotScore = hotScore ;
		m_distScore = distScore ;
		m_levelScore = levelScore;
		m_priceScore = priceScore;
		return 0;
	}
	const LYPlace* m_lp;
	double m_beenToScore;
	double m_hotScore;
	double m_distScore;
	double m_levelScore;
	double m_priceScore;
	double m_gradeScore;
	double m_finalScore;
};

class ScoreNodeV:public ScoreNode{
public:
	ScoreNodeV(const LYPlace* LyPlace) {
		m_lp = LyPlace;
	}
	virtual ~ScoreNodeV() {
	}
};

class ScoreNodeS:public ScoreNode{
public:
	ScoreNodeS(const LYPlace* LyPlace) {
		m_lp = LyPlace;
	}
	virtual ~ScoreNodeS() {

	}
	
};

class ScoreNodeR:public ScoreNode{
public:
	ScoreNodeR(const LYPlace* LyPlace) {
		m_lp = LyPlace;
	}
	virtual ~ScoreNodeR(){

	}
	
};

class ScoreNodeH:public ScoreNode{
public:
	ScoreNodeH(const LYPlace* LyPlace) {
		m_lp = LyPlace;
	}
	virtual ~ScoreNodeH(){

	}
	
};

class ScoreNodeVS:public ScoreNode{
public:
	ScoreNodeVS(const LYPlace* LyPlace) {
		m_lp = LyPlace;
	}
	virtual ~ScoreNodeVS(){

	}
	
};


#endif
