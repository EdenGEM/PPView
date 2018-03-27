#include <iostream>
#include "MQProducer.h"
#include "MyConfig.h"

const MQProducer* MQProducer::GetInstance() {
	static MQProducer vMQProducer;
	return &vMQProducer;
}

MQProducer::MQProducer() {
	m_rabbitMQProducer = new MJ::MyRabbitMQ_Producer(MyConfig::m_MQHost, MyConfig::m_MQPort, MyConfig::m_MQVHost, MyConfig::m_MQUser, MyConfig::m_MQPasswd);
	m_rabbitMQProducer->bind(MyConfig::m_MQRetQueue1, MyConfig::m_MQExchange, MyConfig::m_MQRetKey1, "direct");
	MJ::PrintInfo::PrintLog("MQProducer::MQProducer, %s:%d, vHost: %s, ex: %s, retKey1: %s, retKey2: %s", MyConfig::m_MQHost.c_str(), MyConfig::m_MQPort, MyConfig::m_MQVHost.c_str(), MyConfig::m_MQExchange.c_str(), MyConfig::m_MQRetKey1.c_str(), MyConfig::m_MQRetKey2.c_str());

	pthread_mutex_init(&m_mutex, NULL);
}

MQProducer::~MQProducer() {
	if (m_rabbitMQProducer) {
		delete m_rabbitMQProducer;
		m_rabbitMQProducer = NULL;
	}
	pthread_mutex_destroy(&m_mutex);
}

int MQProducer::Publish(const std::string& msg, const std::string& mqRetKey) const {
	pthread_mutex_lock(&m_mutex);
	int ret = m_rabbitMQProducer->publish(msg, MyConfig::m_MQExchange, mqRetKey);
	std::cerr<<"hyhy from "<<mqRetKey<<std::endl;
	pthread_mutex_unlock(&m_mutex);

	return ret;
}
