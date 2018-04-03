#include "HttpServer.h"
#include "QueryProcessor.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <assert.h>
#include "MJLog.h"
#include "AuxTools.h"
#include <unistd.h>


using namespace MJ;

HttpServer::HttpServer()
{
	_epoll_fd = -1;
	_socket_server_listen = -1;
	_stop_task = 0;
	_epoll_ready_event_num = 0;
	_now_connections = 0;
	_ip = getLocalIP();

	pthread_mutex_init(&_epoll_mutex, NULL);
	pthread_mutex_init(&_connections_mutex,NULL);

}

HttpServer::~HttpServer()
{
	::close(_epoll_fd);
	::close(_socket_server_listen);
	pthread_mutex_destroy(&_epoll_mutex);
	pthread_mutex_destroy(&_connections_mutex);
}

void HttpServer::link(QueryProcessor * processor){
	m_processor = processor;
}

int HttpServer::open(size_t thread_num, size_t stack_size, int port)//, pthread_barrier_t *processor_init)
{
	if ((_epoll_fd = epoll_create(MAX_FD_NUM/*该参数大于1即可*/)) == -1){
		fprintf(stderr,"[Error]: epoll_create() fail!\n");
		return -1;
	}
	if (create_listen(_socket_server_listen, port)){
		fprintf(stderr,"[Error]: Socket监听端口失败 fail !! \n");
		return -1;
	}

	return TaskBase::open(thread_num+2, stack_size);
}

int HttpServer::stop()
{
	_stop_task = 1;
	join();
	MJ_LOG("http server stop.");
	return 0;
}

void HttpServer::checkUnreleaseWorker(){
	pthread_mutex_lock(&_connections_mutex);
	MJ_LOG_INFO("[未释放worker(%zd)]",_unreleaseWorkers.size());
	std::tr1::unordered_map<HttpWorker*,int>::const_iterator it = _unreleaseWorkers.begin();
	for (;it!=_unreleaseWorkers.end();){
		if (it->second == -1 && it->first->m_stat != 1){
			MJ_LOG_INFO("[delete worker(%p) stat(%d)]",it->first,it->first->m_stat);
			delete it->first;
			it = _unreleaseWorkers.erase(it);
		}else
			it++;
	}
	pthread_mutex_unlock(&_connections_mutex);
	return;
}

int HttpServer::closeSession(int fd, HttpWorker* worker){
	pthread_mutex_lock(&_connections_mutex);
	std::tr1::unordered_map<HttpWorker*,int>::const_iterator it = _unreleaseWorkers.find(worker);
	if (it != _unreleaseWorkers.end() && it->second == fd){
		//1, 删除epoll监控事件
		epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		//2,关闭socket
		::close(fd);
		_now_connections--;
		MJ_LOG_INFO("[Conn close(%d), rest(%d)]", fd, _now_connections);
		_unreleaseWorkers[worker] = -1;
	}else{
		MJ_LOG_INFO("[Conn close(%d) failed, rest(%d)]", fd, _now_connections);
	}
	pthread_mutex_unlock(&_connections_mutex);
	return 0;
}

int HttpServer::createSession(int fd){
	pthread_mutex_lock(&_connections_mutex);
	set_socket(fd, O_NONBLOCK);
	HttpWorker * worker = new HttpWorker(fd);
	_unreleaseWorkers[worker] = fd;
	_now_connections++;
	MJ_LOG_INFO("[New Conn(%d), rest(%d)]", fd, _now_connections);
	add_input_fd(fd,worker);
	pthread_mutex_unlock(&_connections_mutex);
	return 0;
}


bool HttpServer::isRecvOver(HttpWorker* worker){
	int ret = worker->m_request->parseREQ(worker->m_requestStr);
	if (ret == 2){
		get_client_ip(worker->m_socket, worker);	
		return true;
	}else{
		return false;
	}
}

void HttpServer::logRequest(const HttpWorker* worker){
	struct timeval t;
	gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);
	
	logMJOB_RequestServer(worker->m_request->_type, worker->m_request->_uid, worker->m_request->_csuid, worker->m_request->_qid, ts, _ip, worker->m_request->_refer_id, worker->m_request->_cur_id, UrlEncode(worker->m_request->_data_str));
	return;
}

