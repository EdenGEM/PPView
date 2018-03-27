#ifndef _MY_THREAD_POOL_H_
#define _MY_THREAD_POOL_H_

#include <pthread.h>
#include <stdlib.h>
#include <vector>
#include "threads/wait_list.hpp"
#include "SPathAlloc.h"
#include "SPathHeap.h"
#include "ThreadMemPool.h"
#include <iostream>
#include "threads/linked_list.hpp"
#include "ThreadMemPool.h"

class MyWorker {
	public:
		MyWorker();
		virtual ~MyWorker() {
		}
	public:
		virtual int doWork(ThreadMemPool* tmPool)=0;
	public:
		linked_list_node_t task_list_node;
		unsigned char _state;	//0:worker尚未被处理 1:worker已经被处理完
		unsigned char _id;
};

class MyThreadPool {
	public:
		MyThreadPool();
		virtual ~MyThreadPool();
		virtual int open(int thread_num, int stack_size);
		virtual int activate();
		virtual int add_worker(MyWorker* worker);
		//线程回收，等待所有线程任务完成
		virtual int wait_worker_done(const std::vector<MyWorker*>& workers);
		//判断是否任务全部完成
		virtual bool is_worker_done(const std::vector<MyWorker*>& workers);
		int SetTopK(int richHeapLimit, int dfsHeapLimit);
	private:
		static void* run_svc(void *arg);
		virtual int svc();
		virtual int join();
		virtual int stop();
		int merge();
	protected:
		pthread_t *m_thread;
		pthread_barrier_t m_barrier;
		wait_list_t<MyWorker, &MyWorker::task_list_node>  m_task_list;

		pthread_mutex_t m_mutex;
		int m_threadID;
		std::vector<ThreadMemPool*> m_tmPoolList;
	public:
		int m_thread_num;
};
#endif //__MY_THREAD_POOL_H__

