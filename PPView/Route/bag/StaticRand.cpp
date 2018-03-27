#include <iostream>
#include "StaticRand.h"

int* StaticRand::m_rlist = NULL;

int StaticRand::Max() {
	return m_max;
}

int StaticRand::Capacity() {
	return m_capacity;
}

int StaticRand::Get(int index) {
	return m_rlist[index % m_capacity];
}

int StaticRand::Release() {
	if (m_rlist) {
		delete[] m_rlist;
		m_rlist = NULL;
	}
	return 0;
}

int StaticRand::Init() {
	Release();
	m_rlist = new int[m_capacity];

	srand(1);
	for (int i = 0; i < m_capacity; ++i) {
		m_rlist[i] = rand();
	}
	return 0;
}
