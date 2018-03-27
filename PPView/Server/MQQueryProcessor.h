#ifndef _QUERY_PROCESSOR_H_
#define _QUERY_PROCESSOR_H_

#include "http/TaskBase.h"
#include "worker.h"
#include <iconv.h>
#include <pthread.h>
#include <vector>
#include <map>
#include "MJCommon.h"
#include "Route/Route.h"
#include "threads/wait_list.hpp"

class MQQueryProcessor : public MJ::TaskBase{
public:
	MQQueryProcessor();
	~MQQueryProcessor();

	int Init(pthread_barrier_t* processorInit);
	int SubmitTask(const std::string& uri);
	bool IsReady();

	int stop();
	int svc(size_t idx);
private:
	int ParseWorker(Worker* worker);
	int ParseUri(const std::string& uri, std::vector<std::pair<std::string, std::string> >& retList);

protected:
	wait_list_t<Worker, &Worker::m_taskListNode>  m_taskList;
	pthread_barrier_t* m_processorInit;

	int m_threadID;
	pthread_mutex_t m_mutex;

	std::vector<Route*> m_routeList;
};

#endif


