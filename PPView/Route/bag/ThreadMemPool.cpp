#include <iostream>
#include "Route/base/BagParam.h"
#include "ThreadMemPool.h"

int ThreadMemPool::Release() {
	Clear();
	if (m_richHeap) {
		delete m_richHeap;
		m_richHeap = NULL;
	}
	if (m_spAlloc) {
		delete m_spAlloc;
		m_spAlloc = NULL;
	}
	return 0;
}

int ThreadMemPool::Clear() {
	m_richHeap->Clear();
	for (std::tr1::unordered_map<uint32_t, SPathHeap*>::iterator it = m_dfsHeapMap.begin(); it != m_dfsHeapMap.end(); ++it) {
		SPathHeap* dfsHeap = it->second;
		if (dfsHeap) {
			std::vector<const SPath*> sPathList;
			dfsHeap->CopyVal(sPathList);
			for (int i = 0; i < sPathList.size(); ++i) {
				const SPath* sPath = sPathList[i];
				if (sPath) {
					m_spAlloc->Delete(sPath);
				}
			}
			delete dfsHeap;
		}
	}
	m_dfsHeapMap.clear();
	m_sList.clear();
	return 0;
}

SPathHeap* ThreadMemPool::GetDFSHeap(uint32_t hash) {
	if (m_dfsHeapMap.find(hash) == m_dfsHeapMap.end()) {
		m_dfsHeapMap[hash] = new SPathHeap(BagParam::m_dfsSPSHashHeapLimit);
	}
	return m_dfsHeapMap[hash];
}

int ThreadMemPool::SetTopK(int richHeapLimit, int dfsHeapLimit) {
	m_richHeapLimit = richHeapLimit;
	m_dfsHeapLimit = dfsHeapLimit;
	if (m_richHeap) {
		m_richHeap->Init(m_richHeapLimit);
	}
	for (std::tr1::unordered_map<uint32_t, SPathHeap*>::iterator it = m_dfsHeapMap.begin(); it != m_dfsHeapMap.end(); ++it) {
		SPathHeap* dfsHeap = it->second;
		if (dfsHeap) {
			dfsHeap->Init(m_dfsHeapLimit);
		}
	}
	return 0;
}
