#include "QueryProcessor.h"
#include "HttpServer.h"
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <algorithm>
#include "MJLog.h"
#include "MJHotUpdate.h"
#include "AuxTools.h"


using namespace std;
using namespace MJ;

QueryProcessor::QueryProcessor(){
	m_threadCnt = 0;
	m_taskThreshold = 20;
	m_ip = getLocalIP();
	if (m_ip.length()==0){
		MJ_LOG_ERROR("获取IP地址IP失败");
		m_ip = "unknown_ip_addr";
	}
	m_pName = getProcessName();
}

QueryProcessor::~QueryProcessor(){
}

void QueryProcessor::setTaskThreshold(const int threshold){
	m_taskThreshold = threshold;
}

void QueryProcessor::link(HttpServer * server){
	m_httpserver = server;
}

int QueryProcessor::submitWorker(HttpWorker * worker){
	//请求接收完毕
	worker->m_stat = 1;
	worker->m_requestStr = "";
	gettimeofday(&worker->m_recv_time, NULL);
	//worker->m_request->dump();
	m_task_list.put(*worker);
	return 0;
}



int QueryProcessor::open(size_t thread_num, size_t stack_size){
	m_threadCnt = thread_num;
	return TaskBase::open(thread_num, stack_size);
}

int QueryProcessor::stop(){
	m_task_list.flush();
	join();
	MJ_LOG_INFO("query process stop");
	return 0;
}

int QueryProcessor::writeResponse(const std::string& respStr, const HttpWorker* worker){
	HttpWorker* wk = const_cast<HttpWorker*>(worker);
	size_t len = respStr.length();
	char* buff = new char[len+200];
	sprintf(buff,"HTTP/1.1 200 OK\r\nContent-Type: application/json;charset=utf-8\r\nContent-Length: %zd\r\nConnection: close\r\n\r\n%s",len,respStr.c_str());
	wk->m_responseStr = buff;
	delete [] buff;

	//重新添加到epoll事件中，等待发送
	gettimeofday(&wk->m_done_time, NULL);
	if (wk->m_socket < 0){
		//socket断开 什么都不做
		MJ_LOG_ERROR("连接已断开,无法Response");
		return 1;
	}else{
		m_httpserver->mod_output_fd(wk->m_socket,wk);	
	}

	return 0;
}

int QueryProcessor::svc(size_t idx){
	HttpWorker * worker;
	printf("Processor<%zd> 启动\n", idx);
	while ((worker = m_task_list.get()) != NULL){
		//检查任务队列长度
		int len = m_task_list.len();
		if (len >= m_taskThreshold){
			struct timeval t;
			gettimeofday(&t,NULL);
			long ts = t.tv_sec*1000+t.tv_usec/1000;
			char wbuff[256];
			snprintf(wbuff, 255, "模块:%s 队列长度:%d", m_pName.c_str(), len); 
			logMJOB_Exception("ex1002", worker->m_request->_uid,worker->m_request->_csuid,worker->m_request->_qid,ts,
                m_ip,worker->m_request->_refer_id,worker->m_request->_cur_id,wbuff);
		}

		worker->m_threadIndex = idx;

		//设置共享数据指针
		MJHotUpdate::addSharedDataPtr(idx);

		//实际请求处理的入口
		doWork(worker);

		//移除共享数据指针
		MJHotUpdate::delSharedDataPtr(idx);

		//worker正式结束使命 待销毁
		worker->m_stat = 2;
	}//while

	MJ_LOG_INFO("Processor<%zd>线程退出",idx);

	return 0;
}

