#include <signal.h>
#include <iostream>
#include <stdio.h>
#include "Route/base/BagParam.h"
#include "MyThreadPool.h"

using namespace std;

unsigned char worker_num = 0;
MyWorker::MyWorker()
{
	_state = 0;
	_id = worker_num;
	worker_num = (++worker_num) % 256;
}

MyThreadPool::MyThreadPool(): m_thread(NULL), m_thread_num(0) {
	pthread_mutex_init(&m_mutex, NULL);
	m_threadID = 0;
	m_tmPoolList.clear();
}

MyThreadPool::~MyThreadPool()
{
	stop();
	pthread_mutex_destroy(&m_mutex);
}

int MyThreadPool::open(int thread_num, int stack_size)
{
	int ret = -1;
	int i;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	do {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (pthread_attr_setstacksize(&attr, stack_size))
			break;

		if (thread_num == 0 || (m_thread = (pthread_t*)malloc(thread_num * sizeof(pthread_t))) == NULL)
			break;

		pthread_barrier_init(&m_barrier, NULL, thread_num + 1);
		for (i=0; i<thread_num; i++)
			if (pthread_create(m_thread+i, &attr, run_svc, this))
				break;

		if ((m_thread_num = i) != thread_num)
			break;

		ret = 0;
	} while (false);

	for (int i = 0; i < thread_num; ++i) {
		ThreadMemPool* tmPool = new ThreadMemPool(BagParam::m_spAllocLimit, BagParam::m_richHeapLimit, BagParam::m_dfsHeapLimit);
		m_tmPoolList.push_back(tmPool);
	}

	pthread_attr_destroy(&attr);
	return ret;
}

int MyThreadPool::SetTopK(int richHeapLimit, int dfsHeapLimit) {
	for (int i = 0; i < m_tmPoolList.size(); ++i) {
		ThreadMemPool* tmPool = m_tmPoolList[i];
		tmPool->SetTopK(richHeapLimit, dfsHeapLimit);
	}
	return 0;
}

int MyThreadPool::activate()
{
	pthread_barrier_wait(&m_barrier);
	return 0;
}

int MyThreadPool::join()
{
	if (m_thread) {
		for (int i=0; i<m_thread_num; i++) {
			pthread_kill(m_thread[i], SIGTERM);
			pthread_join(m_thread[i], NULL);
		}
		free(m_thread);
		m_thread = NULL;
		pthread_barrier_destroy(&m_barrier);
	}
	return 0;
}

void* MyThreadPool::run_svc(void *arg)
{
	MyThreadPool *task = (MyThreadPool *)arg;
	pthread_barrier_wait(&task->m_barrier);
	task->svc();
	return NULL;
}

int MyThreadPool::add_worker(MyWorker* worker)
{
	m_task_list.put(*worker);
	return 0;
}

int MyThreadPool::stop()
{
	m_task_list.flush();
	join();
	for (int i = 0; i < m_tmPoolList.size(); ++i) {
		delete m_tmPoolList[i];
	}
	m_tmPoolList.clear();
	return 0;
}

int MyThreadPool::svc() {
	MyWorker * worker;
	int queueLen;

	pthread_mutex_lock(&m_mutex);
	int threadID = m_threadID++;
	pthread_mutex_unlock(&m_mutex);

	fprintf(stderr, "MyThreadPool::svc, initialized ThreadPool <%d>\n", threadID);

	while ((worker = m_task_list.get()) != NULL) {
		queueLen = m_task_list.len();
//		fprintf(stderr, "MyThreadPool::svc, start workerID: %d\tt%d\tleftNum: %d\n", static_cast<int>(worker->_id), threadID, queueLen);
		worker->doWork(m_tmPoolList[threadID]);
//		fprintf(stderr, "MyThreadPool::svc, done workerID: %d\n", static_cast<int>(worker->_id));
		worker->_state = 1;
	}

	fprintf(stderr, "MyThreadPool::svc, close ThreadPool <%d>\n", threadID);
	return 0;
}

int MyThreadPool::wait_worker_done(const std::vector<MyWorker*>& workers){
	int i=0;
	int len = workers.size();
	while(len>0){
		while(i<len && workers[i]->_state==1){
			i++;
		}
		if (i>=len){
			break;
		}else{
			usleep(2000);
		}
	}
	return 0;
}


bool MyThreadPool::is_worker_done(const std::vector<MyWorker*>& workers){
	for (int i=0;i<workers.size();i++){
		if (workers[i]->_state==0)
			return false;
	}
	return true;
}