void HttpServer::logResponse(const HttpWorker* worker){
	struct timeval t;
	gettimeofday(&t,NULL);
    long ts = t.tv_sec*1000+(t.tv_usec/1000);

	long cost = ts - (worker->m_conn_time.tv_sec*1000+(worker->m_conn_time.tv_usec/1000));

	size_t pos = worker->m_responseStr.find("\r\n\r\n");
	if (pos != std::string::npos)
		logMJOB_ResponseServer(worker->m_request->_type, worker->m_request->_uid, worker->m_request->_csuid, worker->m_request->_qid, ts, _ip, worker->m_request->_refer_id, worker->m_request->_cur_id, UrlEncode(worker->m_responseStr.substr(pos+4)), cost);
	else
		logMJOB_ResponseServer(worker->m_request->_type, worker->m_request->_uid, worker->m_request->_csuid, worker->m_request->_qid, ts, _ip, worker->m_request->_refer_id, worker->m_request->_cur_id, UrlEncode("Response报文不合法"), cost);
	return;
}

int HttpServer::svc(size_t idx)
{
	printf("HttpThread<%zd> 启动\n", idx);
	int fd,new_fd,event_idx,cur_event_type;
	HttpWorker* worker;
	// pthread_barrier_wait(processor_init);
	char buf[READ_BUF_SIZE + 1];
	//单独的建立连接线程(accept)
	if (idx == 0){
		while(_stop_task==0 && (new_fd = accept(_socket_server_listen, NULL, NULL)) >= 0){
			createSession(new_fd);
		}
	}else if (idx == 2){
		while (!_stop_task){
			checkUnreleaseWorker();
			sleep(5);
		}
	}else{
		while (!_stop_task){
			pthread_mutex_lock(&_epoll_mutex);
			if (_stop_task) {
				pthread_mutex_unlock(&_epoll_mutex);
				break;
			}
			if (_epoll_ready_event_num <= 0){
				_epoll_ready_event_num = epoll_wait(_epoll_fd, _epoll_ready_event, MAX_EPOLL_EVENT_NUM, -1);
				// MJ_LOG_INFO("epoll_wait events: %d in thread<%d>",_epoll_ready_event_num,idx);
			}
			if (_epoll_ready_event_num-- < 0){
				pthread_mutex_unlock(&_epoll_mutex);
				if (errno == EINTR)
					continue;	
				else
					break;
			}
			event_idx = _epoll_ready_event_num;
			worker = (HttpWorker*) _epoll_ready_event[event_idx].data.ptr;
			fd = worker->m_socket;
			if (_epoll_ready_event[event_idx].events & EPOLLIN)
				cur_event_type = EPOLLIN;
			else if (_epoll_ready_event[event_idx].events & EPOLLOUT)
				cur_event_type = EPOLLOUT;
			else 
				cur_event_type = EPOLLERR;
			pthread_mutex_unlock(&_epoll_mutex);
			switch (cur_event_type){
				case EPOLLIN:
					int readCnt;
					while(true){
						readCnt = read(fd, buf, READ_BUF_SIZE);
						// printf("read: %d\n", readCnt);
						if (readCnt > 0){
							buf[readCnt] = '\0';
							worker->m_requestStr += buf;
							continue;
						}else if (readCnt == 0){
							MJ_LOG_ERROR("socket(%d)被客户端关闭",worker->m_socket);
							closeSession(fd,worker);
							break;
						}else{
							// printf("errno:%d\n", errno);
							if (errno == EAGAIN || errno == EWOULDBLOCK){
								if (isRecvOver(worker)){
									//保持epoll监控事件(监控客户端关闭链接)
									mod_input_fd(worker->m_socket, worker);
									//添加日志
									logRequest(worker);
									//加入processor队列
									m_processor->submitWorker(worker);
									// MJ_LOG_INFO("请求read完成");
									break;
								}else{
									//重置监控事件，等待再次接收
									// MJ_LOG_ERROR("read未结束");
									mod_input_fd(worker->m_socket, worker);
									break;
								}
							}else{
								MJ_LOG_ERROR("Read失败(errno=%d),关闭连接",errno);
								closeSession(fd,worker);
								break;
							}
						}
					}
					break;					
				case EPOLLOUT:
					int writeCnt;
					while(true){
						const char* rest_buff = worker->m_responseStr.c_str()+worker->m_sendCnt;
						int len = worker->m_responseStr.length() - worker->m_sendCnt;
						if (len <= 0){
							//添加日志
							logResponse(worker);
							//发送完毕
							// MJ_LOG_INFO("响应send完成1");
							closeSession(fd,worker);
							break;
						}
						writeCnt = write(worker->m_socket,rest_buff,len);
						// printf("write: %d\n", writeCnt);
						if (writeCnt > 0){
							if (writeCnt >= len){
								//添加日志
								logResponse(worker);
								//发送完毕
								// MJ_LOG_INFO("响应send完成2");
								closeSession(fd,worker);
								break;
							}
							//继续发送
							worker->m_sendCnt += writeCnt;
							continue;
						}else if (writeCnt == 0){
							MJ_LOG_ERROR("socket(%d)被客户端关闭",worker->m_socket);
							closeSession(fd,worker);
							break;
						}else{
							if (errno == EAGAIN || errno == EWOULDBLOCK){
								//缓冲区满 等待下次发送
								// MJ_LOG_ERROR("write未结束");
								mod_output_fd(worker->m_socket, worker);
								break;
							}else{
								MJ_LOG_ERROR("Send失败(errno=%d),关闭连接",errno);
								closeSession(fd,worker);
								break;
							}
						}
					}
					break;
				default:
					MJ_LOG_INFO("Epoll错误事件(%d),关闭连接",cur_event_type);
					closeSession(fd,worker);
					break;
			}
		}
	}
	MJ_LOG_INFO("HttpServer线程<%zd>结束", idx);
	return 0;
}


