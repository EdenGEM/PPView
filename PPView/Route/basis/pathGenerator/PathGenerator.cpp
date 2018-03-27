#include "PathGenerator.h"
#include<iostream>
#include<limits>

#define TopK 3
bool debugInfo = false;
using namespace std;

struct point{
    int num;
    int pos;
    bool operator<(const point &a) const{
        return num<a.num;
    }
};

PathGenerator::PathGenerator(const Json::Value& routeJ, const Json::Value& controls) {
	m_inRouteJ = routeJ;
	m_controls = controls;
    m_isSearch = false;
    initPlayBorders();
}
bool PathGenerator::initPlayBorders() {
	const Json::Value& firstNode = m_inRouteJ[0u];
	const Json::Value& lastNode = m_inRouteJ[m_inRouteJ.size()-1];
	int rangeIdx = 0;
	if (firstNode.isMember("arrange") && firstNode["arrange"].isMember("controls") && firstNode["arrange"]["controls"].isMember("rangeIdx") && firstNode["arrange"]["controls"]["rangeIdx"].isInt()) {
		rangeIdx = firstNode["arrange"]["controls"]["rangeIdx"].asInt();
		if (rangeIdx < 0 || rangeIdx >= firstNode["fixed"]["times"].size()) rangeIdx = 0;
	}
    int left=m_inRouteJ[0u]["fixed"]["times"][rangeIdx][1].asInt();

	rangeIdx = 0;
	if (lastNode.isMember("arrange") && lastNode["arrange"].isMember("controls") && lastNode["arrange"]["controls"].isMember("rangeIdx") && lastNode["arrange"]["controls"]["rangeIdx"].isInt()) {
		rangeIdx = lastNode["arrange"]["controls"]["rangeIdx"].asInt();
		if (rangeIdx < 0 || rangeIdx >= lastNode["fixed"]["times"].size()) rangeIdx = 0;
	}
    int right=m_inRouteJ[m_inRouteJ.size()-1]["fixed"]["times"][rangeIdx][0u].asInt();
	m_playBorders = std::make_pair(left, right);
}
//判断天的边界本身是否有冲突
bool PathGenerator::isBordersConflict() {
	if (m_playBorders.second - m_playBorders.first < 0) {
		_INFO("playBorder is err: %d:00 - %d:00", m_playBorders.second/3600, m_playBorders.first/3600);
		return false;
	}
	return true;
}
bool PathGenerator::setNodeError (Json::Value& nodeJ) {
	Json::Value& lastNode = m_outRouteJ[m_outRouteJ.size()-1];
	std::string id = nodeJ["id"].asString();
	if (!lastNode["toNext"].isMember(id)) {
		_INFO("%s -> %s no traffic info", lastNode["id"].asString().c_str(), id.c_str());
		return false;
	}
	if (NodeJ::isFixed(nodeJ)) {
		int rangeIdx = 0;
		if (nodeJ.isMember("arrange") && nodeJ["arrange"].isMember("controls") && nodeJ["arrange"]["controls"].isMember("rangeIdx") && nodeJ["arrange"]["controls"]["rangeIdx"].isInt()) {
			rangeIdx = nodeJ["arrange"]["controls"]["rangeIdx"].asInt();
			if (rangeIdx < 0 || rangeIdx >= nodeJ["fixed"]["times"].size()) rangeIdx = 0;
		}
		const Json::Value& lastNodeTime = lastNode["arrange"]["time"];
		const Json::Value& nodeTimes = nodeJ["fixed"]["times"][rangeIdx];
		//上一个点的结束时间+交通时间
		int stime = lastNodeTime[1].asInt() + lastNode["toNext"][id].asInt();
		nodeJ["arrange"]["time"] = nodeTimes;
		//客观冲突判断
		//A B C   为了判断A C 是否颠倒
		isBackFixedConflict(nodeJ);
		if (stime <= nodeTimes[0u].asInt()) {
			//没出错
		} else {
			//锁定时刻无法到达
			NodeJ::setError(nodeJ,5);
		}
	}
	if (NodeJ::isFree(nodeJ)) {
		int rangeIdx = -1;
		//无rangeIdx 和 非法rangeIdx均认为不关心开关门
		if (nodeJ.isMember("arrange") && nodeJ["arrange"].isMember("controls") && nodeJ["arrange"]["controls"].isMember("rangeIdx") && nodeJ["arrange"]["controls"]["rangeIdx"].isInt()) {
			rangeIdx = nodeJ["arrange"]["controls"]["rangeIdx"].asInt();
			if (rangeIdx > 0 && rangeIdx >= nodeJ["free"]["openClose"].size()) {
				rangeIdx = -1;
			}
		}
		const Json::Value& lastNodeTime = lastNode["arrange"]["time"];
		int stime = lastNodeTime[1].asInt() + lastNode["toNext"][id].asInt();
		int dur = nodeJ["free"]["durs"][0u].asInt();
		if (rangeIdx == -1 || nodeJ["free"]["openClose"].size() == 0) {
			//不关心开关门
			NodeJ::setPlayRange(nodeJ, stime, stime + dur);
		} else {
			const Json::Value& nodeOpenClose = nodeJ["free"]["openClose"][rangeIdx];
			int openTime = nodeOpenClose[0u].asInt();
			int closeTime = nodeOpenClose[1].asInt();
			if (stime >= openTime && stime < closeTime) {
				NodeJ::setPlayRange(nodeJ, stime, stime + dur);
			} else if (stime < openTime) {
				NodeJ::setPlayRange(nodeJ, openTime, openTime + dur);
			} else if (stime >= closeTime) {
				NodeJ::setPlayRange(nodeJ, stime, stime + dur);
			}
		}
		if (!isOpen(nodeJ, rangeIdx)) {
			Json::FastWriter fw;
			NodeJ::setError(nodeJ, 4);
		}
	}
	if (m_outRouteJ.size()+1 != m_inRouteJ.size() && isConflictWithBorders(nodeJ)) {
		NodeJ::setError(nodeJ, 6);
	}
	return true;
}

