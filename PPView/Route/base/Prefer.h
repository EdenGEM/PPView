#ifndef __PREFER_H__
#define __PREFER_H__

#include "MJCommon.h"

class Prefer {
public:
	Prefer() {
	}
	virtual ~Prefer() {
	}

	static int PreferMatch ();
	static int PreferExactMatch();
};

class CityPrefer : public Prefer {
public:

	CityPrefer() {
		m_intensityPrefer = 2;
		m_shopPreferType = 0;
		m_startTime = "08:00";
		m_stopTime = "20:00";
		m_trafficPrefer.push_back(0);
		m_trafficPrefer.push_back(1);
		m_trafficPrefer.push_back(2);
		m_trafficPrefer.push_back(3);
	}
	int Dump() {
		MJ::PrintInfo::PrintDbg("[Prefer] intensityPrefer %d, shopPreferType %d, startTime %s, stopTime %s",m_intensityPrefer, m_shopPreferType, m_startTime.c_str(), m_stopTime.c_str());
	}
	~CityPrefer() {
	}
public:
	int m_intensityPrefer;
	int m_shopPreferType;
	std::vector<int> m_shopSpecial;
	std::vector<int> m_trafficPrefer;

	std::string m_startTime;
	std::string m_stopTime;

	std::vector<std::string> m_shopTagList;
};

#endif
