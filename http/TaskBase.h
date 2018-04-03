#ifndef __TASK_BASE_H__
#define __TASK_BASE_H__

#include <pthread.h>

namespace MJ{

struct task_param_struct;

class TaskBase {
	public:
		TaskBase();
		virtual ~TaskBase();

		virtual int open(size_t thread_num, size_t stack_size);
		virtual int activate();
		virtual int stop() = 0;
		virtual int svc(size_t idx) = 0;
		virtual int join();		

	private:
		static void* run_svc(void *arg);

	protected:
		pthread_t *m_thread;
		size_t m_thread_num;
		pthread_barrier_t m_barrier;		
		task_param_struct * task_param_struct_p;
};

struct task_param_struct{
	TaskBase * task_bask_p;
	size_t thread_idx;
};

}	//namespace MJ

#endif //__TASK_BASE_H__