int HttpServer::create_listen(int &socket_fd, unsigned short port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;	//IP地址族
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	//创建socket
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		MJ_LOG_ERROR("创建socket失败");
		return -1;
	}

	int options;
	options = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(int));
	int on = 1;
	if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) < 0){
		MJ_LOG_ERROR("设置socket屏蔽Nagle算法失败");
		return -1;
	}


	if (bind(socket_fd, (const sockaddr*)&addr, sizeof addr))
		return -1;
	if (listen(socket_fd, SOMAXCONN))
		return -1;

	MJ_LOG_INFO("Socket开始监听端口 %d", port);
	return 0;
}

int HttpServer::set_socket(int fd, int flag)
{
	//设置socket缓冲区
	int options;
	options = SOCKET_SND_BUF_SIZE;
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &options, sizeof(int));
	options = SOCKET_RCV_BUF_SIZE;
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &options, sizeof(int));
	//设置socket重用地址bind
	options = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(int));
	//设置socket非阻塞模式
	options = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, options | flag);
	//屏蔽Nagle算法，保证实施发送TCP数据报文
	int on = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) < 0){
		MJ_LOG_ERROR("设置socket屏蔽Nagle算法失败");
		return -1;
	}
	return 0;
}




int HttpServer::add_input_fd(int fd, HttpWorker* worker)
{
	//向epoll实例绑定要监控的fd及监控事件
	epoll_event event;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	// event.data.fd = fd;
	event.data.ptr = worker;
	epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event);
	
	return 0;
}
int HttpServer::add_output_fd(int fd, HttpWorker* worker)
{
	//向epoll实例绑定要监控的fd及监控事件
	epoll_event event;
	event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
	// event.data.fd = fd;
	event.data.ptr = worker;
	epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event);

	return 0;
}
int HttpServer::mod_input_fd(int fd, HttpWorker* worker)
{
	//向epoll实例绑定要监控的fd及监控事件
	epoll_event event;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	// event.data.fd = fd;
	event.data.ptr = worker;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);

	return 0;
}
int HttpServer::mod_output_fd(int fd, HttpWorker* worker){
	//向epoll实例绑定要监控的fd及监控事件
	epoll_event event;
	event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
	// event.data.fd = fd;
	event.data.ptr = worker;
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);

	return 0;
}

int HttpServer::del_input_fd(int fd){
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	return 0;
}

int HttpServer::get_client_ip(int fd, HttpWorker* worker)
{
		if (fd == 0)
			return 0;
		
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof addr);
		socklen_t len = sizeof addr;
		
		if (getpeername(fd, (struct sockaddr*)&addr, &len) != 0)
		{
			return 0;
		}
		worker->m_request->_client_ip =  std::string(inet_ntoa(addr.sin_addr));
//		char tmp_ip[INET_ADDRSTRLEN];
//		inet_ntop(AF_INET, &(addr.sin_addr), tmp_ip, INET_ADDRSTRLEN);
//		worker->client_ip = std::string(tmp_ip);
		return 0;

}
