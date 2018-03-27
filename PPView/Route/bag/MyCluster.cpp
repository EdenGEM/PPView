#include "MyCluster.h"
#include "Route/base/define.h"
#include "BagPlan.h"
#include "SPath.h"
#include "Route/base/BagParam.h"
#include "StaticRand.h"
#include "MJCommon.h"
#include "math.h"

namespace CLUSTERALG {

void Result::Show() const {
	for(int i = 0; i < clusterResult.size(); ++i) {
		cerr << i << ": ";
		for(auto it = clusterResult[i].begin(); it != clusterResult[i].end(); ++it) {
			cerr << *it << " ";
		}
		cerr << endl;
	}
}

void ClusterInfo::Show() const {
	cerr << "Cluster:" << endl;
	m_result.Show();
	for(int i = 0; i < m_edges.size(); i ++) {
		cerr << m_edges[i].idx << " - " << m_edges[i].idy << " dist = " << m_edges[i].value << endl;
	}
}

Cluster::Cluster(const BaseMatrix *baseMatrix, int clusterNum):m_baseMatrix(baseMatrix),m_clusterNum(clusterNum),m_isComplate(false) {
	for(int i = 0; i < baseMatrix->size(); ++i) {
		vector<Node> list;
		for(int j = 0; j < (*baseMatrix)[i].size(); ++j) {
			list.push_back(Node(j, (*baseMatrix)[i][j]));
		}
		sort(list.begin(), list.end());
		m_calMatrix.push_back(list);
	}
}

int Kmeans::ShowBaseMatrix() const {
	cerr << "BaseMatrix:" << endl;
	int len = (*m_baseMatrix).size();
	for(int i = 0; i < len; ++i) {
		for(int j = 0; j < len; ++j) {
			cerr << (*m_baseMatrix)[i][j] << " ";
		}
		cerr << endl;
	}
	return 0;
}

void HierarchicalClustering::ShowResult() const {
	for(int i = 0; i < m_result.size(); ++i) {
		cerr << "cluster i: " << i << endl;
		m_result[i].Show();
	}
}

int HierarchicalClustering::CalClusterDist(ClusterInfo &clusterInfo) {
	//聚类数据改变
	const Result &result = clusterInfo.m_result;
	vector<Edge> &edges = clusterInfo.m_edges;
	edges.clear();
	for(int i = 0; i < result.clusterResult.size(); ++i) {
		if(result.clusterResult[i].empty()) {
			continue;
		}
		for(int j = i+1; j < result.clusterResult.size(); ++j) {
			if(result.clusterResult[j].empty()) {
				continue;
			}
			int dist = GetClusterDist(result, i, j);
			edges.push_back(Edge(i, j, dist));
		}
	}
	sort(edges.begin(), edges.end());
	return 0;
}

int HierarchicalClustering::Run() {
	ClusterInfo clusterInfo;
	for(int i = 0; i < m_calMatrix.size(); ++i) {
		std::tr1::unordered_set<int> set;
		set.insert(i);
		clusterInfo.m_result.clusterResult.push_back(set);
	}
	while(clusterInfo.m_result.clusterResult.size() < m_clusterNum) {
		std::tr1::unordered_set<int> set;
		clusterInfo.m_result.clusterResult.push_back(set);
	}
	CalClusterDist(clusterInfo);
	//if(1) {
	//	cerr << "in HierarchicalClustering" << endl;
	//	clusterInfo.Show();
	//}
	DfsCluster(0, clusterInfo);
	return 0;
}

//层次聚类过程
int HierarchicalClustering::DfsCluster(int depth, const ClusterInfo &clusterInfo) {
	//当前聚类数目
	int nowCluster = m_baseMatrix->size()-depth;
	//两类过程
	if(nowCluster <= m_clusterNum) {
		m_result.push_back(clusterInfo.m_result);
	}
	else {
		int MaxCluster = 3;
		if(depth >= 3) {
			MaxCluster = 1;
		}
		ClusterInfo tmp;
		for(int i = 0, cnt = 0; i < clusterInfo.m_edges.size() && cnt < MaxCluster; ++i) {
			//如果需要多于一个解则进行复制,否则不需要复制
			if(!CanMerge(clusterInfo, i, depth)) {
				continue;
			}
			ClusterInfo *useCluster = NULL;
			if(MaxCluster > 1) {
				tmp = clusterInfo;	//用户记录中间状态
				Merge(tmp, i);		//合并两个集合
				useCluster = &tmp;
			}
			else {
				useCluster = const_cast<ClusterInfo *>(&clusterInfo);
				Merge(*useCluster, i);		//合并两个集合
			}
			DfsCluster(depth+1, *useCluster);
			++cnt;
		}
	}
	return 0;
}

//层次聚类合并集合
int HierarchicalClustering::CanMerge(const ClusterInfo &clusterInfo, int idx, int depth) {
	//合并集合 xid, yid
	int xid = clusterInfo.m_edges[idx].idx, yid = clusterInfo.m_edges[idx].idy;
	//集合改变
	const Result &result = clusterInfo.m_result;
	std::vector<std::vector<int> > tmp;
	tmp.reserve(result.clusterResult.size());
	for(int i = 0; i < result.clusterResult.size(); ++i) {
		if(i == yid) {
			continue;
		}
		std::vector<int> v;
		for(auto it = result.clusterResult[i].begin(); it != result.clusterResult[i].end(); ++it) {
			v.push_back(*it);
		}
		if(i == xid) {
			for(auto it = result.clusterResult[yid].begin(); it != result.clusterResult[yid].end(); ++it) {
				v.push_back(*it);
			}
		}
		sort(v.begin(), v.end());
		tmp.push_back(v);
	}
	sort(tmp.begin(), tmp.end());
	if(m_hasVisitSet[depth].find(tmp) == m_hasVisitSet[depth].end()) {
		m_hasVisitSet[depth].insert(tmp);
		return 1;
	}
	return 0;
}

//层次聚类合并集合
int HierarchicalClustering::Merge(ClusterInfo &clusterInfo, int idx) {
	//合并集合 xid, yid
	int xid = clusterInfo.m_edges[idx].idx, yid = clusterInfo.m_edges[idx].idy;
	//集合改变
	Result &result = clusterInfo.m_result;
	result.clusterResult[xid].insert(result.clusterResult[yid].begin(), result.clusterResult[yid].end());
	result.clusterResult[yid].clear();

	//聚类之间的距离改变
	CalClusterDist(clusterInfo);
	return 0;
}

//两个聚类之间的距离
int HierarchicalClustering::GetClusterDist(const Result &result, int clusterA, int clusterB) {//计算两个类间的“距离”，取所有距离对的前三短的平均值
	auto setA = result.clusterResult[clusterA];
	auto setB = result.clusterResult[clusterB];
	std::set<int> distSet;
	for(auto itA = setA.begin(); itA != setA.end(); ++itA) {
		for(auto itB = setB.begin(); itB != setB.end(); ++itB) {
			distSet.insert(((*m_baseMatrix)[*itA][*itB]));
			if(distSet.size() > 3) {
				distSet.erase(--distSet.end());
			}
		}
	}
	int sum = 0, num = distSet.size();
	if(num == 0) {
		num = 1;
	}
	for(auto it = distSet.begin(); it != distSet.end(); ++it) {
		sum += *it;
	}
	return sum/num;
}

int Kmeans::GetCenter(const std::tr1::unordered_set<int> &cluster) {
	int minIdx = -1;
	int minDist = 99999999;
	if(cluster.size() > 0) {
		minIdx = *cluster.begin();
	}
	for (auto cit = cluster.begin(); cit != cluster.end(); ++cit) {
		int dist = 0;
		for (auto cit1 = cluster.begin(); cit1 != cluster.end(); ++cit1) {
			if (*cit1 == *cit) {
				continue;
			}
			dist += (*m_baseMatrix)[*cit][*cit1];
		}
		if (dist < minDist) {
			minDist = dist;
			minIdx = *cit;
		}
	}
	return minIdx;
}

int Kmeans::GetAllCenters(const Result &cluster, std::vector<int> &nodes) {
	for (int i = 0; i < cluster.clusterResult.size(); ++i) {
		//枚举每个集合内部的点
		int minIdx = GetCenter(cluster.clusterResult[i]);
		nodes.push_back(minIdx);
	}
	return 0;
}

}

