#ifndef  __HTTP_WORKER_H__
#define  __HTTP_WORKER_H__

#include <string.h>
#include "http/HttpDefine.h"
#include "threads/wait_list.hpp"

namespace MJ{

class HttpWorker{
public:
	int m_socket;
	int m_stat;		//0:新连接 等待收消息 1:消息已接收，等待处理 2:处理完毕，等待发送
	
	//response相关参数
	std::string m_responseStr;
	int m_sendCnt;

	timeval m_conn_time;	//连接建立的时间点
	timeval m_recv_time;	//请求接收完毕，放入任务队列的时间点
	timeval m_done_time;	//任务处理完毕，等待发送的时间点
	linked_list_node_t task_list_node;

	//http request相关
	std::string m_requestStr;	//生成结构化数据后会清空
	MJHttpRequest* m_request;

	//处理线程相关参数
	size_t m_threadIndex;
public:
	HttpWorker(int socket);
	~HttpWorker();
	bool isAlive();
};

}	//namespace MJ

#endif //__HTTP_WORKER_H__

