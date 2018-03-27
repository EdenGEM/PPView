#ifndef _SPATH_ALLOC_H_
#define _SPATH_ALLOC_H_

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stack>
#include <tr1/unordered_set>
#include "SPath.h"

class SPathAlloc {
public:
	SPathAlloc(unsigned int initLen) {
		m_initLen = initLen;
		Expand(m_initLen);
	}
	virtual ~SPathAlloc() {
		while (!m_freeS.empty()) {
			delete m_freeS.top();
			m_freeS.pop();
		}
	}
public:
	SPath* New() {
		if (m_freeS.empty()) {
			Expand(m_initLen / 10 + 1);
		}
		if (m_freeS.empty()) return NULL;

		SPath* ret = m_freeS.top();
		m_freeS.pop();
		return ret;
	}
	int Delete(const SPath* p) {
		if (p == NULL) return 1;
		p->Clear();
		m_freeS.push(const_cast<SPath*>(p));
		return 0;
	}
private:
	int Expand(unsigned int expandLen) {
		for (int i = 0; i < expandLen; ++i) {
			m_freeS.push(new SPath);
		}
		return 0;
	}
private:
	unsigned int m_initLen;
public:
	std::stack<SPath*> m_freeS;
};
#endif