bool PathGenerator::append(const Json::Value& nodeJ) {
	if(debugInfo) _INFO("append node: %s", nodeJ["id"].asString().c_str());
	m_outRouteJ.append(nodeJ);
}

int ErrorCount(Json::Value routeJ){
	int count=0;
	int len=routeJ.size();
	for(int i=0;i<len;i++){
		Json::Value nodeJ=routeJ[i];
		for(int j=0;j<7;j++){
			if(nodeJ["arrange"]["error"][j].asInt()==1)
				count++;
		}
	}
	return count;
}

Json::Value PathGenerator::popBack() {
	if (m_outRouteJ.size()>0) {
		Json::Value lastNode = m_outRouteJ[m_outRouteJ.size()-1];
		Json::Value tmp = Json::arrayValue;
		for (int i = 0; i < m_outRouteJ.size()-1; i++) {
			tmp.append(m_outRouteJ[i]);
		}
		m_outRouteJ = tmp;
		return lastNode;
	}
	return Json::Value();
}

bool PathGenerator::DayPathExpand() {
    if(debugInfo) cerr<<"before DayPathExpand"<<endl;
	if (!NodeJ::checkRouteJ(m_inRouteJ)) {
		_INFO("input routeJ is err");
		return false;
	}
	Json::Value nodeJ = m_inRouteJ[0u];
	int rangeIdx = 0;
	if (nodeJ.isMember("arrange") && nodeJ["arrange"].isMember("controls") && nodeJ["arrange"]["controls"].isMember("rangeIdx") && nodeJ["arrange"]["controls"]["rangeIdx"].isInt()) {
		rangeIdx = nodeJ["arrange"]["controls"]["rangeIdx"].asInt();
		if (rangeIdx < 0 || rangeIdx >= nodeJ["fixed"]["times"].size()) rangeIdx = 0;
	}
	nodeJ["arrange"]["time"] = nodeJ["fixed"]["times"][rangeIdx];
	append(nodeJ);
	for (int i = 1; i < m_inRouteJ.size(); i++) {
		nodeJ = m_inRouteJ[i];
		if (!setNodeError(nodeJ)) return false;
		append(nodeJ);
	}
	return true;
}
bool PathGenerator::DayPathExpandOpt() {
	if (!NodeJ::checkRouteJ(m_inRouteJ)) {
		_INFO("input routeJ is error");
		return false;
	}
	if (selectFixedPoiTimes()) {
        _INFO("expandopt success");
		//选取合理场次成功
	} else {
		m_outRouteJ.resize(0);
		_INFO("expandopt error, use default expand");
		DayPathExpand();
        _INFO("default expand over");
	}
    if(debugInfo) {
        Json::FastWriter fw;
        cerr<<"after selectFixedPoiTime"<<endl<<"m_outRouteJ: "<<fw.write(m_outRouteJ)<<endl;
    }
    for(int i=0;i<m_outRouteJ.size();i++) {
        Json::Value& nodeJ=m_outRouteJ[i];
        NodeJ::setDefaultError(nodeJ);
    }
    PathGenerator path(m_outRouteJ);
    path.DayPathExpand();
    m_outRouteJ=path.GetResult();
	selectOpenClose(m_outRouteJ);
	if (!m_controls.isMember("keepTime")) TimeEnrich();
	return true;
}
bool PathGenerator::selectFixedPoiTimes() {
    if(debugInfo) cerr<<"before selectFixedPoiTimes"<<endl;
	Json::Value firstNode = m_inRouteJ[0u];
	for (int tNum = 0; tNum < firstNode["fixed"]["times"].size(); tNum++) {
		firstNode["arrange"]["time"] = firstNode["fixed"]["times"][tNum];
		NodeJ::setRangeIdx(firstNode, tNum);
		append(firstNode);
		int fixIdx = 0;
		int i = 0;
		for (i = 1; i < m_inRouteJ.size(); i++) {
			Json::Value nodeJ = m_inRouteJ[i];
			if (NodeJ::isFree(nodeJ)) {
				NodeJ::setRangeIdx(nodeJ, -1);
				if(!setNodeError(nodeJ)) {
                    return false;
                }
			} else if (NodeJ::isFixed(nodeJ)) {
				fixIdx++;
				bool appendSuccess = setFixedPoiTimes(nodeJ);
				if (fixIdx ==1 && !appendSuccess && firstNode["fixed"]["times"].size()>1) {
					//边界与首个锁定点客观冲突
					m_outRouteJ.resize(0);
					break;
				}
				if (!appendSuccess) return false;
			}
			append(nodeJ);
		}
		if (i == m_inRouteJ.size()) {
			break;
		}
	}
	return true;
}
//选取fixed点合理的场次
bool PathGenerator::setFixedPoiTimes(Json::Value& nodeJ) {
	int j = 0;
	Json::Value tmpNode = nodeJ;
	for (j = 0; j < nodeJ["fixed"]["times"].size();j++) {
		tmpNode = nodeJ;
		NodeJ::setRangeIdx(tmpNode, j);
		if (!setNodeError(tmpNode)) {
			return false;
		}
		if (NodeJ::hasFixedConflictError(tmpNode) || !NodeJ::isFixedArvOnTime(tmpNode)) {
			continue;
		} else {
			break;
		}
	}
	nodeJ = tmpNode;
	if (j < nodeJ["fixed"]["times"].size()) {
		if (debugInfo) _INFO("select fixed times success, times tidx:%d", j);
		return true;
	} else {
		if (debugInfo) _INFO("select fixed times failed, all times has conflict");
		return false;
	}
}

