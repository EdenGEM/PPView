#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdio.h>
#include "string"
#include "threads/linked_list.hpp"
#include "Route/base/Utils.h"
#include "MJCommon.h"


class Worker {
public:
	Worker();
	virtual ~Worker() { }

public:
	int m_fd;
	long long m_htpTime;
	timeval m_recvTime;

	std::string m_uri;
	std::string m_msg;

	std::string m_queryS;
	std::string m_retS;

	QueryParam m_qParam;
	linked_list_node_t m_taskListNode;

	static unsigned int m_wCount;
	std::string m_wid;
	std::string m_tid;

	std::string m_cIdxS;
	unsigned int m_cNum;

	//hy {
	std::string m_mqRetKey;
	//hy }
};


#endif

