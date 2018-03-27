#ifndef __STATIC_RAND_H__
#define __STATIC_RAND_H__

#include <iostream>
#include <stdlib.h>

class StaticRand {
public:
	StaticRand() {
		m_rlist = NULL;
	}
	~StaticRand() {
	}
public:
	static int Init();
	static int Release();
	static int Get(int index);
	static int Max();
	static int Capacity();
private:
	static int* m_rlist;
	static const int m_capacity = 10000000;
	static const int m_max = RAND_MAX;
};
#endif

