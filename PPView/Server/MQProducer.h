#include <iostream>
#include <pthread.h>
#include "worker.h"
#include "MyRabbitMQ.h"

class MQProducer {
public:
	static const MQProducer* GetInstance();
public:
	int Publish(const std::string& msg, const std::string& mqRetKey) const;

private:
	MQProducer();
	~MQProducer();
	MQProducer(const MQProducer& vMQProducer) {}
	MQProducer& operator=(const MQProducer& vMQProducer) {}
private:
	MJ::MyRabbitMQ_Producer* m_rabbitMQProducer;
	mutable pthread_mutex_t m_mutex;
};
