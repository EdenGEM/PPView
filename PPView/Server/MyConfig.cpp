#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include "MyConfig.h"


unsigned int MyConfig::m_threadStackSize = 204800000;

unsigned int MyConfig::m_processorNum = 4;

int MyConfig::m_cmdLenWarning = 1024;
std::string MyConfig::m_dataDir = "../data/";
std::string MyConfig::m_confPath = "../conf/";

int MyConfig::m_needDbg = 0;
int MyConfig::m_needLog = 1;
int MyConfig::m_needErr = 1;
int MyConfig::m_needDump = 1;

std::string MyConfig::m_MQHost = "127.0.0.1";
unsigned int MyConfig::m_MQPort = 5672;
std::string MyConfig::m_MQVHost = "PPViewChat";
std::string MyConfig::m_MQUser = "master";
std::string MyConfig::m_MQPasswd = "master";
std::string MyConfig::m_MQExchange = "PPViewChat";
std::string MyConfig::m_MQTaskQueue = "Task";
std::string MyConfig::m_MQTaskKey = "Task";
std::string MyConfig::m_MQRetQueue1 = "Ret1";
std::string MyConfig::m_MQRetKey1 = "Ret1";
std::string MyConfig::m_MQRetQueue2 = "Ret2";
std::string MyConfig::m_MQRetKey2 = "Ret2";
std::string MyConfig::private_db_host;
std::string MyConfig::private_db_user;
std::string MyConfig::private_db_passwd;
std::string MyConfig::private_db_name;

unsigned int MyConfig::m_runTimeOut = 50 * 1000 * 1000;  // 50s

int MyConfig::Init(const std::string& filename) {
	m_confPath = filename;
	std::ifstream fin(filename.c_str());
	if (!fin) {
		fprintf(stderr, "MyConfig::Init, open file err: %s\n", filename.c_str());
		return 1;
	}

	std::string line;
	while (std::getline(fin, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::string::size_type pos = line.find('=');
		if (pos == std::string::npos) {
			fprintf(stderr, "MyConfig::Init, format err line: %s\n", line.c_str());
			return 1;
		}
		
		std::string key = line.substr(0, pos);
		std::string val = line.substr(pos + 1);
		if (key == "ThreadStackSize") {
			m_threadStackSize = atoi(val.c_str());
		} else if (key == "CommandLengthWarning") {
			m_cmdLenWarning = atoi(val.c_str());
		} else if (key == "ProcessorThread") {
			m_processorNum = atoi(val.c_str());
		} else if (key == "ConfPath") {
			m_confPath = val;
		} else if (key == "DataDir") {
			m_dataDir = val;
		} else if (key == "NeedDbg") {
			m_needDbg = atoi(val.c_str());
		} else if (key == "NeedLog") {
			m_needLog = atoi(val.c_str());
		} else if (key == "NeedErr") {
			m_needErr = atoi(val.c_str());
		} else if (key == "NeedDump") {
			m_needDump = atoi(val.c_str());
		} else if (key == "MQHost") {
			m_MQHost = val;
		} else if (key == "MQPort") {
			m_MQPort = atoi(val.c_str());
		} else if (key == "MQVHost") {
			m_MQVHost = val;
		} else if (key == "MQUser") {
			m_MQUser = val;
		} else if (key == "MQPasswd") {
			m_MQPasswd = val;
		} else if (key == "MQExchange") {
			m_MQExchange = val;
		} else if (key == "MQTaskQueue") {
			m_MQTaskQueue = val;
		} else if (key == "MQTaskKey") {
			m_MQTaskKey = val;
		} else if (key == "MQRetQueue1") {
			m_MQRetQueue1 = val;
		} else if (key == "MQRetKey1") {
			m_MQRetKey1 = val;
		} else if (key == "MQRetQueue2") {
			m_MQRetQueue2 = val;
		} else if (key == "MQRetKey2") {
			m_MQRetKey2 = val;
		} else if (key == "RunTimeOut") {
			m_runTimeOut = atoi(val.c_str());
		} else if (key == "private_db_host") {
            private_db_host = val;
        } else if (key == "private_db_user") {
            private_db_user = val;
        } else if (key == "private_db_passwd") {
            private_db_passwd = val;
		} else if (key == "private_db_name") {
            private_db_name = val;
		} else {
			fprintf(stderr, "MyConfig::Init, UNK: %s/%s\n", key.c_str(), val.c_str());
		}
	}
	return 0;
	
}

