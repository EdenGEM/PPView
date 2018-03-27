#ifndef _MY_CONFIG_H_
#define _MY_CONFIG_H_

#include <string>

class MyConfig {
public:
	MyConfig() {}
public:
	static int Init(const std::string& filename);
public:
	static unsigned int m_threadStackSize;

	static unsigned int m_htpReceiverNum;
	static unsigned int m_processorNum;

	static int m_cmdLenWarning; //qo 的result结果长度最长为4k 因为此处是人工编辑 所以限定最大长度 否则可能被丢弃结果
	static std::string m_dataDir;
	static std::string m_confPath;

	static int m_needDbg;
	static int m_needLog;
	static int m_needErr;
	static int m_needDump;

	static std::string m_MQHost;
	static unsigned int m_MQPort;
	static std::string m_MQVHost;
	static std::string m_MQUser;
	static std::string m_MQPasswd;
	static std::string m_MQExchange;
	static std::string m_MQTaskQueue;
	static std::string m_MQTaskKey;
	static std::string m_MQRetQueue1;
	static std::string m_MQRetKey1;
	static std::string m_MQRetQueue2;
	static std::string m_MQRetKey2;

	static std::string private_db_host;
	static std::string private_db_user;
	static std::string private_db_passwd;
	static std::string private_db_name;

	static unsigned int m_runTimeOut;  // 超时阈值 与slave统一
};


#endif


