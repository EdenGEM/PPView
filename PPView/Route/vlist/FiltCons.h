#ifndef _FILTCONS_H_
#define _FILTCONS_H_

#include <tr1/unordered_set>
#include <string>
#include <vector>

class FiltCons {
public:
	FiltCons() {
	}

	virtual ~FiltCons() {
	}

	std::vector<std::pair<int, std::tr1::unordered_set< std::string > > > m_tagList;
	std::vector<std::pair<int, std::pair<double, double> > > m_rangeList;
};
#endif