namespace YANGSHU {

using namespace CLUSTERALG;

static int __debug_level__ = 0;

ClusterOrganizer::ClusterOrganizer(BagPlan* bagPlan, std::vector<const SPath*> *rootList):m_bagPlan(bagPlan),m_rootList(rootList) {}

int ClusterOrganizer::GetPlanPlace(std::vector<const VarPlace *> &vList, int  loop) {
	//计算总的游玩天数和总游玩时长
	int canPlayDur = 0;
	for(int i = 0; i < m_bagPlan->GetBlockNum(); ++i) {//计算确实有剩余时间能够填充景点的block数量
		int blockDur = m_bagPlan->GetBlock(i)->_avail_dur;
		if(blockDur > 5400 and not m_bagPlan->GetBlock(i)->IsNotPlay()) {
			canPlayDur += blockDur;
		}
	}
	int seedId = loop % StaticRand::Capacity();
	double rval = StaticRand::Get(seedId) / static_cast<double>(StaticRand::Max());
	canPlayDur *= (1-rval*0.3); //因为一天之内无就餐了,会导致游玩时间变长
	int leftDur = canPlayDur;
	int oneDayDur = m_bagPlan->m_HotelOpenTime - m_bagPlan->m_HotelCloseTime;
	int totPoiNumLimit = static_cast<int>(canPlayDur*1.0/oneDayDur * (BagParam::m_dayViewLimit - 1));
	_INFO("oneDayDur:%d,canPlayDur:%d,totPoiNumLimit:%d",oneDayDur,canPlayDur,totPoiNumLimit);

	//选择
	float eps =0.000001;
	float noMissDropProbability = 0;
	float otherDropProbability = 0;
	int select = loop % 4;
	//(loop == 0) 路线加所有必去点 
	//	noMissDropProbability = 0;
	//	otherDropProbability = 0;
	if (loop == 1) {
		//代替原来的easyplan 和 limitplan
		noMissDropProbability = 1+eps;
		otherDropProbability = 1+eps;
	} else if (select == 0) {
		noMissDropProbability = 0.1;
		otherDropProbability = 0.02;
	} else if (select == 1) {
		noMissDropProbability = 0.2;
		otherDropProbability = 0.05;
	} else if (select == 2) {
		noMissDropProbability = 0.5;
		otherDropProbability = 0.2;
	} else if (select == 3) {
		noMissDropProbability = 0.9;
		otherDropProbability = 0.3;
	}
	if(m_bagPlan->qType == "ssv005_s130")
	{
		noMissDropProbability = loop*1.0/100;
		otherDropProbability = 1+eps;
	}

	std::tr1::unordered_set<uint8_t> addVPosSet;

	//必选点
	std::map<int,std::tr1::unordered_set<const LYPlace*>> category;
	category[0]=m_bagPlan->m_userMustPlaceSet;
	category[1]=m_bagPlan->m_userOptSet;
	category[2]=m_bagPlan->m_systemOptSet;
	category[3].insert(m_bagPlan->m_posPList.begin(),m_bagPlan->m_posPList.end());
	int zucheNum = 0;
	for(int i=0;i<category.size();i++)
	{
		auto searchSet = category[i];
		for (int vPos = 0; vPos < m_bagPlan->PosNum() ; ++vPos) {
			if (addVPosSet.count(vPos)) continue;
			const VarPlace* vPlace = dynamic_cast<const VarPlace*>(m_bagPlan->GetPosPlace(vPos));
			if (NULL == vPlace) continue;//keynode点都被跳过了
			if(m_bagPlan->m_failSet.count(vPlace)) continue;
			if (not searchSet.count(vPlace)) continue;
			//特例:当loop值为零时,加入所有必选点而不考虑游玩时长
			if (i == 0 and loop == 0) {
				addVPosSet.insert(vPos);
				vList.push_back(vPlace);
				continue;
			}
			if(addVPosSet.size() > totPoiNumLimit+zucheNum) break;
			if (leftDur < m_bagPlan->GetAllocDur(vPlace)) continue;
			//某种条件下随机删点
			//有概率跳过
			{
				double dropProbability = noMissDropProbability;
				if(i>0) dropProbability = otherDropProbability;
				double rval = StaticRand::Get(loop++) / static_cast<double>(StaticRand::Max());
				loop = loop % StaticRand::Capacity();
				if (rval < dropProbability) continue;
			}

			addVPosSet.insert(vPos);
			vList.push_back(vPlace);
			leftDur -= m_bagPlan->GetAllocDur(vPlace);
			if (vPlace->getRawType() & LY_PLACE_TYPE_CAR_STORE) {
				zucheNum++;
			}
		}
	}
	_INFO("total:%d,select:%d",m_bagPlan->PosNum(),vList.size());

	return 0;
}

int ClusterOrganizer::List2Matrix(const std::vector<const VarPlace *> &vList, CLUSTERALG::BaseMatrix &matrix) {
	for (int i = 0; i < vList.size(); ++i) { //填充地图信息
		const VarPlace *vPlace = vList[i];
		vector<int> list;
		for (int j = 0; j < vList.size(); ++j) {
			const VarPlace *wPlace = vList[j];
			int dist = m_bagPlan->GetDist(vPlace, wPlace);
			if(PlaceGroup::IsFreq(vPlace->_ID, wPlace->_ID)) {
				dist = 0;
			}
			if(dist < 0) {
				dist = 0;
			}
			list.push_back(dist);
		}
		matrix.push_back(list);
	}
	return 0;
}

int ClusterOrganizer::Run() {
	std::vector<NodeInfo> nodeInfo;	//每个节点的关系
	std::vector<BlockInfo> blockInfo;	//block信息
	const int loopNum = 100;
	for(int loop = 0; loop < loopNum; ++loop) {
		nodeInfo.clear();
		blockInfo.clear();
		//选点
		std::vector<const VarPlace *> vList;
		//选点逻辑需要同步
		GetPlanPlace(vList, loop);
		if(vList.size()==0) continue;
		//准备补入 m_bindInfo m_blockInfo
		InitInfo(vList, nodeInfo, blockInfo);
		int clusterNum = blockInfo.size();

		//选点去重，如果这些点已经选过一次，那么第二次出现时聚类结果肯定一样
		auto hash = CalHashPointSet(nodeInfo);
		if(HasVisit(hash)) {
			cerr << "has Same Cluster" << endl;
			continue;
		}
		// -------------------------
		if(__debug_level__ > 5) {
			cerr << "clusterNum = " << clusterNum << endl;
			for(int i = 0; i < vList.size(); i ++) {
				cerr << vList[i]->_name << endl;
			}
		}
		//转成矩阵
		CLUSTERALG::BaseMatrix matrix;
		List2Matrix(vList, matrix);

		if(__debug_level__ > 8) {
			for(int i = 0; i < matrix.size(); i ++) {
				for(int j = 0; j < matrix[i].size(); j ++) {
					cerr << matrix[i][j] << " ";
				}
				cerr << endl;
			}
		}

		//层次聚类,hash去重
		CLUSTERALG::HierarchicalClustering h(&matrix, clusterNum);
		h.Run();

		//将结果合并
		auto tmp = h.GetResult();
		CLUSTERALG::ClusterResult results;
		const int pathNumPerSet=20;
		int keepLimit = pow((loopNum-loop)*1.0/loopNum,5)*pathNumPerSet;
		keepLimit=std::min(keepLimit,pathNumPerSet);
		keepLimit=std::max(keepLimit,1);
		if(tmp.size()>keepLimit) tmp.resize(keepLimit);
		for(int i = 0; i < tmp.size(); ++i) {
			auto &t = tmp[i];
			CLUSTERALG::Result result;
			result.clusterResult.reserve(clusterNum);
			for(int j = i; j < t.clusterResult.size()+i; ++j) {
				int index = j % t.clusterResult.size();
				if( t.clusterResult[index].size() != 0 ){
					result.clusterResult.push_back(t.clusterResult[index]);
				}
			}
			//对于kmeans,初始类数必须等于clusterNum
			while(result.clusterResult.size() < clusterNum) {
				result.clusterResult.push_back(std::tr1::unordered_set<int>());
			}
			results.push_back(result);
		}

		if(__debug_level__ > 8) {
			ShowCluster(&results, vList);
		}

		CLUSTERALG::ClusterResult lastResults;
		for(int i = 0; i < results.size(); ++i) {
			if(__debug_level__ > 5) {
				cerr << "kmeans i = " << i << endl;
			}
			MyKmeans myKmeans(&(results[i]), &matrix, clusterNum, m_bagPlan, vList, nodeInfo, blockInfo);
			myKmeans.Run();
			auto &result = myKmeans.GetResult();
			if(result.size() > 0) {
				//cerr << "push result" << endl;
				lastResults.push_back(result[0]);
			}
		}
		//结果转成线路
		//cerr << "yangshu size = " << lastResults.size() << endl;
		ClusterDumpToSPath(lastResults, nodeInfo, blockInfo);
		if (m_bagPlan->IsRootTimeOut()) {
			MJ::PrintInfo::PrintLog("[%s]Cluster::DoCluster, TIMEOUT path length now: %d, iter: %d", m_bagPlan->m_qParam.log.c_str(), m_rootList->size(),loop);
			break;
		}
	}
	//最终线路筛选
	if(m_bagPlan->qType == "ssv005_s130")
	{
		//智能优化需要的计算量太大,需要减少路线数量;大约降低4倍
		std::vector<const SPath*> selectPaths;
		for(int j=0,k=0; j<m_rootList->size();j++)
		{
			int selectedIdx = pow(k,1.4);
			if(j==selectedIdx)
			{
				selectPaths.push_back((*m_rootList)[j]);
				k++;
			}
			else
			{
				delete (*m_rootList)[j];
			}
		}
		m_rootList->clear();
		m_rootList->insert(m_rootList->begin(),selectPaths.begin(),selectPaths.end());
	}

	//放入一个空行程保底
	{
		SPath *tmpPath = new SPath();
		tmpPath->Init(m_bagPlan);
		m_rootList->push_back(tmpPath);
	}
	std::sort(m_rootList->begin(), m_rootList->end(), SPathCmp());
	_INFO("kmeans results size:%d",m_rootList->size());
	for(int i = 0; i < m_rootList->size() && i < 5; ++i) {
		((*m_rootList)[i])->DumpDetial(m_bagPlan, true);
	}
	return 0;
}

int ClusterOrganizer::InitInfo(std::vector<const VarPlace *> &vList, std::vector<NodeInfo> &m_bindInfo, std::vector<BlockInfo> &m_blockInfo) {
	m_blockInfo.clear();
	m_bindInfo.clear();
	//确定最小景点的游玩时间
	int minPOIDur = 999999;
	for(int i = 0; i < vList.size(); ++i) {
		int dur = m_bagPlan->GetAllocDur(vList[i]);
		if(dur < minPOIDur) {
			minPOIDur = dur;
		}
	}
	//一天的游玩时间最少是2个小时,并且要多于最小景点的游玩时间
	minPOIDur = ((minPOIDur>5400)?minPOIDur:5400);

	//记录一天最多的游玩时间
	int maxBlockDur = -1;
	for(int i = 0; i < m_bagPlan->GetBlockNum(); ++i) {
		int blockDur = m_bagPlan->GetBlock(i)->_avail_dur;
		if(maxBlockDur < blockDur) {
			maxBlockDur = blockDur;
		}
		BlockInfo blockInfo;
		blockInfo.m_idx = i;
		blockInfo.m_dur = blockDur;
		blockInfo.m_is_not_play = m_bagPlan->GetBlock(i)->IsNotPlay();
		if (blockDur <= minPOIDur) {
			blockInfo.m_is_not_play = true;
		}
		if(blockDur >0) m_blockInfo.push_back(blockInfo);
	}

	for(int i = 0; i < vList.size(); ++i) {
		NodeInfo nodeInfo;
		nodeInfo.m_idx = 0;
		for(int j = 0; j < m_bagPlan->PosNum(); ++j) {
			if(vList[i] == m_bagPlan->GetPosPlace(j)) {
				nodeInfo.m_idx = j;
				break;
			}
		}
		nodeInfo.m_place = vList[i];
		nodeInfo.m_dur = m_bagPlan->GetAllocDur(vList[i]);
		//游玩时间超过最长的一天时,这一天为该景点的游玩时长
		if(nodeInfo.m_dur > maxBlockDur) {
			nodeInfo.m_dur = maxBlockDur;
		}
		for(int j = 0; j < vList.size(); ++j) {
			if(i == j) {
				nodeInfo.m_bindInfo.push_back(j);
			}
			else if(PlaceGroup::IsFreq(vList[i]->_ID, vList[j]->_ID)) {
				nodeInfo.m_bindInfo.push_back(j);
			}
		}
		m_bindInfo.push_back(nodeInfo);
	}

	sort(m_blockInfo.begin(), m_blockInfo.end(), BlockInfo::CmpOfDur);
	return 0;
}

int ClusterOrganizer::ShowCluster(const CLUSTERALG::ClusterResult *result, const std::vector<const VarPlace *> &vList) const {
	for(int i = 0; i < result->size(); ++i) {
		cerr << "Cluster: " << i << endl;
		auto &tmp = (*result)[i];
		for(int j = 0; j < tmp.clusterResult.size(); ++j) {
			cerr << "i: " << j << " :";
			for(auto it = tmp.clusterResult[j].begin(); it != tmp.clusterResult[j].end(); ++it) {
				cerr << vList[*it]->_name << " ";
			}
			cerr << endl;
		}
	}
	return 0;
}


int ClusterOrganizer::ClusterDumpToSPath(const CLUSTERALG::ClusterResult &results, const std::vector<NodeInfo> &m_bindInfo, const std::vector<BlockInfo> &m_blockInfo) {
	std::vector<const SPath*> legalPath;
	std::set<int> resultHashs;
	for(int i = 0; i < results.size(); ++i) {
		if (results[i].clusterResult.size() == 0) {
			continue;
		}
		if (m_blockInfo.size() == 0) {
			continue;
		}

		vector<int> tMatch(results[i].clusterResult.size(),0);
		SPath *tmpPath = new SPath();
		tmpPath->Init(m_bagPlan);
		for(int j = 0; j < tMatch.size(); ++j) {
			int cluster = j;
			if(tMatch[j] < 0) {
				continue;
			}
			int block = m_blockInfo[cluster].m_idx;
			//cerr << "block = " << block << "  cluster = " << cluster << endl;
			//cerr << "size = " << results[i].clusterResult.size() << endl;
			int allDur = 0;
			//cerr << cluster << " node:" << endl;
			for(auto it = results[i].clusterResult[cluster].begin(); it != results[i].clusterResult[cluster].end(); ++it) {
				//cerr << " " << *it;
				const VarPlace* vPlace = m_bindInfo[*it].m_place;
				int idx = m_bindInfo[*it].m_idx;
				int hot = vPlace ? vPlace->GetHotLevel(m_bagPlan->m_shopIntensity) : 0;
				int allocDur = m_bagPlan->GetAllocDur(vPlace);
				uint8_t vType = m_bagPlan->GetVType(block, idx);
				bit160 bid = m_bagPlan->GetPosBid(idx);
				tmpPath->PushV(block, idx, hot, allocDur, bid, vType);
				tmpPath->SetScore(tmpPath->m_hot);
				allDur += allocDur;
			}
			//cerr << endl;
			//cerr << "allDur = " << allDur << endl;
		}
		tmpPath->CalHashPointSet(m_bagPlan);
		tmpPath->CalHashDayOrder(m_bagPlan);
		if(resultHashs.find(tmpPath->Hash())==resultHashs.end())
		{
			resultHashs.insert(tmpPath->Hash());
			m_rootList->push_back(tmpPath);
		}
	}
	return 0;
}

int MyKmeans::KmeansAdjust() {
	//清空老的聚类
	for(int i = 0; i < m_clusterList.m_cluster.size(); ++i) {
		m_clusterList.m_cluster[i].m_node.clear();
	}

	//把每个点放新的聚类中
	for(int i = 0; i < m_bindInfo.size(); ++i) {
		int minClusterIndex = -1;
		int minDist = 99999999;
		for(int j = 0; j < GetClusterNum(); ++j) {
			if(m_P2Cluster[i][j] < minDist) {
				minDist = m_P2Cluster[i][j];
				minClusterIndex = j;
			}
		}
		//cerr << "minClusterIndex = " << minClusterIndex << endl;
		if( minClusterIndex != -1)//此处即kmeans的删点逻辑
			m_clusterList.m_cluster[minClusterIndex].m_node.push_back(i);
	}

	//重新计算游玩时间
	for(int i = 0; i < m_clusterList.m_cluster.size(); ++i) {
		int &allDur = m_clusterList.m_cluster[i].m_duration;
		allDur = 0;
		for(int j = 0; j < m_clusterList.m_cluster[i].m_node.size(); ++j) {
			//int dur = m_bagPlan->GetAllocDur(m_bindInfo[i].m_place);
			int dur = m_bindInfo[m_clusterList.m_cluster[i].m_node[j]].m_dur;
			allDur += dur;
		}
		m_clusterList.m_cluster[i].m_leftTime = m_blockInfo[i].m_dur - allDur;
	}

	//更新result
	m_clusterList.m_result.clusterResult.clear();
	for(int i = 0; i < m_clusterList.m_cluster.size(); ++i) {
		std::tr1::unordered_set<int> tmp;
		for(int j = 0; j < m_clusterList.m_cluster[i].m_node.size(); ++j) {
			tmp.insert(m_clusterList.m_cluster[i].m_node[j]);
		}
		m_clusterList.m_result.clusterResult.push_back(tmp);
	}
	return 0;
}

//点到集合之间的距离
int MyKmeans::CalPointToClusterDist(const vector<int> &vCenter) {
	for(int i = 0; i < vCenter.size(); ++i) {
		CalPointToClusterDistOneCluster(i, vCenter[i]);
	}
	return 0;
}

int MyKmeans::CalPointToClusterDistOneCluster(int cluster, int centerIdx) {
	if(cluster < 0) {
		cerr << "[yangshuError]: cluster:" << cluster << endl;
		return 0;
	}
	for(int i = 0; i < m_bindInfo.size(); ++i) {
		int idx = m_blockInfo[cluster].m_idx;
		int vPos = m_bindInfo[i].m_idx;
		if( m_blockInfo[cluster].m_is_not_play)
		{
			if(m_bindInfo[i].m_place->getRawType() == LY_PLACE_TYPE_CAR_STORE and m_bagPlan->GetVType(idx,vPos) == NODE_FUNC_PLACE_VIEW_SHOP)
			{
				m_P2Cluster[i][cluster] = 0;
			}
			else
			{
				m_P2Cluster[i][cluster] = std::numeric_limits<int>::max();//无穷大的意思是不能放
			}
		}
		else
		{
			if(m_bagPlan->GetVType(idx,vPos) == NODE_FUNC_NULL){
				m_P2Cluster[i][cluster] = std::numeric_limits<int>::max();
			}
			else {
				m_P2Cluster[i][cluster] = CalPointToClusterDistOneNode(i, centerIdx);
			}
		}

	}
	return 0;
}

int MyKmeans::ShowP2ClusterData() const {
	for(int i = 0; i < m_bindInfo.size(); ++i) {
		for(int j = 0; j < GetClusterNum(); ++j) {
			cerr << m_P2Cluster[i][j] << " ";
		}
		cerr << endl;
	}
	return 0;
}

int MyKmeans::CalPointToClusterDistOneNode(const int idxA, const int idxB) const {
	int dist = 0;
	int num = 0;
	if( idxA>=0 and idxA <m_bindInfo.size() and idxB>=0 and idxB<m_bindInfo.size() )
	{
		for(int i = 0; i < m_bindInfo[idxA].m_bindInfo.size(); ++i) {
			const auto &from = m_bindInfo[idxA].m_bindInfo[i];
			for(int j = 0; j < m_bindInfo[idxB].m_bindInfo.size(); ++j) {
				const auto &to = m_bindInfo[idxB].m_bindInfo[j];
				dist += GetDist(from, to);
				++num;
			}
		}
	}
	if(idxB<0) return 10000;//设置一个很大的值,防止所有点一下子全移到空block
	if(num ==0) return std::numeric_limits<int>::max(); //如果至少有一个点不存在,则认为这两个点无限远
	return dist/num;
}

int MyKmeans::GreedSwitchAllPoint() {
	std::vector<const ClusterNode*> legalClusterList;
	std::vector<const ClusterNode*> illegalClusterList;
	for(int i = 0; i < m_clusterList.m_cluster.size(); ++i) {
		if(m_clusterList.m_cluster[i].m_leftTime >= 0 && m_clusterList.m_cluster[i].m_node.size() <= BagParam::m_dayViewLimit) {
			legalClusterList.push_back(&(m_clusterList.m_cluster[i]));
		}
		else {
			illegalClusterList.push_back(&(m_clusterList.m_cluster[i]));
		}
	}

	if(illegalClusterList.size() == 0) {
		if(__debug_level__ > 3) {
			cerr << "has GreedSwitchAllPoint" << endl;
		}
		return 0;
	}

	//按照剩余时间从小到大排，在本列表中所有的剩余时间都为负数
	std::sort(illegalClusterList.begin(), illegalClusterList.end(), ClusterNode::LeftTimeCmpOfPoint);
	//for(int i = 0; i < illegalClusterList.size(); ++i) {
	//	cerr << illegalClusterList[i]->m_leftTime << endl;
	//}
	//exit(0);

	for (int i = 0; i < illegalClusterList.size(); i++) {
		const ClusterNode* illegalClusterNode = illegalClusterList[i];
		int minfromCIndex = illegalClusterNode->m_bidx;
		bool canSwitch = true;
		while(canSwitch && (illegalClusterNode->m_leftTime < 0 || illegalClusterNode->m_node.size() > BagParam::m_dayViewLimit)) {
			int minDist = 99999999;
			int minPIndex = -1;
			int minCToIndex = -1;
			int numLeast = 1;//普通cluster必须至少有一个点,否则无法计算一个点到cluster的距离
			if(m_blockInfo[illegalClusterNode->m_bidx].m_is_not_play) numLeast=0;//非游玩cluster可以特殊被搬空

			int groupDist = 300;
			bool careGroup[2] = {true,false};
			for(int r=0;r<2;r++)
			{
				bool fillEmpty = false;
				for(int pIndex = 0; not fillEmpty and pIndex < illegalClusterNode->m_node.size() and illegalClusterNode->m_node.size() >numLeast; ++pIndex) {
					int localPos = illegalClusterNode->m_node[pIndex];
					int dur = m_bindInfo[localPos].m_dur;
					bool groupExist =false;
					for(int k=0; k<illegalClusterNode->m_node.size(); k++)
					{
						int tmpPos = illegalClusterNode->m_node[k];
						if(m_P2Cluster[localPos][illegalClusterNode->m_bidx] == 0 ||
								k!=pIndex and GetDist(localPos,tmpPos) <= groupDist)
						{
							if(__debug_level__ > 8)
							{
								cerr<<"p2cluster[" << localPos << "][" << illegalClusterNode->m_bidx << "] == " << m_P2Cluster[localPos][illegalClusterNode->m_bidx] << std::endl;
								cerr<<"move skip "<< m_bagPlan->GetPosPlace(m_bindInfo[localPos].m_idx)->_name
									<<" because of "<< m_bagPlan->GetPosPlace(m_bindInfo[tmpPos].m_idx)->_name
									<< " the dist is: " << GetDist(localPos,tmpPos) <<std::endl;
							}
							groupExist =true;
							break;
						}
					}
					if(careGroup[r] and groupExist) continue;

					for(int j = 0; j < legalClusterList.size(); ++j) {
						const ClusterNode* legalClusterNode = legalClusterList[j];
						if(legalClusterNode->m_leftTime < dur ||legalClusterNode->m_node.size()>=BagParam::m_dayViewLimit) {
							continue;
						}
						int legalClusterIndex = legalClusterNode->m_bidx;
						int distP2C = m_P2Cluster[localPos][legalClusterIndex];
						if (distP2C < minDist) { //如果一个点在一个block不开门则,distP2C 为最大整数,所以一个点不会被移到不开门的天
							minDist = distP2C;
							minPIndex = localPos;
							minCToIndex = legalClusterIndex;
							if(legalClusterList.size() == 0)
							{
								fillEmpty = true;
								_INFO("fillEmpty");
								break;
							}
						}
					}
				}
				if(minPIndex >= 0) {
					MovePointFromClusterAToClusterB(minPIndex, minfromCIndex, minCToIndex);
					break;
				}
				else {
					canSwitch = false;
				}
			}
		}//while
	}
	return 0;
}


int MyKmeans::Run() {
	if(GetClusterNum() > m_initCluster->clusterResult.size()) {
		return 0;
	}
	//cerr << "YANGSHU m_bindInfo.size() = " << m_bindInfo.size() << endl;
	std::vector<int> vCenter;
	GetAllCenters(*m_initCluster, vCenter);
	if(__debug_level__ > 5) {
		ShowBaseMatrix();
	}
	vector<vector<int> > preCluster(m_initCluster->clusterResult.size());
	for(int i = 0; i < m_initCluster->clusterResult.size(); ++i) {
		std::vector<int> tmp;
		for(auto it = m_initCluster->clusterResult[i].begin(); it != m_initCluster->clusterResult[i].end(); ++it) {
			tmp.push_back(*it);
		}
		sort(tmp.begin(), tmp.end());
		preCluster.push_back(tmp);
	}
	sort(preCluster.begin(), preCluster.end());

	for (int i = 0; i < 10; ++i) {
		if(__debug_level__ > 8) {
			cerr << "Center:" << endl;
			for(int j = 0; j < vCenter.size(); ++j) {
				cerr << vCenter[j] << " ";
			}
			cerr << endl;
		}
		CalPointToClusterDist(vCenter);
		if(__debug_level__ > 9) {
			ShowP2ClusterData();
		}
		if(__debug_level__ > 5) {
			m_clusterList.ShowClusterInfo();
		}

		//RecordClusters
		KmeansAdjust();
		CalPointToClusterDist(vCenter);
		vCenter.clear();
		GetAllCenters(m_clusterList.m_result, vCenter);
		if(__debug_level__ > 5) {
			m_clusterList.ShowClusterInfo();
		}

		if(__debug_level__ > 5) {
			cerr << "first adjust" << endl;
		}
		//第一次调整
		GreedSwitchAllPoint();

		{
			//两次聚类结果一致时结束
			vector<vector<int> > newCluster(m_clusterList.m_result.clusterResult.size());
			for(int i = 0; i < m_clusterList.m_result.clusterResult.size(); ++i) {
				std::vector<int> tmp;
				for(auto it = m_clusterList.m_result.clusterResult[i].begin(); it != m_clusterList.m_result.clusterResult[i].end(); ++it) {
					tmp.push_back(*it);
				}
				sort(tmp.begin(), tmp.end());
				newCluster.push_back(tmp);
			}
			sort(newCluster.begin(), newCluster.end());
			if(newCluster == preCluster) {
				break;
			}
			preCluster = newCluster;
		}
		vCenter.clear();
		GetAllCenters(m_clusterList.m_result, vCenter);
	}
	SetResult(m_clusterList.m_result);
	return 0;
}

int MyKmeans::ClusterAddPoint(int clusterIdx, int nodeIdx) {
	auto &cluster = m_clusterList.m_cluster[clusterIdx];
	cluster.m_node.push_back(nodeIdx);
	m_clusterList.m_result.clusterResult[clusterIdx].insert(nodeIdx);
	cluster.m_duration += m_bindInfo[nodeIdx].m_dur;
	cluster.m_leftTime -= m_bindInfo[nodeIdx].m_dur;
	return 0;
}
int MyKmeans::ClusterSubPoint(int clusterIdx, int nodeIdx) {
	auto &cluster = m_clusterList.m_cluster[clusterIdx];
	auto it = find(cluster.m_node.begin(), cluster.m_node.end(), nodeIdx);
	cluster.m_node.erase(it);
	m_clusterList.m_result.clusterResult[clusterIdx].erase(nodeIdx);
	auto place = m_bagPlan->GetPosPlace(m_bindInfo[nodeIdx].m_idx);
	if(__debug_level__ > 8)
		std::cerr<< "from "<<clusterIdx<<" sub id: "<<place->_ID<<" name: "<<place->_name<<std::endl;
	cluster.m_duration -= m_bindInfo[nodeIdx].m_dur;
	cluster.m_leftTime += m_bindInfo[nodeIdx].m_dur;
	return 0;
}

int MyKmeans::MovePointFromClusterAToClusterB(int nodeIdx, int clusterAIdx, int clusterBIdx) {
	const auto &node = m_bindInfo[nodeIdx];
	//餐厅和景点分开处理
	if(__debug_level__ > 5) {
		cerr << "move before" << endl;
		m_clusterList.ShowClusterInfo();
	}
	//m_clusterList.ShowClusterInfo();
	//cerr << "in Sub" << endl;
	ClusterSubPoint(clusterAIdx, nodeIdx);
	//m_clusterList.ShowClusterInfo();
	//cerr << "in Add" << endl;
	ClusterAddPoint(clusterBIdx, nodeIdx);
	//m_clusterList.ShowClusterInfo();
	//exit(0);
	if(__debug_level__ > 5) {
		cerr << "move ret" << endl;
		m_clusterList.ShowClusterInfo();
	}
	//更新数据
	int minIdx = GetCenter(m_clusterList.m_result.clusterResult[clusterAIdx]);
	//m_clusterList.ShowClusterInfo();
	//cerr << "idx = " << clusterAIdx << " minIdx = " << minIdx << endl;
	CalPointToClusterDistOneCluster(clusterAIdx, minIdx);
	minIdx = GetCenter(m_clusterList.m_result.clusterResult[clusterBIdx]);
	CalPointToClusterDistOneCluster(clusterBIdx, minIdx);
	return 0;
}

int ClusterList::ShowClusterInfo() const {
	cerr << "ClusterList:" << endl;
	for(int i = 0; i < m_cluster.size(); ++i) {
		cerr << "i:" << i << " use: " << m_cluster[i].m_duration <<
		" left: " << m_cluster[i].m_leftTime << " rest: " << m_cluster[i].m_restNum << "info:";
		for(int j = 0; j < m_cluster[i].m_node.size(); ++j) {
			cerr << "  " << m_cluster[i].m_node[j];
		}
		cerr << endl;
		for(auto it = m_result.clusterResult[i].begin(); it != m_result.clusterResult[i].end(); ++it) {
			cerr << *it << " ";
		}
		cerr << endl;
	}
	return 0;
}

uint32_t ClusterOrganizer::CalHashPointSet(const std::vector<NodeInfo> &nodeInfo) {
	bit160 vPathBid;
	for (int i = 0; i < nodeInfo.size(); ++i) {
		uint8_t vPos = nodeInfo[i].m_idx;
		vPathBid |= m_bagPlan->GetPosBid(vPos);
	}
	return SPath::Hash(vPathBid);
}


}

