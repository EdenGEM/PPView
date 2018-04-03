#include "MyThreadPool.h"
#include <signal.h>
#include <iostream>
#include <stdio.h>

using namespace std;

namespace MJ{

unsigned char worker_num=0;
Worker::Worker(){
	_state=0;
	_id = worker_num;
	worker_num = (++worker_num)%256;
}


MyThreadPool::MyThreadPool(): m_thread(NULL), m_thread_num(0)
{
}

MyThreadPool::~MyThreadPool()
{
	stop();
}

int MyThreadPool::open(size_t thread_num, size_t stack_size)
{
	int ret = -1;
	size_t i;
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

	pthread_attr_destroy(&attr);
	return ret;
}

int MyThreadPool::activate()
{
	pthread_barrier_wait(&m_barrier);
	return 0;
}

int MyThreadPool::join()
{
	if (m_thread) {
		for (size_t i=0; i<m_thread_num; i++) {
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

int MyThreadPool::add_worker(Worker* worker)
{
	m_task_list.put(*worker);
	return 0;       
}

int MyThreadPool::stop()
{
	m_task_list.flush();
	join();
	return 0;
}

void* MyThreadPool::bind_ptr()
{
	return NULL;
}

unsigned char current_thread_num=0;

int MyThreadPool::svc()
{
	Worker * worker;
	int queueLen;
	int thread_id = (int)(current_thread_num++);
	void* ptr = bind_ptr();
	printf("initialized ThreadPool <%d>\n", thread_id);
	while ((worker = m_task_list.get()) != NULL)
	{
		queueLen = m_task_list.len();
		//cerr<<"[Thread Pool Job Num]:"<<queueLen<<endl;
		//cerr<<"[Thread "<<thread_id<<"]:"<<(int)worker->_id<<endl;
		//doWork(worker);
		//fprintf(stderr,"RouteID:(%d) WorkerThreadID:(%d) WorkerID:(%d)\n",this->routeID,thread_id,worker->_id);
		if (ptr == NULL)
		{
			worker->doWork();
		}
		else
		{
			worker->doWork(ptr);
		}
		worker->_state = 1;
	}//while

	cerr<<"closed thread!!"<<endl;

	return 0;
}



int MyThreadPool::wait_worker_done(const std::vector<Worker*>& workers){
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


bool MyThreadPool::is_worker_done(const std::vector<Worker*>& workers){
	for (size_t i=0;i<workers.size();i++){
		if (workers[i]->_state==0)
			return false;
	}
	return true;
}











}	//namespace

