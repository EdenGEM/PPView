#ifndef __MYCLUSTER_H__
#define __MYCLUSTER_H__

#include <vector>
#include <set>
#include <tr1/unordered_set>
#include <algorithm>
#include <iostream>
#include "../base/define.h"

using namespace std;

class BagPlan;
class SPath;
class VarPlace;

namespace CLUSTERALG {

//基础数据,代表到key的距离是dist
struct Node {
	Node():id(0), value(0) {}
	Node(int i, int dist):id(i), value(dist) {}
	bool operator <(const Node &rhs) const {
		return value < rhs.value;
	}
public:
	int id;
	int value;
};

//表示从 x 到 y 的距离,指示聚类的方案
struct Edge {
public:
	Edge():idx(0), idy(0), value(0) {}
	Edge(int x, int y, int dist):idx(x), idy(y), value(dist) {}
	bool operator <(const Edge &rhs) const {
		return value < rhs.value;
	}
public:
	int idx;
	int idy;
	int value;
};

struct Result {
	std::vector<std::tr1::unordered_set<int> > clusterResult;
	void Show() const;
};

typedef std::vector<std::vector<Node> > CalMatrix;			//计算的矩阵
typedef std::vector<std::vector<int> > BaseMatrix;			//基础数据矩阵
typedef std::vector<Result>     ClusterResult;		//聚类结果

//聚类中间数据结构
class ClusterInfo {
public:
	Result m_result;	//当前聚类结果
	std::vector<Edge> m_edges;	//距离矩阵
public:
	void Show() const;
};

//聚类的数据结构
class Cluster {
public:
	const BaseMatrix *m_baseMatrix;	//基础矩阵
	CalMatrix m_calMatrix;		//计算矩阵
	const int m_clusterNum;		//聚类数目
	ClusterResult m_result;		//聚类结果
	bool m_isComplate;				//标记是否完成
public:
	Cluster(const BaseMatrix *baseMatrix, int clusterNum);
	virtual ~Cluster() {}
};


//kmeans算法 私有继承 has-a关系 与业务无关
class Kmeans:private Cluster {
public:
	const Result *m_initCluster;
public:
	Kmeans(const Result *initCluster, const BaseMatrix *baseMatrix, int clusterNum)
	:m_initCluster(initCluster), Cluster(baseMatrix, clusterNum) {
		//深复制
		m_result.push_back(*initCluster);
	}
	//获取多个聚类中心节点
	int GetAllCenters(const Result &cluster, std::vector<int> &nodes);
	//获取集合中心节点
	int GetCenter(const std::tr1::unordered_set<int> &cluster);
	int GetDist(int lhs, int rhs) const {
		return (*m_baseMatrix)[lhs][rhs];
	}
	const ClusterResult& GetResult() const { return m_result; };	//获取数据
	int GetClusterNum() const {
		return m_clusterNum;
	}
public:
	int ShowBaseMatrix() const;
	int SetResult(const Result &result) {
		m_result.clear();
		m_result.push_back(result);
	}
};

//层次聚类
class HierarchicalClustering:private Cluster {
public:
	HierarchicalClustering(const BaseMatrix *baseMatrix, int clusterNum, int nodeNum=160):Cluster(baseMatrix, clusterNum) {
		m_hasVisitSet.resize(nodeNum);
	}
	int Run();
	void ShowResult() const;
	const ClusterResult& GetResult() const { return m_result; };	//获取数据
private:
	int CanMerge(const ClusterInfo &clusterInfo, int idx, int depth);
	int DfsCluster(int depth, const ClusterInfo &result);	//聚类过程
	int Merge(ClusterInfo &clusterInfo, int idx);
	//得到集合A到集合B之间的距离
	int GetClusterDist(const Result &result, int clusterA, int clusterB);
	//计算集合的距离
	int CalClusterDist(ClusterInfo &clusterInfo);
private:
typedef std::vector<std::vector<int> > uniqueCluster;
	std::vector<std::set<uniqueCluster> > m_hasVisitSet;
	//std::set<uniqueCluster> m_hasVisitSet[160];
};

}

