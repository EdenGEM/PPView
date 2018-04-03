#include "MyRabbitMQ.h"
#include "MJLog.h"
#include "AuxTools.h"
#include "amqp_tcp_socket.h"
#include <string>
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace MJ;

int on_amqp_error(amqp_rpc_reply_t x, char const *context){
	switch (x.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			return 0;

		case AMQP_RESPONSE_NONE:
			fprintf(stderr, "%s: missing RPC reply type!\n", context);
			break;

		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
			break;

		case AMQP_RESPONSE_SERVER_EXCEPTION:
			switch (x.reply.id) {
				case AMQP_CONNECTION_CLOSE_METHOD: {
					amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
					fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
					      context,
					      m->reply_code,
					      (int) m->reply_text.len, (char *) m->reply_text.bytes);
					break;
				}
				case AMQP_CHANNEL_CLOSE_METHOD: {
					amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
					fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
				    	context,
				    	m->reply_code,
				    	(int) m->reply_text.len, (char *) m->reply_text.bytes);
					break;
				}
				default:
		 			fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
		 			break;
		 	}
		break;
	}
	return 1;
}

MyRabbitMQ_Producer::MyRabbitMQ_Producer(const string& host,
			const int& port,
			const string& vhost,
			const string& user,
			const string& pwd){
	m_host = host;
	m_port = port;
	m_vhost = vhost;
	m_user = user;
	m_password = pwd;
	// m_channelCnt = 1;

	this->connect();
}

int MyRabbitMQ_Producer::connect(){
	m_socket = NULL;
  	m_conn = amqp_new_connection();
  	m_socket = amqp_tcp_socket_new(m_conn);
	if (!m_socket) {
		cerr<<"MyRabbitMQ_Producer:Socket创建失败\n";
		return 1;
	}

	int status = amqp_socket_open(m_socket, m_host.c_str(), m_port);
	if (status) {
		fprintf(stderr, "MyRabbitMQ_Producer:Socket连接失败\n");
		return 1;
	}

 	if (on_amqp_error(amqp_login(m_conn, m_vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_user.c_str(), m_password.c_str()),
                    "MyRabbitMQ_Producer:Login失败")!=0)
 		return 1;
 	return this->createChannel(1);
}

int MyRabbitMQ_Producer::createChannel(int channel){
	amqp_channel_open(m_conn, channel);
	if (on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Producer:channel创建失败")!=0)
		return 1;
	return 0;
}

int MyRabbitMQ_Producer::release(){
	int channel = 1;
	on_amqp_error(amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS),"MyRabbitMQ_Producer:关闭channel失败");
 	on_amqp_error(amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS),"MyRabbitMQ_Producer:关闭connection失败");
 	amqp_destroy_connection(m_conn);
 	m_socket = NULL;
	return 0;
}

MyRabbitMQ_Producer::~MyRabbitMQ_Producer(){
	this->release();
}

int MyRabbitMQ_Producer::bind(const std::string& queue,
							const std::string& exchange,
							const std::string& routeKey,
							const std::string& exchangeType){
	// return bind_internal(queue,exchange,routeKey,exchangeType,1);
	int channel = 1;
	amqp_exchange_declare(m_conn, channel, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(exchangeType.c_str()),
                        0, 1, 0, 0, amqp_empty_table);
	on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Producer:创建exchange失败");
	
	amqp_queue_declare(m_conn, channel, amqp_cstring_bytes(queue.c_str()), 
						0, 1, 0, 0, amqp_empty_table);
	on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Producer:创建queue失败");

	amqp_queue_bind(m_conn, channel,
					amqp_cstring_bytes(queue.c_str()),
					amqp_cstring_bytes(exchange.c_str()),
					amqp_cstring_bytes(routeKey.c_str()),
					amqp_empty_table);
	on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Producer:Binding失败");

	return 0;
}

int MyRabbitMQ_Producer::publish_in(const std::string& msg,
								const std::string& exchange,
								const std::string& routeKey,
								int expire){
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;//props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    //props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    if (expire!=0){
        props.expiration = amqp_cstring_bytes(std::to_string(expire).c_str());
    }

    int ret = 0;
    if ((ret = amqp_basic_publish(m_conn,1,amqp_cstring_bytes(exchange.c_str()),amqp_cstring_bytes(routeKey.c_str()),0,0,&props,amqp_cstring_bytes(msg.c_str()))) < 0){
    	MJ_LOG("publish失败(%d)",ret);
    }
 	return ret;
}

int MyRabbitMQ_Producer::publish(const std::string& msg,
								const std::string& exchange,
								const std::string& routeKey,
								int expire){
	if (publish_in(msg,exchange,routeKey,expire)){
		if (this->release() == 0 && this->connect() == 0){
			MJ_LOG("重连成功");
			if (publish_in(msg,exchange,routeKey,expire)){
				goto GOTO_ERROR;
			}
		}else{
			MJ_LOG("重连失败");
			goto GOTO_ERROR;
		}
	}
	return 0;
GOTO_ERROR:
	struct timeval t;
	gettimeofday(&t,NULL);
	long ts = t.tv_sec*1000+t.tv_usec/1000;
	char qid[32];
	snprintf(qid, 32, "%ld", ts); 
	logMJOB_Exception("ex1004", "", "", qid, ts, MJ::getLocalIP(), "", "",msg);
	return 1;
}


