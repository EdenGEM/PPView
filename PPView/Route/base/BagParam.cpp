#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include "MJCommon.h"
#include "Utils.h"
#include "BagParam.h"

std::string BagParam::m_confName = "bagParam.conf";
std::string BagParam::m_cbPName = "cityBParam";
std::tr1::unordered_map<std::string, std::vector<std::pair<int, CityBParam> > > BagParam::m_cbPMap;
const CityBParam BagParam::m_minCBParam(0.5, 0.5, 100, 1000, 1000, 100, 64800, 2, 2);

int BagParam::m_rootHeapLimit = 100000;

// Bag
double BagParam::m_minPushScale = 0.5;
double BagParam::m_maxPushScale = 0.85;
double BagParam::m_diffPushScale = 0.1;
int BagParam::m_rootHotGap = 11;
int BagParam::m_rootRetLimit = 1000;

//筛包参数
double BagParam::m_bagWeightA = 0.5;
double BagParam::m_bagThre = 0.5;
// rich
int BagParam::m_richHeapLimit = 100000;
int BagParam::m_richMissLimit = 2;
int BagParam::m_richExtraDur = 18 * 3600;
int BagParam::m_richTopK = 2;

// dfs
int BagParam::m_dfsHeapLimit = 100000;
int BagParam::m_dfsDayHeapLimit = 1;
int BagParam::m_dfsSPSHashHeapLimit = 2;

// 全局
int BagParam::m_spAllocLimit = std::max(BagParam::m_richHeapLimit, BagParam::m_dfsHeapLimit) * 1.5;
int BagParam::m_bgPoolLimit = BagParam::m_rootHeapLimit * 2 + 100;
int BagParam::m_timeOut = 1000 * 60 * 2;  // 单位毫秒
int BagParam::m_queueTimeOut = 10 * 1000; // 单位毫秒
int BagParam::m_rootTimeOut = 1000 * 30;  // 单位毫秒
int BagParam::m_richTimeOut = 1000 * 30;  // 单位毫秒
int BagParam::m_dfsTimeOut = 1000 * 10;  // 单位毫秒
int BagParam::m_routeTimeOut = 1000 * 30;  // 单位毫秒
int BagParam::m_threadNum = 14;
int BagParam::m_dayViewLimit = 9;
bool BagParam::m_useCityBP = true;

int BagParam::Init(const std::string& dataPath) {
	int ret = 0;
	ret = InitBase(dataPath);
	if (ret == 0 && m_useCityBP) {
		ret = InitCBP(dataPath);
	}
	return ret;
}

