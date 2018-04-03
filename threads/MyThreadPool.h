#ifndef __MY_THREAD_POOL_H__
#define __MY_THREAD_POOL_H__

#include <pthread.h>
#include <stdlib.h>
#include "threads/linked_list.hpp"
#include "threads/wait_list.hpp"
#include <vector>
#include <iostream>
namespace MJ{

class Worker
{
	public:
		Worker();
		virtual ~Worker(){}
	public:
		virtual int doWork()=0;
		virtual int doWork(void* p){return 0;}
	public:
		linked_list_node_t task_list_node;
		unsigned char _state;	//0:worker尚未被处理 1:worker已经被处理完
		unsigned char _id;
};


class MyThreadPool {
	public:
		MyThreadPool();
		virtual ~MyThreadPool();
		virtual int open(size_t thread_num, size_t stack_size);
		virtual int activate();
		virtual int add_worker(Worker* worker);
		//线程回收，等待所有线程任务完成
		virtual int wait_worker_done(const std::vector<Worker*>& workers);
		//判断是否任务全部完成
		virtual bool is_worker_done(const std::vector<Worker*>& workers);
		virtual void* bind_ptr();
	private:
		static void* run_svc(void *arg);
		virtual int svc();
		virtual int join();
		virtual int stop();
	protected:
		pthread_t *m_thread;
		size_t m_thread_num;
		pthread_barrier_t m_barrier;	
		wait_list_t<Worker, &Worker::task_list_node>  m_task_list;
	public:
		int routeID;
};




};	//MJ

#endif //__MY_THREAD_POOL_H__