namespace YANGSHU {

//Block信息
struct BlockInfo {
	int m_idx;		//对应原下标
	int m_dur;		//停留时间
	bool m_is_not_play; //是否可以游玩,不可游玩时只能放租还车点
public:
	static bool CmpOfDur(const BlockInfo &lhs, const BlockInfo &rhs) {
		return lhs.m_dur> rhs.m_dur;
	}
};

struct NodeInfo {
	int m_idx;	//对应原始下标
	int m_dur;	//游玩时间
	const VarPlace *m_place;	//原始信息
	std::vector<int> m_bindInfo;	//绑定关系
};

class ClusterNode {
public:
	std::vector<int> m_node;	//集合的元素
	int m_duration;				//集合的已用时间
	int m_leftTime;				//剩余时间
	int m_restNum;				//已用餐馆的数目
	int m_restLeft;				//剩下餐馆的个数
	int m_bidx;					//m_blockInfo索引下标
public:
	ClusterNode():m_duration(0), m_leftTime(0), m_restLeft(0), m_restNum(0), m_bidx(-1) {}
public:
	static bool LeftTimeCmpOfPoint(const ClusterNode *lhs, const ClusterNode *rhs) {
		return lhs->m_leftTime < rhs->m_leftTime;
	}
	static bool LeftTimeCmp(const ClusterNode &lhs, const ClusterNode &rhs) {
		return lhs.m_leftTime < rhs.m_leftTime;
	}
	static bool RestLeftCmpOfPoint(const ClusterNode *lhs, const ClusterNode *rhs) {
		return lhs->m_restLeft < rhs->m_restLeft;
	}
	static bool RestLeftCmp(const ClusterNode &lhs, const ClusterNode &rhs) {
		return lhs.m_restLeft < rhs.m_restLeft;
	}
	static bool DurCmp(const ClusterNode &lhs, const ClusterNode &rhs) {
		return lhs.m_duration > rhs.m_duration;
	}
	static bool DurCmpOfPoint(const ClusterNode *lhs, const ClusterNode *rhs) {
		if(lhs->m_duration == rhs->m_duration) {
			return lhs->m_bidx < rhs->m_bidx;
		}
		return lhs->m_duration > rhs->m_duration;
	}
	static bool RestNumCmp(const ClusterNode &lhs, const ClusterNode &rhs) {
		return lhs.m_restNum < rhs.m_restNum;
	}
};

class ClusterList {
public:
	std::vector<ClusterNode> m_cluster;
	CLUSTERALG::Result m_result;
public:
	int ShowClusterInfo() const;
};

//MyKmeans算法 业务相关的kmeans is-a关系
class MyKmeans:public CLUSTERALG::Kmeans {
private:
	BagPlan *m_bagPlan;
	//全部景点
	//std::vector<const VarPlace*> &m_vList;
	int m_P2Cluster[200][200];	//点到集合之间的距离,用于计算更适合把哪个景点调整到哪个集合,暂时这样
	const std::vector<NodeInfo> &m_bindInfo;	//绑定关系
	const std::vector<BlockInfo> &m_blockInfo;	//block信息
	ClusterList m_clusterList;	//聚类中间数据
	std::tr1::unordered_set<std::vector<int> > m_clusterCenterSet;
public:
	MyKmeans(const CLUSTERALG::Result *initCluster,
			const CLUSTERALG::BaseMatrix *baseMatrix,
			int clusterNum,
			BagPlan *bagPlan,
			std::vector<const VarPlace *> &vList,
			const std::vector<NodeInfo> &bindInfo,
			const std::vector<BlockInfo> &blockInfo)
	:Kmeans(initCluster, baseMatrix, clusterNum),
	m_bagPlan(bagPlan),m_bindInfo(bindInfo),m_blockInfo(blockInfo) {
		m_clusterList.m_result = *initCluster;
		for(int i = 0; i < m_clusterList.m_result.clusterResult.size(); ++i) {
			const auto &mySet = m_clusterList.m_result.clusterResult[i];
			ClusterNode clusterNode;
			for(auto cit = mySet.begin(); cit != mySet.end(); ++cit) {
				clusterNode.m_node.push_back(*cit);
				clusterNode.m_duration += m_bindInfo[*cit].m_dur;
			}
			m_clusterList.m_cluster.push_back(clusterNode);
		}
		std::sort(m_clusterList.m_cluster.begin(), m_clusterList.m_cluster.end(), ClusterNode::DurCmp);
		for(int i = 0; i < m_clusterList.m_cluster.size(); ++i) {
			//现在已经强制要求block和cluster数目对应,在kmeans阶段即指定其所属的block
			m_clusterList.m_cluster[i].m_bidx = i;
		}
	}
	int Run();	//计算逻辑
private:
	int KmeansAdjust();
	int CalPointToClusterDist(const vector<int> &vCenter);
	int CalPointToClusterDistOneNode(const int idxA, const int idxB) const;
	int GreedSwitchAllPoint();
	int CalPointToClusterDistOneCluster(int cluster, int centerIdx);
private:
	//加点和减点
	int ClusterAddPoint(int clusterIdx, int nodeIdx);
	int ClusterSubPoint(int clusterIdx, int nodeIdx);

	//节点在不同聚类之间的移动
	int MovePointFromClusterAToClusterB(int nodeIdx, int clusterAIdx, int clusterBIdx);
private:
	int ShowP2ClusterData() const;
};

class ClusterOrganizer {
	enum {
		Nomiss_all,
		Nomiss_random,
		Nomiss_all_Others_random
	};
public:
	//ClusterOrganizer(const BagPlan* bagPlan, std::vector<const SPath*> *rootList);	常量版本
	ClusterOrganizer(BagPlan* bagPlan, std::vector<const SPath*> *rootList);
	int Run();
private:
	int ClusterDumpToSPath(const CLUSTERALG::ClusterResult &results, const std::vector<NodeInfo> &m_bindInfo, const std::vector<BlockInfo> &m_blockInfo);
	//选择游玩景点
	int GetPlanPlace(std::vector<const VarPlace *> &vList, int  loop);
	int InitInfo(std::vector<const VarPlace *> &vList, std::vector<NodeInfo> &m_bindInfo, std::vector<BlockInfo> &m_blockInfo);	//初始化各种数据
	//景点列表转成矩阵
	int List2Matrix(const std::vector<const VarPlace *> &vList, CLUSTERALG::BaseMatrix &matrix);
	int ShowCluster(const CLUSTERALG::ClusterResult *result, const std::vector<const VarPlace *> &vList) const;
public:
	BagPlan *m_bagPlan;
	std::vector<const SPath*> *m_rootList;
private:
	std::tr1::unordered_set<uint32_t> m_hashViewedSet;	//表示已经聚类过的集合
	//uint32_t Hash(bit160& tVec) const;
	uint32_t CalHashPointSet(const std::vector<NodeInfo> &nodeInfo);
	bool HasVisit(const uint32_t &hash) {
		if(m_hashViewedSet.find(hash) == m_hashViewedSet.end()) {
			m_hashViewedSet.insert(hash);
			return false;
		}
		return true;
	}
	//std::vector<NodeInfo> m_bindInfo;	//每个节点的关系
	//std::vector<BlockInfo> m_blockInfo;	//block信息
};
}

#endif
