#ifndef __QUERY_PROCESSOR_H__
#define __QUERY_PROCESSOR_H__

#include "http/TaskBase.h"
#include "http/HttpWorker.h"
#include <iconv.h>
#include <pthread.h>


namespace MJ{

class HttpServer;

class QueryProcessor : public TaskBase
{
public:
	friend class HttpServer;
	QueryProcessor();
	virtual ~QueryProcessor();
	virtual int open(size_t thread_num, size_t stack_size);//, pthread_barrier_t *processor_init);
	virtual int stop();
	//关联http和processor
	void link(HttpServer * server);

public:
	/*务必实现以下纯虚函数*/
	//processor初始化代码 子类必须实现  return=0表示正常 !=0表示初始化失败
	virtual int init(const void* config)=0;
	/*对外公共方法*/
	//设置服务的任务队列的长度报警阈值（超过则报警，说明有任务堵塞）
	void setTaskThreshold(const int threshold);
protected:
	/*务必实现以下纯虚函数*/
	//每个processor线程的业务部分代码 子类必须实现
	virtual int doWork(const HttpWorker* worker)=0;

	/*子类可能用到的方法*/
	//doWork()中调用以下函数用来返回结果给上游，并断开socket连接
	int writeResponse(const std::string& respStr,const HttpWorker* worker);
private:
	//提交任务到处理队列 -- 不需要重写
	int submitWorker(HttpWorker* worker);
	//线程任务 -- 不需要重写
	int svc(size_t idx);

protected:
	size_t m_threadCnt;
	int m_taskThreshold;	//任务队列长度超过m_taskThreshold则报警
	std::string m_ip;		//本机IP
	std::string m_pName;	//进程名称
private:
	HttpServer *m_httpserver;
	QueryProcessor *m_processor;
	wait_list_t<HttpWorker, &HttpWorker::task_list_node>  m_task_list;
};

}	//namespace MJ

#endif //__QUERY_PROCESSOR_H__