bool PathGenerator::DayPathSearch() {
    m_isSearch=true;
	if (!NodeJ::checkRouteJ(m_inRouteJ)) {
		_INFO("input routeJ is error");
		return false;
	}
	MJ::MyTimer t;
	t.start();
	InitSearch();
	Search();
	SelectBestPath();
	int costTime = t.cost()/1000;
	if (m_outRouteJ.size() != m_inRouteJ.size()) {
		_INFO("search failed...");
		m_outRouteJ.resize(0);
		DayPathExpand();
	}
	_INFO("return... cost %d ms", costTime);
	return true;
}

bool PathGenerator::SelectBestPath() {
	std::vector<int> addOpenScore(TopK,0);
	int bestIdx = -1, idx = 0;
	float bestScore = 0.1;
	for (auto it = m_pathList.begin(); it != m_pathList.end(); it++) {
		Json::Value& pathRoute = it->second;
		Json::FastWriter fw;
		selectOpenClose(pathRoute);
		std::cerr << "final 3 path idx:" << idx << "  score:" << it->first << std::endl << fw.write(pathRoute) << std::endl;
		for (int i = 0; i < pathRoute.size(); i++) {
			const Json::Value& node = pathRoute[i];
			if (isOpen(node,-1)) {
				addOpenScore[idx]++;
			} else {
				if (debugInfo) std::cerr << "(not open)";
			}
			if (debugInfo) {
				std::cerr << node["id"].asString();
				if (i != pathRoute.size()-1) std::cerr << " - ";
				else std::cerr << std::endl;
			}
		}
        if(debugInfo) cerr<<"after add score: "<<addOpenScore[idx] + it->first<<endl;
		if ((addOpenScore[idx] + it->first) > bestScore) {
			bestScore = addOpenScore[idx] + it->first;
			bestIdx = idx;
		}
		idx++;
	}
	std::cerr << "select bestIdx:" << bestIdx << "  bestScore:" << bestScore << std::endl;
	auto it = m_pathList.begin();
	if (bestIdx != -1) {
		while(bestIdx !=0) {
			it++;
			bestIdx--;
		}
		m_outRouteJ = it->second;
	}
	return true;
}
//首尾点各两个开门
bool PathGenerator::Search() {
	Json::Value& firstNode = m_inRouteJ[0u];
	Json::Value& lastNode = m_inRouteJ[m_inRouteJ.size()-1];
	for (int i = 0; i < firstNode["fixed"]["times"].size(); i++) {
		NodeJ::setRangeIdx(firstNode, i);
		for (int j = 0; j < lastNode["fixed"]["times"].size(); j ++) {
			NodeJ::setRangeIdx(lastNode, j);

			initPlayBorders();
			m_outRouteJ.resize(0);

			Json::Value node = firstNode;
			node["arrange"]["time"] = firstNode["fixed"]["times"][i];
			append(node);
			m_screeningsSearchTime.start();
			DoSearch(0);
		}
	}
	return true;
}

