#include <iostream>
#include "worker.h"
#include "MyConfig.h"
#include "MQConsumer.h"

MQConsumer* MQConsumer::GetInstance() {
	static MQConsumer vMQConsumer;
	return &vMQConsumer;
}

MQConsumer::MQConsumer() {
	pthread_mutex_init(&m_mutex, NULL);
	m_queryProcessor = NULL;
	m_svcActive = true;
	m_rabbitMQConsumer = new MJ::MyRabbitMQ_Consumer(MyConfig::m_MQHost, MyConfig::m_MQPort, MyConfig::m_MQVHost, MyConfig::m_MQUser, MyConfig::m_MQPasswd);
	m_rabbitMQConsumer->bind(MyConfig::m_MQTaskQueue);
	MJ::PrintInfo::PrintLog("MQConsumer::MQConsumer, %s:%d, vHost: %s, queue: %s", MyConfig::m_MQHost.c_str(), MyConfig::m_MQPort, MyConfig::m_MQVHost.c_str(), MyConfig::m_MQTaskQueue.c_str());
	Init();
	m_threadID = 0;
}

int MQConsumer::Init() {
	return TaskBase::open(1, MyConfig::m_threadStackSize);
}

MQConsumer::~MQConsumer() {
	if (m_rabbitMQConsumer) {
		delete m_rabbitMQConsumer;
		m_rabbitMQConsumer = NULL;
	}
	pthread_mutex_destroy(&m_mutex);
}

int MQConsumer::RegisterQueryProcessor(MQQueryProcessor* queryProcessor) {
	m_queryProcessor = queryProcessor;
	return 0;
}

int MQConsumer::stop() {
	m_svcActive = false;
	join();
	MJ::PrintInfo::PrintLog("MQConsumer::stop, MQConsumer stop");
	return 0;
}

int MQConsumer::svc(size_t idx) {
	MJ::PrintInfo::PrintLog("MQConsumer::svc, starting...");

	pthread_mutex_lock(&m_mutex);
	int threadID = m_threadID++;
	MJ::PrintInfo::PrintLog("MQConsumer::svc, thread <%d> starting...", threadID);
	pthread_mutex_unlock(&m_mutex);

	while (m_svcActive) {
		if (m_queryProcessor->IsReady()) {
			pthread_mutex_lock(&m_mutex);
			std::string uri = m_rabbitMQConsumer->consume();
			pthread_mutex_unlock(&m_mutex);
			m_queryProcessor->SubmitTask(uri);
		} else {
			sleep(0.1);
		}
	}
	return 0;
}
