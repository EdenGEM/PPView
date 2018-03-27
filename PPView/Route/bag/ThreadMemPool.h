#ifndef _STATIC_MEM_POOL_H_
#define _STATIC_MEM_POOL_H_

#include <iostream>
#include <vector>
#include <tr1/unordered_set>
#include "SPath.h"
#include "SPathAlloc.h"
#include "SPathHeap.h"

class ThreadMemPool {
public:
	ThreadMemPool(int spAllocLimit, int richHeapLimit, int dfsHeapLimit) {
		m_spAlloc = new SPathAlloc(spAllocLimit);
		m_richHeap = new SPathHeap;
		m_sList.reserve(spAllocLimit);
		SetTopK(richHeapLimit, dfsHeapLimit);
	}
	~ThreadMemPool() {
		Release();
	}
private:
	int Release();
public:
	int Clear();
	SPathHeap* GetDFSHeap(uint32_t hash);
	int SetTopK(int richHeapLimit, int dfsHeapLimit);
public:
	SPathAlloc* m_spAlloc;  // 内存分配
	SPathHeap* m_richHeap;
	std::tr1::unordered_map<uint32_t, SPathHeap*> m_dfsHeapMap;
	std::vector<const SPath*> m_sList;  // 堆结果排序
private:
	int m_richHeapLimit;
	int m_dfsHeapLimit;
};

#endif
