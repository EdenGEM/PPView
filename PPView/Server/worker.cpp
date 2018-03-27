#include <iostream>
#include "worker.h"

unsigned int Worker::m_wCount = 0;

Worker::Worker() {
	char buf[100];
	snprintf(buf, sizeof(buf), "w%zu", m_wCount);
	m_wid.assign(buf);
	m_wCount = (m_wCount + 1) % 10000;
	m_tid = "";
}
