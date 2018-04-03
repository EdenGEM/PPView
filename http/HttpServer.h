#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "TaskBase.h"
#include "HttpWorker.h"
#include <sys/epoll.h>
#include <string>
#include <tr1/unordered_set>


namespace MJ{

#define MAX_EPOLL_EVENT_NUM     256
#define MAX_FD_NUM              1024		//epoll下无意义 大于1即可
#define SOCKET_SND_BUF_SIZE     (128*1024)		//Socket发送缓冲区
#define SOCKET_RCV_BUF_SIZE     (128*1024)		//Socket接收缓冲区
#define READ_BUF_SIZE       	(128*1024)

class QueryProcessor;

class HttpServer:public TaskBase{
	public:
		friend class QueryProcessor;
		HttpServer();
		virtual ~HttpServer();
		
		int open(size_t thread_num, size_t stack_size, int port);//,pthread_barrier_t *processor_init);
		int stop();
		//关联http和processor
		void link(QueryProcessor* processor);

	private:
		int svc(size_t idx);

		int create_listen(int &socket_fd, unsigned short port);
		int set_socket(int fd, int flag);
		int createSession(int fd);
		int closeSession(int fd, HttpWorker * worker);
		void checkUnreleaseWorker();
		bool isRecvOver(HttpWorker* worker);

		int add_input_fd(int fd, HttpWorker* worker);
		int add_output_fd(int fd, HttpWorker* worker);
		int mod_input_fd(int fd, HttpWorker* worker);
		int mod_output_fd(int fd, HttpWorker* worker);
		int del_input_fd(int fd);
		
		int get_client_ip(int fd, HttpWorker* worker);

		void logRequest(const HttpWorker* worker);
		void logResponse(const HttpWorker* worker);
	private:
		QueryProcessor *m_processor;
		// pthread_barrier_t *processor_init;

		int _epoll_fd;	
		int _socket_server_listen;
		int _epoll_ready_event_num;
		pthread_mutex_t _epoll_mutex;
		pthread_mutex_t _connections_mutex;
		int _stop_task;
		int _now_connections;
		struct epoll_event _epoll_ready_event[MAX_EPOLL_EVENT_NUM];

		std::tr1::unordered_map<HttpWorker*, int> _unreleaseWorkers;
    	
    	std::string _ip;
};

}	//namespace MJ

#endif //__HTTP_SERVER_H__

