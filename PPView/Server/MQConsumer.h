#ifndef _MQ_CONSUMER_H_
#define _MQ_CONSUMER_H_

#include <iostream>
#include <pthread.h>
#include "http/TaskBase.h"
#include "MQQueryProcessor.h"
#include "MyRabbitMQ.h"


class MQConsumer : public MJ::TaskBase{
public:
	static MQConsumer* GetInstance();
	int RegisterQueryProcessor(MQQueryProcessor* queryProcessor);
	int stop();

private:
	MQConsumer();
	~MQConsumer();
	MQConsumer(const MQConsumer& vMQConsumer) {}
	MQConsumer& operator=(const MQConsumer& vMQConsumer) {}

	int Init();
	int svc(size_t idx);

private:
	MQQueryProcessor* m_queryProcessor;
	bool m_svcActive;
	MJ::MyRabbitMQ_Consumer* m_rabbitMQConsumer;
	mutable pthread_mutex_t m_mutex;
	int m_threadID;
};
#endif