bool PathGenerator::DoSearch(int depth) {
	if (debugInfo) _INFO("search depth:%d", depth);
	m_sCnt ++;
	if (m_sCnt > 20000) {
		if (IsScreeningsTimeOut() || IsTimeOut()) {
			std::cerr << "cost time:" << m_screeningsSearchTime.cost()/1000 << "  timelimit:" << BagParam::BagParam::m_dfsTimeOut/4 << std::endl;
			_INFO("search time out");
			return false;
		}
		m_sCnt = 0;
	}
	Json::Value lastNode = m_outRouteJ[m_outRouteJ.size()-1];
	if (m_outRouteJ.size() == m_poiList.size() + 1) {
		Json::Value node = m_inRouteJ[m_inRouteJ.size()-1];
		NodeJ::setDefaultError(node);
		if (!setNodeError(node)) {
            return false;
        }
		if (NodeJ::hasFixedConflictError(node) || !NodeJ::isFixedArvOnTime(node)) {
			return false;
		}
        /*
		if (NodeJ::hasFixedConflictError(node) && isFixedArvOnTime(node)) {
			return false;
		}*/
        if(debugInfo) cerr<<"last append"<<endl;
		append(node);
		m_trafTime += lastNode["toNext"][node["id"].asString()].asInt();
		if (debugInfo) {
			std::cerr << "success path trafTime: " << m_trafTime << std::endl;
			for (int size = 0; size < m_outRouteJ.size(); size++) {
				Json::Value& nodeJ = m_outRouteJ[size];
				std::cerr << nodeJ["id"].asString() << " - ";
			}
			std::cerr << std::endl;
			for (int size = 0; size < m_outRouteJ.size(); size++) {
				Json::FastWriter fw;
				Json::Value& nodeJ = m_outRouteJ[size];
//				std::cerr << "node: " << nodeJ["id"].asString() << fw.write(nodeJ) << std::endl;
			}
		}
		int blockTime = m_playBorders.second - m_playBorders.first;
		float score = 10*m_fixedNum + 3*m_freeNum - 1.0*blockTime/(24.0*3600) - 1.0*m_trafTime/(24.0*3600);
		if (m_pathList.size() == TopK) {
			auto it = m_pathList.begin();
			if (score > it->first) {
				m_pathList.insert(std::make_pair(score, m_outRouteJ));
			}
		} else {
			m_pathList.insert(std::make_pair(score, m_outRouteJ));
		}
		if (m_pathList.size() > TopK) {
			//删除最小得分路线
			auto it = m_pathList.begin();
			m_pathList.erase(it);
		}

		m_trafTime -= lastNode["toNext"][node["id"].asString()].asInt();
		popBack();
		return true;
	}
	for (int i = 0; i < m_poiList.size(); i++) {
		m_IdxVisited[depth][i] = false;
	}
	//只需要遍历(前一点)lastNode.toNext长度
	Json::Value::Members toNextList = lastNode["toNext"].getMemberNames();
	int i =0;
	for (auto it = toNextList.begin(); it != toNextList.end(); it++) {
		int nextIdx = -1;
		if (GetNextNodeByGreedy(lastNode, depth, nextIdx)) {
			if (nextIdx != -1) {
				m_IdxVisited[depth][nextIdx] =true;

				Json::Value node = m_poiList[nextIdx];
				NodeJ::setDefaultError(node);
                if(debugInfo)cerr<<"lastNode: "<<lastNode["id"].asString()<<" curNode: "<<node["id"].asString()<<endl;
				if (NodeJ::isFixed(node)) {
					//fixed 点基于场次搜索
					if (!setFixedPoiTimes(node)) {
                        continue;
                    }
				} else if (NodeJ::isFree(node)) {
					//设置free点为不关心开关门
					NodeJ::setRangeIdx(node, -1);
					if (!setNodeError(node)) {
                        continue;
                    }
				}
                if (isConflictWithBorders(node) && NodeJ::isNodeCanDel(node)) {
					if (debugInfo) _INFO("conflict with borders... continue");
					continue;
				}
				append(node);
				if (debugInfo) {
					Json::FastWriter fw;
					for (int size = 0; size < m_outRouteJ.size(); size++) {
						Json::Value& nodeJ = m_outRouteJ[size];
						std::cerr << nodeJ["id"].asString() << " - ";
					}
					std::cerr << std::endl;
					for (int size = 0; size < m_outRouteJ.size(); size++) {
						Json::Value& nodeJ = m_outRouteJ[size];
						std::cerr << "node: " << nodeJ["id"].asString() << fw.write(nodeJ) << std::endl;
					}
				}
				if (NodeJ::isFree(node)) {
					m_freeNum ++;
				} else if (NodeJ::isFixed(node)) {
					m_fixedNum ++;
				}
				m_trafTime += lastNode["toNext"][node["id"].asString()].asInt();
				m_visited[nextIdx] = true;

				DoSearch(depth+1);
				
				m_visited[nextIdx] = false;
				if (NodeJ::isFree(node)) {
					m_freeNum --;
				} else if (NodeJ::isFixed(node)) {
					m_fixedNum --;
				}
				m_trafTime -= lastNode["toNext"][node["id"].asString()].asInt();
				popBack();
			}
		}
	}
	return true;
}