MyRabbitMQ_Consumer::MyRabbitMQ_Consumer(const string& host,
			const int& port,
			const string& vhost,
			const string& user,
			const string& pwd){
	m_host = host;
	m_port = port;
	m_vhost = vhost;
	m_user = user;
	m_password = pwd;
	m_socket = NULL;
	m_queues.clear();

	this->connect();
}


int MyRabbitMQ_Consumer::connect(){
	m_socket = NULL;
	m_conn = amqp_new_connection();
	m_socket = amqp_tcp_socket_new(m_conn);
	if (!m_socket) {
		fprintf(stderr, "MyRabbitMQ_Consumer:Socket创建失败\n");
		return 1;
	}

	int status = amqp_socket_open(m_socket, m_host.c_str(), m_port);
	if (status) {
		fprintf(stderr, "MyRabbitMQ_Consumer:Socket连接失败\n");
		return 1;
	}

 	on_amqp_error(amqp_login(m_conn, m_vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_user.c_str(), m_password.c_str()),
                    "MyRabbitMQ_Consumer:Login失败");

	return this->createChannel(1);
}


MyRabbitMQ_Consumer::~MyRabbitMQ_Consumer(){
	m_queues.clear();
	this->release();
}


int MyRabbitMQ_Consumer::release(){
	int channel = 1;
	on_amqp_error(amqp_channel_close(m_conn, channel, AMQP_REPLY_SUCCESS),"MyRabbitMQ_Consumer:关闭channel失败");
 	on_amqp_error(amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS),"MyRabbitMQ_Consumer:关闭connection失败");
 	amqp_destroy_connection(m_conn);
 	m_socket = NULL;
	return 0;
}

int MyRabbitMQ_Consumer::createChannel(int channel){
	amqp_channel_open(m_conn, 1);
	if (on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Consumer:channel创建失败") != 0)
		return 1;
	return 0;
}

int MyRabbitMQ_Consumer::bind(const std::string& queue,int channel){
	m_queues.push_back(queue);
	
	// amqp_queue_declare(m_conn, channel, amqp_cstring_bytes(queue.c_str()), 
						// 0, 1, 0, 0, amqp_empty_table);
	// on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Consumer:创建Queue失败");

	amqp_basic_consume(m_conn, channel, amqp_cstring_bytes(queue.c_str()), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
	on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Consumer:绑定Queue失败");

	return 0;
}

int MyRabbitMQ_Consumer::rebind(int channel){
	size_t i;
	for (i=0;i<m_queues.size();i++){
		// amqp_queue_declare(m_conn, channel, amqp_cstring_bytes(m_queues[i].c_str()), 
							// 0, 1, 0, 0, amqp_empty_table);
		// on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Consumer:创建Queue失败");

		amqp_basic_consume(m_conn, channel, amqp_cstring_bytes(m_queues[i].c_str()), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
		on_amqp_error(amqp_get_rpc_reply(m_conn), "MyRabbitMQ_Consumer:绑定Queue失败");
	}

	return 0;
}

int MyRabbitMQ_Consumer::consume(std::string& msg){
	amqp_rpc_reply_t ret;
	amqp_envelope_t envelope;
	amqp_maybe_release_buffers(m_conn);
	ret = amqp_consume_message(m_conn, &envelope, NULL, 0);
	if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
		MJ_LOG("consume ret: type=%d library_error=%d",ret.reply_type,ret.library_error);
		goto GOTO_ERROR;
	} else {
		char* str = (char*)envelope.message.body.bytes;  
		if (str != NULL)
			msg.assign(str, envelope.message.body.len);
		amqp_basic_ack(m_conn,1,envelope.delivery_tag,0);
		amqp_destroy_envelope(&envelope);
    }
    return 0;

GOTO_ERROR:
	MJ_LOG("consume失败");
	return 1;
}

std::string MyRabbitMQ_Consumer::consume(int retryTimeInterval){
	std::string msg = "";
	int ret;
	while(true) {
		ret = consume(msg);
		//失败重连
		if (ret){
			if (this->release() == 0 && this->connect() == 0 && this->rebind() == 0){
				MJ_LOG("重连成功");
				ret = consume(msg);
				if (ret) {
				GOTO_ERROR:
					MJ_LOG("等待%ds",retryTimeInterval);
					struct timeval t;
					gettimeofday(&t,NULL);
					long ts = t.tv_sec*1000+t.tv_usec/1000;
					char qid[32];
					snprintf(qid, 32, "%ld", ts); 
					logMJOB_Exception("ex1005", "", "", qid, ts, MJ::getLocalIP(), "", "",msg);
					sleep(retryTimeInterval);
				}
			}else{
				MJ_LOG("重连失败");
				goto GOTO_ERROR;
			}
		}
	    if (msg.length()>0)
	    	break;
	}
	return msg;
}

