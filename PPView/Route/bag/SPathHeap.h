#ifndef _SPATH_HEAP_H_
#define _SPATH_HEAP_H_

#include <iostream>
#include <queue>
#include <functional>
#include "SPath.h"

class SPathQueue: public std::priority_queue<const SPath*, std::vector<const SPath*>, SPathCmp > {
public:
	SPathQueue(int capacity = 0) {
 		Reserve(capacity);
   	}
public:
	int Reserve(int capacity) {
		this->c.reserve(capacity);
   	}
	int Capacity() const {
		return this->c.capacity();
	}
	int Clear() {
		this->c.clear();
	}
	int CopyVal(std::vector<const SPath*>& retList) {
		retList.assign(this->c.begin(), this->c.end());
	}
};

// SPathHeap定义 自己不管理内存
class SPathHeap {
public:
	SPathHeap() {
		m_topK = 0;
	}
	SPathHeap(int topK) {
		Init(topK);
	}
	~SPathHeap() {
	}
public:
	int Init(int topK);
	int Length() const;
	bool Empty() const;
	const SPath* Push(const SPath* path, bool dupHashAllow = false);
	const SPath* Pop();
	const SPath* Top();
	bool CanPush(double score, int missLevel = 0, int clash = 0) const;
	int Clear();
	int CopyVal(std::vector<const SPath*>& retList);
private:
	SPathQueue m_queue;
	int m_topK;
	std::tr1::unordered_set<uint32_t> m_hashSet;
};

#endif