bool PathGenerator::InitSearch() {
	for (int i = 1; i < m_inRouteJ.size()-1; i++) {
		Json::Value nodeJ = m_inRouteJ[i];
		m_poiList.append(nodeJ);
	}
	m_sCnt = 0;
	m_fixedNum = 0;
	m_freeNum = 0;
	m_trafTime = 0;
	m_searchTime.start();
	for (int i = 0; i < m_poiList.size(); i++) m_visited[i] = false;
}

bool PathGenerator::IsTimeOut() {
	if (m_searchTime.cost()/1000 > BagParam::BagParam::m_dfsTimeOut) return true;
	return false;
}

bool PathGenerator::IsScreeningsTimeOut() {
	if (m_screeningsSearchTime.cost()/1000 > BagParam::BagParam::m_dfsTimeOut/4) return true;
	return false;
}

bool PathGenerator::GetNextNodeByGreedy(const Json::Value& nodeJ, int idx, int& nextIdx) {
	int minTrafTime = std::numeric_limits<int>::max();
	if (!nodeJ.isMember("toNext") || nodeJ["toNext"].isNull()) {
		return false;
	}
	Json::Value::Members nextIdList = nodeJ["toNext"].getMemberNames();
	int idIndex = 0;
	std::string nextId = "";
	for (Json::Value::Members::iterator it = nextIdList.begin(); it != nextIdList.end(); ++it) {
		if (nodeJ["toNext"][*it].asInt() < minTrafTime) {
			if (GetIdxByIdstr(*it, idIndex)) {
				if (m_IdxVisited[idx][idIndex]) {
					continue;
				} else if (m_visited[idIndex]) {
					continue;
				}
			} else {
				//_INFO("nodeJ id: %s ,toNext has error id: %s",nodeJ["id"].asString().c_str(), (*it).c_str());
				continue;
			}
			minTrafTime = nodeJ["toNext"][*it].asInt();
			nextIdx = idIndex;
			nextId = *it;
		}
	}
	return true;
}
bool PathGenerator::GetIdxByIdstr(std::string id, int& index) {
	for (int i = 0; i < m_poiList.size(); i++) {
		Json::Value& nodeJ = m_poiList[i];
		if (nodeJ["id"].isString() && nodeJ["id"].asString() == id) {
			index = i;
			return true;
		}
	}
	return false;
}

