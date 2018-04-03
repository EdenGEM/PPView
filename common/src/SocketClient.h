#ifndef __SOGOU_CLIENT_H_
#define __SOGOU_CLIENT_H_

//#include "QuestionAnswer.h"
//#include "NLPTools_Number.h"
//#include "QuestionClassify.h"
//
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctype.h>
//#include <boost/algorithm/string.hpp>
#include <sstream>
#include <pthread.h>
#include <map>
//#include "extra/FuncSet.h"

namespace MJ
{

class ServerRst
{
	public:
		int ret_len;
		std::string ret_str;

	public:
		ServerRst()
		{
			ret_len = -1;
			ret_str = "";
		};
		~ServerRst(){};
};

class SocketClient
{
	public:
		SocketClient(){};
		~SocketClient(){};
		bool init(const std::string& addr,const long& timeout);
		bool set_timeout(const int time_out)
		{
			m_timeout = time_out;
			return true;
		}

	private:
		int read_timeout(int fd, char* buf ,int len, timeval *timeout);
		int readn_timeout(int fd, char* content, int need_to_read, timeval *timeout);
		int read_http_header_timeout(int fd,char* content, timeval *timeout);
		int recv_result(char* result, int& len, int fd,long time_out_us);
		bool string_split(const std::string& str, const std::string& pattern, std::vector<std::string>& v);

		void decodeBody_Chunked(std::string& ret,char* buf);
		void decodeBody(std::string& ret,char* buf,int body_pos);

	public:
		std::string m_ip;
		int m_port;
		std::string m_addr;
		long m_timeout;
		in_addr m_real_addr;
		bool getRstFromHost(const std::string& query,ServerRst& sr,unsigned short type=0);

};

}

#endif
