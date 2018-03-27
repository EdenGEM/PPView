#include <iostream>
#include <limits>
#include "SPathHeap.h"

int SPathHeap::Length() const {
	return m_queue.size();
}

bool SPathHeap::Empty() const {
	return m_queue.empty();
}

const SPath* SPathHeap::Pop() {
	if (m_queue.empty()) {
		return NULL;
	}
	const SPath* ret = m_queue.top();
	m_queue.pop();
	return ret;
}

const SPath* SPathHeap::Top() {
	if (m_queue.empty()) {
		return NULL;
	}
	const SPath* ret = m_queue.top();
	return ret;
}

bool SPathHeap::CanPush(double score, int missLevel, int clash) const {
	if (m_queue.size() < m_topK) return true;
	if (missLevel < m_queue.top()->m_missLevel) return true;
	if (missLevel > m_queue.top()->m_missLevel) return false;
	if (clash < m_queue.top()->m_clash) return true;
	if (clash > m_queue.top()->m_clash) return false;
	return (score > m_queue.top()->m_score - 0.000001);
}

int SPathHeap::Init(int topK) {
	m_topK = topK;
	m_queue.Reserve(m_topK);
	return 0;
}

int SPathHeap::Clear() {
	m_queue.Clear();
	m_hashSet.clear();
}

int SPathHeap::CopyVal(std::vector<const SPath*>& retList) {
	m_queue.CopyVal(retList);
}

// 如果不允许重复，或未计算Hash，显示将dupHashAllow置为true
const SPath* SPathHeap::Push(const SPath* sPath, bool dupHashAllow) {
	bool isPush = false;
	if (m_queue.size() < m_topK) {
		isPush = true;
	} else if (SPathCmp()(sPath, m_queue.top())) {
		isPush = true;
	} else {
		isPush = false;
	}
	if (!dupHashAllow) {
		if (isPush) {
			if (m_hashSet.find(sPath->Hash()) != m_hashSet.end()) {
				isPush = false;
			} else {
				m_hashSet.insert(sPath->Hash());
			}
		}
	}
	if (!isPush) return sPath;

	const SPath* popSPath = NULL;
	if (m_queue.size() >= m_topK) {
		popSPath = m_queue.top();
		m_queue.pop();
	}
	m_queue.push(sPath);
	return popSPath;
}