bool PathGenerator::isConflictWithBorders(const Json::Value& nodeJ) {
    if(debugInfo) cerr<<" left time: "<<nodeJ["arrange"]["time"][1].asInt()<<" border right: "<<m_playBorders.second<<endl;
	if (nodeJ["arrange"]["time"][1].asInt() > m_playBorders.second) return true;
	return false;
}
bool PathGenerator::isBackFixedConflict(Json::Value& nodeJ) {
	int rangeIdx = 0;
	if (nodeJ.isMember("arrange") && nodeJ["arrange"].isMember("controls") && nodeJ["arrange"]["controls"].isMember("rangeIdx") && nodeJ["arrange"]["controls"]["rangeIdx"].isInt()) {
		rangeIdx = nodeJ["arrange"]["controls"]["rangeIdx"].asInt();
		if (rangeIdx < 0 || rangeIdx >= nodeJ["fixed"]["times"].size()) rangeIdx = 0;
	}
	const Json::Value& times = nodeJ["fixed"]["times"][rangeIdx];
	Json::Value lastNodeTime;
	int playDurTime = 0;
	for (int j = m_outRouteJ.size()-1; j>=0; j--) {
		const Json::Value& lastNode = m_outRouteJ[j];
		lastNodeTime = lastNode["arrange"]["time"];
		if (NodeJ::isFixed(lastNode)) {
			//顺序颠倒
			if (lastNodeTime[0u].asInt() > times[1].asInt() or
					(lastNodeTime[0u].asInt() == times[1].asInt() and (lastNodeTime[1]>lastNodeTime[0u] or times[1]>times[0u]))
			   ) {
				_INFO("lastNode: %s and the node %s, order error",lastNode["id"].asString().c_str(), nodeJ["id"].asString().c_str());
				NodeJ::setError(nodeJ, 0);
                if (m_isSearch) break;
			}
			//时间重叠
			else if (lastNodeTime[1].asInt() > times[0u].asInt()) {
				_INFO("lastNode: %s(etime:%d) and the node %s(stime:%d), time cover",lastNode["id"].asString().c_str(),lastNodeTime[1].asInt(), nodeJ["id"].asString().c_str(),times[0u].asInt());
				NodeJ::setError(nodeJ, 1);
                if (m_isSearch) break;
			}
			//可用时间不足
			else if (playDurTime > times[0u].asInt() - lastNodeTime[1].asInt()) {
				_INFO("lastNode: %s and the node %s, playDurTime: %d, lastNode etime:%d, the node stime:%d ,dur not enought",lastNode["id"].asString().c_str(), nodeJ["id"].asString().c_str(), playDurTime, lastNodeTime[1].asInt(), times[0u].asInt());
				NodeJ::setError(nodeJ, 2);
                if (m_isSearch) break;
			}
		} else {
			playDurTime += lastNodeTime[1].asInt() - lastNodeTime[0u].asInt();
		}
	}
	return NodeJ::hasFixedConflictError(nodeJ);
}

bool PathGenerator::isOpen(Json::Value nodeJ,int pos){
	if(NodeJ::isFixed(nodeJ)) return true;
	if (nodeJ["free"]["openClose"].size() == 0) return false;
    if(pos!=-1){
        Json::Value nodeOpenClose=nodeJ["free"]["openClose"][pos];
        int openTime=nodeOpenClose[0].asInt();
        int closeTime=nodeOpenClose[1].asInt();
        int stime=nodeJ["arrange"]["time"][0].asInt();
        int etime=nodeJ["arrange"]["time"][1].asInt();
		if (stime>=openTime-900 && stime<=closeTime
				&& etime>=openTime && etime<=closeTime+900
				&& etime - stime >= 900) {
            return true;
		}
        else
            return false;
    }
    else
    {
        for(int j=0;j<nodeJ["free"]["openClose"].size();j++){
            Json::Value nodeOpenClose=nodeJ["free"]["openClose"][j];
            int openTime=nodeOpenClose[0].asInt();
            int closeTime=nodeOpenClose[1].asInt();
            int stime=nodeJ["arrange"]["time"][0].asInt();
            int etime=nodeJ["arrange"]["time"][1].asInt();
			if (stime>=openTime-900 && stime<=closeTime
					&& etime>=openTime && etime<=closeTime+900
					&& etime - stime >= 900) {
                return true;
			}
        }
        return false;
    }
}

