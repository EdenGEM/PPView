
#include "HttpWorker.h"
#include <stdio.h>
#include <string>
#include <sys/time.h>

namespace MJ{



HttpWorker::HttpWorker(int socket){
	m_socket = socket;
	m_stat = 0;

	m_requestStr = "";
	m_sendCnt = 0;

	m_request = new MJHttpRequest();
	m_responseStr = "";

	m_threadIndex = 0;
	
	gettimeofday(&m_conn_time, NULL);
}

HttpWorker::~HttpWorker(){
	if (m_request){
		delete m_request;
		m_request = NULL;
	}
}
bool HttpWorker::isAlive(){
	return (m_socket > 0);
}

}		//namespace MJ