int BagParam::InitBase(const std::string& dataPath) {
	std::string filePath = dataPath + "/" + BagParam::m_confName;
	MJ::PrintInfo::PrintLog("BagParam::InitBase, Load BagParam conf: %s", filePath.c_str());
	std::ifstream fin(filePath.c_str());
	if (!fin) {
		MJ::PrintInfo::PrintErr("BagParam::InitBase, BagParam Init failed!");
		return 1;
	}
	std::string line;
	while (std::getline(fin, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::string::size_type pos = line.find("=");
		if (pos == std::string::npos) {
			MJ::PrintInfo::PrintErr("BagParam::InitBase, format err line: %s", line.c_str());
			continue;
		}
		std::string key = line.substr(0, pos);
		std::string val = line.substr(pos + 1);
		if (key == "m_rootHeapLimit") {
			m_rootHeapLimit = atoi(val.c_str());
		} else if (key == "m_richHeapLimit") {
			m_richHeapLimit = atoi(val.c_str());
		} else if (key == "m_dfsHeapLimit") {
			m_dfsHeapLimit = atoi(val.c_str());
		} else if (key == "m_dfsDayHeapLimit") {
			m_dfsDayHeapLimit = atoi(val.c_str());
		} else if (key == "m_dfsSPSHashHeapLimit") {
			m_dfsSPSHashHeapLimit = atoi(val.c_str());
		} else if (key == "m_rootRetLimit") {
			m_rootRetLimit = atoi(val.c_str());
		} else if (key == "m_richExtraDur") {
			m_richExtraDur = atoi(val.c_str());
		} else if (key == "m_richTopK") {
			m_richTopK = atoi(val.c_str());
		} else if (key == "m_richMissLimit") {
			m_richMissLimit = atoi(val.c_str());
		} else if (key == "m_threadNum") {
			m_threadNum = atoi(val.c_str());
		} else if (key == "m_dayViewLimit") {
			m_dayViewLimit = atoi(val.c_str());
		} else if (key == "m_timeOut") {
			m_timeOut = atoi(val.c_str());
		} else if (key == "m_rootTimeOut") {
			m_rootTimeOut = atoi(val.c_str());
		} else if (key == "m_richTimeOut") {
			m_richTimeOut = atoi(val.c_str());
		} else if (key == "m_routeTimeOut") {
			m_routeTimeOut = atoi(val.c_str());
		}
	}
	m_bgPoolLimit = m_rootHeapLimit * 2 + 100;
	m_spAllocLimit = std::max(m_rootHeapLimit, m_richHeapLimit) * 1.5;
	MJ::PrintInfo::PrintLog("BagParam::InitBase, m_rootHeapLimit: %d, m_richHeapLimit: %d, m_dfsHeapLimit: %d, m_rootRetLimit: %d, m_richExtraDur: %d, m_richTopK: %d, m_richMissLimit: %d, m_threadNum: %d, m_dayViewLimit: %d, m_timeOut: %d, m_bgPoolLimit: %d, m_spAllocLimit: %d", m_rootHeapLimit, m_richHeapLimit, m_dfsHeapLimit, m_rootRetLimit, m_richExtraDur, m_richTopK, m_richMissLimit, m_threadNum, m_dayViewLimit, m_timeOut, m_bgPoolLimit, m_spAllocLimit);
	fin.close();
	return 0;
}

int BagParam::InitCBP(const std::string& dataPath) {
	std::string filePath = dataPath + "/" + BagParam::m_cbPName;
	MJ::PrintInfo::PrintLog("BagParam::InitCBP, Load city Bag Param: %s", filePath.c_str());
	std::ifstream fin(filePath.c_str());
	if (!fin) {
		MJ::PrintInfo::PrintErr("BagParam::InitCBP, BagParam Init failed!");
		return 1;
	}
	std::string line;
	while (std::getline(fin, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::string::size_type pos = line.find("\t");
		std::string cKey;
		std::string cValue;
		std::string cid;
		int level;
		if (pos != std::string::npos) {
			cKey = line.substr(0, pos);
			cValue = line.substr(pos + 1);
			pos = cKey.find("_");
			if (pos != std::string::npos) {
				cid = cKey.substr(0, pos);
				level = atoi(cKey.substr(pos + 1).c_str());
			}
		}
		if (pos == std::string::npos) {
			MJ::PrintInfo::PrintErr("BagParam::InitCBP, format err line: %s", line.c_str());
			continue;
		}
		if (m_cbPMap.find(cid) == m_cbPMap.end()) {
			m_cbPMap[cid] = std::vector<std::pair<int, CityBParam> >();
		}

		CityBParam cbParam;
		std::vector<std::string> itemList;
		ToolFunc::sepString(cValue, ",", itemList);
		if (itemList.size() != 9) {
			MJ::PrintInfo::PrintErr("BagParam::InitCBP, format err line: %s", line.c_str());
			continue;
		}
		for (int i = 0; i < itemList.size(); ++i) {
			std::string item = itemList[i];
			pos = item.find(":");
			if (pos == std::string::npos) continue;
			std::string pCode = item.substr(0, pos);
			std::string pValueS = item.substr(pos + 1);
			if (pCode == "a") {
				cbParam.m_cbBagWeightA = atof(pValueS.c_str());
			} else if (pCode == "c"){
				cbParam.m_cbBagThre = atof(pValueS.c_str());
			} else if (pCode == "P1") {
				cbParam.m_cbRootHeapLimit = std::min(atoi(pValueS.c_str()), BagParam::m_rootHeapLimit);
			} else if (pCode == "P2") {
				cbParam.m_cbRichHeapLimit = std::min(atoi(pValueS.c_str()), BagParam::m_richHeapLimit);
			} else if (pCode == "P3") {
				cbParam.m_cbDFSHeapLimit = atoi(pValueS.c_str());
			} else if (pCode == "P4") {
				cbParam.m_cbRootRetLimit = std::min(atoi(pValueS.c_str()), BagParam::m_rootRetLimit);
			} else if (pCode == "P5") {
				cbParam.m_cbRichExtraDur = atoi(pValueS.c_str());
			} else if (pCode == "P6") {
				cbParam.m_cbRichTopK = atoi(pValueS.c_str());
			} else if (pCode == "P7") {
				cbParam.m_cbRichMissLimit = atoi(pValueS.c_str());
			}
		}
		m_cbPMap[cid].push_back(std::pair<int, CityBParam>(level, cbParam));
	}
	fin.close();

	for (std::tr1::unordered_map<std::string, std::vector<std::pair<int, CityBParam> > >::iterator it = m_cbPMap.begin(); it != m_cbPMap.end(); ++it) {
		std::vector<std::pair<int, CityBParam> >& cbParamList = it->second;
		std::sort(cbParamList.begin(), cbParamList.end(), sort_pair_first<int, CityBParam, std::less<int> >());
//		for (int i = 0; i < cbParamList.size(); ++i) {
//			std::cerr << "jjj " << it->first << " " << cbParamList[i].first << std::endl;
//		}
	}
	MJ::PrintInfo::PrintLog("BagParam::InitCBP, Load key num: %d", m_cbPMap.size());
	return 0;
}

CityBParam::CityBParam() {
	m_cbBagWeightA = BagParam::m_bagWeightA;
	m_cbBagThre = BagParam::m_bagThre;
	m_cbRootHeapLimit = BagParam::m_rootHeapLimit;
	m_cbRichHeapLimit = BagParam::m_richHeapLimit;
	m_cbDFSHeapLimit = BagParam::m_dfsHeapLimit;
	m_cbRootRetLimit = BagParam::m_rootRetLimit;
	m_cbRichExtraDur = BagParam::m_richExtraDur;
	m_cbRichTopK = BagParam::m_richTopK;
	m_cbRichMissLimit = BagParam::m_richMissLimit;
}