int PathGenerator::countOpenClose(Json::Value routeJ){
    int count=0;
    for(int i=0;i<routeJ.size();i++){
        Json::Value nodeJ=routeJ[i];
        if(nodeJ.isMember("free")){
            int rangeIdx=nodeJ["arrange"]["controls"]["rangeIdx"].asInt();
            if(isOpen(routeJ[i],rangeIdx)){
                count++;
            }
        }
    }
    return count;
}

Json::Value PathGenerator::selectRouteJ(vector<Json::Value> ResultRouteJList){
    int len=ResultRouteJList[0].size();
    int flag[256];
    memset(flag,0,sizeof(flag));
    vector<Json::Value> tempRouteJ,flagRouteJ;
    flagRouteJ=ResultRouteJList;
    for(int i=0;i<len;i++){
        if(flagRouteJ.size()==1)
            break;
        Json::Value routeJ=flagRouteJ[0];
        if(routeJ[i].isMember("fixed"))
            continue;
        for(int j=0;j<flagRouteJ.size();j++){
            routeJ=flagRouteJ[j];
            int rangeIdx=routeJ[i]["arrange"]["controls"]["rangeIdx"].asInt();
            if(isOpen(routeJ[i],rangeIdx)){
                tempRouteJ.push_back(routeJ);
            }
        }
        if(tempRouteJ.size()!=0){
            flagRouteJ.clear();
            flagRouteJ=tempRouteJ;
            tempRouteJ.clear();
        }
    }
    return flagRouteJ[0] ;
}

bool PathGenerator::selectOpenClose(Json::Value& routeJ){
	Json::FastWriter fw;
	MJ::MyTimer openCloseT;
	openCloseT.start();
    int fixId=0;
    for (int i=0;i<routeJ.size();i++) {
        if (NodeJ::isRightConflict(routeJ[i]) || NodeJ::hasFixedConflictError(routeJ[i])) {
            _INFO("select failed");
            return false;
        }
        if (NodeJ::isFixed(routeJ[i])) {
            if(i-fixId>1){
                vector<Json::Value> routeJList,ResultRouteJList;
                Json::Value segmentRouteJ;
                for(int j=fixId;j<=i;j++){
                    segmentRouteJ.append(routeJ[j]);
                }
                routeJList.push_back(segmentRouteJ);
                for(int j=1; j<segmentRouteJ.size(); j++){
                    for(int k=0;k<routeJList.size();k++){
                        Json::Value tempRouteJ=routeJList[k];
                        ResultRouteJList.push_back(tempRouteJ);
                        Json::Value OpenClose=tempRouteJ[j]["free"]["openClose"];
                        for(int z=0;z<OpenClose.size();z++){
                            tempRouteJ=routeJList[k];
                            Json::Value& nodeJ=tempRouteJ[j];
                            NodeJ::setDefaultError(nodeJ);
                            NodeJ::setRangeIdx(nodeJ,z);
                            PathGenerator path(tempRouteJ);
                            path.DayPathExpand();
                            tempRouteJ=path.GetResult();
							bool flag = false;
                            for (int tmpSize = tempRouteJ.size()-2; tmpSize >= 0; tmpSize--) {
                                if (path.isConflictWithBorders(tempRouteJ[tmpSize])) {
                                	flag = true;
									break;
								}
                            }
                            if(!NodeJ::isFixedArvOnTime(tempRouteJ[tempRouteJ.size()-1])){
                                flag=true;
                            }
							if (flag) break;
                            if(isOpen(tempRouteJ[j],z)) {
                                ResultRouteJList.push_back(tempRouteJ);
                                break;
							}
                        }
                    }
                    routeJList.clear();
                    routeJList=ResultRouteJList;
                    ResultRouteJList.clear();
                }
                int len=routeJList.size();
                if(debugInfo)
                    cerr<<"routeJList.size: "<<len<<endl;
                point count[len];
                for(int v=0;v<len;v++){
                    count[v].num=countOpenClose(routeJList[v]);
                    count[v].pos=v;
                }
                stable_sort(count,count+len);
                int maxnum=count[len-1].num;
                for(int v=routeJList.size()-1;v>=0;v--){
                    if(count[v].num==maxnum){
                        ResultRouteJList.push_back(routeJList[count[v].pos]);
                    }
                    else
                        break;
                }
                if(debugInfo)
                    cerr<<"ResultRouteJList.size: "<<ResultRouteJList.size()<<endl;
                segmentRouteJ=selectRouteJ(ResultRouteJList);
                if(debugInfo)
                cerr<<"segmentRouteJ: "<<fw.write(segmentRouteJ)<<endl;
                int num=1;
                for(int j=fixId+1;j<i;j++){
                    routeJ[j]=segmentRouteJ[num++];
                }
            }
            fixId=i;
        }
    }
	_INFO("select openclose cost: %d ms", openCloseT.cost()/1000);
    return true;
}

