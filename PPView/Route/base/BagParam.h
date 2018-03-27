#ifndef _BAG_PARAM_H_
#define _BAG_PARAM_H_

#include <iostream>
#include <vector>
#include <bitset>
#include <tr1/unordered_map>


class CityBParam {
public:
	CityBParam();
	CityBParam(double bagWeightA,  double bagThre, int rootHeapLimit, int richHeapLimit, int dfsHeapLimit, int rootRetLimit, int richExtraDur, int richTopK, int richMissLimit) {
		Copy(bagWeightA,  bagThre, rootHeapLimit, richHeapLimit, dfsHeapLimit, rootRetLimit, richExtraDur, richTopK, richMissLimit);
	}
	CityBParam(const CityBParam& cbP) {
		Copy(cbP.m_cbBagWeightA, cbP.m_cbBagThre, cbP.m_cbRootHeapLimit, cbP.m_cbRichHeapLimit, cbP.m_cbDFSHeapLimit, cbP.m_cbRootRetLimit, cbP.m_cbRichExtraDur, cbP.m_cbRichTopK, cbP.m_cbRichMissLimit);
	}
	CityBParam& operator=(const CityBParam& cbP) {
		if (this == &cbP) {
			return *this;
		}
		this->Copy(cbP.m_cbBagWeightA, cbP.m_cbBagThre, cbP.m_cbRootHeapLimit, cbP.m_cbRichHeapLimit, cbP.m_cbDFSHeapLimit, cbP.m_cbRootRetLimit, cbP.m_cbRichExtraDur, cbP.m_cbRichTopK, cbP.m_cbRichMissLimit);
		return *this;
	}
private:
	int Copy(double bagWeightA, double bagThre, int rootHeapLimit, int richHeapLimit, int dfsHeapLimit, int rootRetLimit, int richExtraDur, int richTopK, int richMissLimit) {
		m_cbBagWeightA = bagWeightA;
		m_cbBagThre = bagThre;
		m_cbRootHeapLimit = rootHeapLimit;
		m_cbRichHeapLimit = richHeapLimit;
		m_cbDFSHeapLimit = dfsHeapLimit;
		m_cbRootRetLimit = rootRetLimit;
		m_cbRichExtraDur = richExtraDur;
		m_cbRichTopK = richTopK;
		m_cbRichMissLimit = richMissLimit;
	}
public:
	double m_cbBagWeightA;
	double m_cbBagThre;
	int m_cbRootHeapLimit;
	int m_cbRichHeapLimit;
	int m_cbDFSHeapLimit;
	int m_cbRootRetLimit;
	int m_cbRichExtraDur;
	int m_cbRichTopK;
	int m_cbRichMissLimit;
};

class BagParam {
public:
	static int Init(const std::string& dataPath);
	static int InitBase(const std::string& dataPath);
	static int InitCBP(const std::string& dataPath);
public:
	static std::string m_confName;
	static std::string m_cbPName;
	// 全局
	static int m_spAllocLimit;  // 内存池大小 必须大于heap长度
	static int m_bgPoolLimit;
	static int m_timeOut;
	static int m_queueTimeOut;
	static int m_rootTimeOut;
	static int m_richTimeOut;
	static int m_dfsTimeOut;
	static int m_routeTimeOut;
	static int m_threadNum;  // 线程数
	static int m_dayViewLimit;  // 每天最大景点(非餐厅)数
	static const int m_dayNodeLimit = 20;  // 每天允许最多几个点

	// 筛包参数
	static double m_bagWeightA; 
        static double m_bagThre;

	// Bag
	static double m_minPushScale;  // 作为骨架最小时长占比
	static double m_maxPushScale;  // 作为骨架最大时长占比(超过不继续搜索)
	static double m_diffPushScale;
	static int m_rootHeapLimit;  // 骨架广搜队列长度
	static int m_rootHotGap;  // 骨架热度保留最高hot-grade路径
	static int m_rootRetLimit;  // 骨架最终输出长度

	// dfs
	static int m_dfsHeapLimit;  // DFS输出长度
	static int m_dfsDayHeapLimit;  // DFS天内输入top几
	static int m_dfsSPSHashHeapLimit;  // DFS相同点集输出top几 SPS: same point set

	
	// rich
	static int m_richHeapLimit;  // rich长度限制
	static int m_richMissLimit;  // rich略过数量剪枝(高hot略过太多)
	static int m_richExtraDur;  // 多玩多久搜索深度
	static int m_richTopK;  // 距离剪枝只留前top近
	

	static std::tr1::unordered_map<std::string, std::vector<std::pair<int, CityBParam> > > m_cbPMap;

	static bool m_useCityBP;
	static const CityBParam m_minCBParam;
};


#endif
