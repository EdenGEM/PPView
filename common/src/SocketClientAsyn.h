#ifndef __SOCKET_CLIENT_ASYN_H__
#define __SOCKET_CLIENT_ASYN_H__

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>

#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string/regex.hpp"
#include "boost/format.hpp"
#include "boost/regex.hpp"
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"


namespace MJ{

class EventRespon {
public:
	std::string m_input;	//输入请求
	int *m_remainRespon;	//剩余需要接收的请求数
	struct event_base *m_base;
	char *m_output;			//输出结果
	size_t m_outputBuffSize;	//输出的buff大小
public:
	EventRespon(const EventRespon &p);	//禁止复制
	EventRespon(const std::string &input, int *remainRespon, struct event_base *base, size_t bufSize=10*1024*1024):m_input(input),m_remainRespon(remainRespon),m_base(base) {
		m_outputBuffSize = bufSize;
		m_output = new char[m_outputBuffSize];
	}
	~EventRespon() {
		delete [] m_output;
	}
};

//libevent 非阻塞客户端
class SocketClientAsyn {
private:
	static void set_tcp_no_delay(evutil_socket_t fd);
	static void timeoutcb(evutil_socket_t fd, short what, void *arg);
	static void readcb(struct bufferevent *bev, void *ctx);
	static void eventcb(struct bufferevent *bev, short events, void *ptr);
	static void decodeBodyChunked(std::string& ret, char* buf);
	static void decodeBody(std::string& ret, char* buf, int body_pos);
	static int recvResult(char* result);
public:
	int init(const std::string &addr, time_t timeout, size_t buffSize=10*1024*1024);
	int doRequest(const std::vector<std::string> &queryList, std::vector<std::string> &respon, int type = 0);
private:
	int m_port;				//端口
	in_addr m_realAddr;		//使用地址
	std::string m_addr;		//地址
	time_t m_timeout;			//超时时间
	size_t m_buffSize;		//输出缓存大小
};

}

#endif 	//__SOCKET_CLIENT_ASYN_H__