int PathGenerator::ErrorCount(Json::Value routeJ){
    int count=0;
    int len=routeJ.size();
    for(int i=0;i<len;i++){
        Json::Value nodeJ=routeJ[i];
        for(int j=0;j<7;j++){
            if(nodeJ["arrange"]["error"][j].asInt()==1)
                count++;
        }
    }
    return count;
}

bool PathGenerator::TimeEnrich() {
	Json::Value& routeJ = m_outRouteJ;
    Json::FastWriter fw;
    if(debugInfo) {
        cerr<<"before TimeEnrich: "<<fw.write(m_outRouteJ)<<endl;
    }
    int len=routeJ.size();
    int vis[len];
    int fixedId=0;
    memset(vis,0,sizeof(vis));
    for(int i=1;i<len;i++){
        if(routeJ[i].isMember("fixed")){
            if(i-fixedId>1){
                int num=0;
                Json::Value tempRouteJ;
                for(int j=fixedId;j<=i;j++){
                    tempRouteJ[num++]=routeJ[j];
                }

				int enrichSteps = 10;
				//u为拉伸次数
                for(int u = 1;u <= enrichSteps; u++) {
                    for(int j = 1; j < num-1; j++) {
                        Json::Value& lastNode=tempRouteJ[j-1];
                        Json::Value& curNode=tempRouteJ[j];
                        if(curNode.isMember("free")&&vis[j]==0){
                            int  time[3];
                            time[0]=curNode["free"]["durs"][0].asInt();
                            time[1]=curNode["free"]["durs"][1].asInt();
                            time[2]=curNode["free"]["durs"][2].asInt();
                            if(time[0]==time[1]&&time[1]==time[2]){
                                vis[j]=1;
                                continue;
                            }
							int oriTime = time[0];
							int stepTime = ((time[1]-time[0])/(1+enrichSteps-u))/900 * 900; //按照15分钟步长调整 小于900s不调整
							if (stepTime == 0) continue;
                            int oldErrorCount=ErrorCount(tempRouteJ);
                            Json::Value ResultRouteJ=tempRouteJ;
                            NodeJ::setDurs(ResultRouteJ[j],oriTime+stepTime,time[1],time[2]);
                            PathGenerator path(ResultRouteJ);
                            path.DayPathExpand();
                            ResultRouteJ=path.GetResult();

                            int newErrorCount=ErrorCount(ResultRouteJ);
                            if(debugInfo) cerr<<"j: "<<j<<" oldError: "<<oldErrorCount<<" newError: "<<newErrorCount<<endl;
                            if (newErrorCount>oldErrorCount) {
                                if(debugInfo) cerr<<"j: "<<j<<" enrich over"<<"u: "<<u<<" oriTime:"<<oriTime << "  stepTime:" << stepTime<<endl;
								if (debugInfo) {
									Json::FastWriter fw;
									cerr << "before enrich: " << fw.write(tempRouteJ) << std::endl;
									cerr << "after enrich : " << fw.write(ResultRouteJ) << std::endl;
								}
                                vis[j]=1;
                            }
                            else {
                                if(debugInfo) cerr<<"strenth j:"<<j<<"u: "<<u<<" oriTime:"<<oriTime << "  stepTime:" << stepTime<<endl;
                                tempRouteJ=ResultRouteJ;
                            }
                        }
                    }
                }
                num=1;
                for(int j=fixedId+1;j<i;j++){
                    routeJ[j]=tempRouteJ[num++];
                }
                if(debugInfo) {
                    cerr<<fixedId<<" to "<<i<<" timeEnrich over, routeJ: "<<fw.write(routeJ)<<endl;
                }
            }
            fixedId=i;
        }
    }   
    return true;
}
