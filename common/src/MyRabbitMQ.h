
#ifndef __MY_RABBITMQ_H__
#define __MY_RABBITMQ_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <stdint.h>

#include "amqp.h"
#include "amqp_framing.h"

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <exception>


namespace MJ{


class MyRabbitMQ_Producer{
private:
	int m_port;
	std::string m_host;
	std::string m_vhost;
	std::string m_user;
	std::string m_password;

	amqp_socket_t *m_socket;
  	amqp_connection_state_t m_conn;
  	// int m_channelCnt;
private:
	int connect();
	int release();
	/*
	* 创建channel，channel可用来多线程并发处理，共用同一个TCP连接。
	* 每个线程一个channel, channel不可线程间共享，否则线程不安全
	* 该函数暂时先不要外部用
	*/
	int createChannel(int channel=1);
	/*
	* return: 0正常 1:发送失败
	*/
	int publish_in(const std::string& msg,
				const std::string& exchange,
				const std::string& routeKey,
				int expire=0);
public:
	MyRabbitMQ_Producer(const std::string& host,
			const int& port,
			const std::string& vhost,
			const std::string& user,
			const std::string& pwd);
	~MyRabbitMQ_Producer();
	/*绑定exchange routeKey和queue  可多次bind*/
	int bind(const std::string& queue,
			const std::string& exchange,
			const std::string& routeKey,
			const std::string& exchangeType="direct");
	/*
	* expire: int 消息过期时间 单位是ms
	* return: 0正常 1:发送失败
	*/
	int publish(const std::string& msg,
				const std::string& exchange,
				const std::string& routeKey,
				int expire=0);

};

/*封装的不完全 暂时不推荐使用*/
class MyRabbitMQ_Consumer{
private:
	int m_port;
	std::string m_host;
	std::string m_vhost;
	std::string m_user;
	std::string m_password;

	amqp_socket_t *m_socket;
	amqp_connection_state_t m_conn;

	std::vector<std::string> m_queues;
public:
	MyRabbitMQ_Consumer(const std::string& host,
			const int& port,
			const std::string& vhost,
			const std::string& user,
			const std::string& pwd);
	~MyRabbitMQ_Consumer();
	/*绑定queue*/
	int bind(const std::string& queue,int channel=1);
	/*阻塞的获取RM消息  默认重试需要5秒时间间隔*/
	std::string consume(int retryTimeInterval=5);
private:
	/*
	* 创建channel，channel可用来多线程并发处理，共用同一个TCP连接。
	* 每个线程一个channel, channel不可线程间共享，否则线程不安全
	*/
	int createChannel(int channel=1);
	int rebind(int channel=1);
	int connect();
	int release();
	int consume(std::string& msg);
};

} //namespace MJ


#endif //__MY_RABBITMQ_H__



