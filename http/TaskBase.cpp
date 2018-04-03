#include "TaskBase.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>


using namespace MJ;

TaskBase::TaskBase(): m_thread(NULL), m_thread_num(0)
{
}

TaskBase::~TaskBase()
{
	join();
}

int TaskBase::open(size_t thread_num, size_t stack_size)
{
	task_param_struct_p=new task_param_struct[thread_num];

	int ret = -1;
	size_t thread_idx;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	do {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (pthread_attr_setstacksize(&attr, stack_size))
			break;

		if (thread_num == 0 || (m_thread = (pthread_t*)malloc(thread_num * sizeof(pthread_t))) == NULL)
			break;

		pthread_barrier_init(&m_barrier, NULL, thread_num + 1);
		for (thread_idx=0; thread_idx<thread_num; thread_idx++)
		{
			task_param_struct_p[thread_idx].thread_idx=thread_idx;
			task_param_struct_p[thread_idx].task_bask_p=this;
			if (pthread_create(m_thread+thread_idx, &attr, run_svc, task_param_struct_p+thread_idx)){
				printf("pthread_create失败(%zd)\n", thread_idx);
				break;
			}
		}

		if ((m_thread_num = thread_idx) != thread_num)
			break;

		ret = 0;
	} while (false);

	pthread_attr_destroy(&attr);
	return ret;
}

int TaskBase::activate()
{
	pthread_barrier_wait(&m_barrier);
	return 0;
}

int TaskBase::join()
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

void* TaskBase::run_svc(void *arg)
{
	task_param_struct *p = (task_param_struct *)arg;
	pthread_barrier_wait(&(p->task_bask_p->m_barrier));
	p->task_bask_p->svc(p->thread_idx);
	return NULL;
}


