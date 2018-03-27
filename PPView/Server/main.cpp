#include <iostream>
#include <malloc.h>
#include <signal.h>
#include "MQQueryProcessor.h"
#include "MQConsumer.h"
#include "MyConfig.h"
#include "MJCommon.h"
#include "MYHotUpdate.h"
#include "Route/base/PrivateConstData.h"

#define DEFAULT_CONFIG_KEYNAME          "SERVER"

//#define WRITE_CERR_LOG_INTO_FILE

void sigterm_handler(int signo) {}
void sigint_handler(int signo) {}

int main(int argc, char* argv[]) {
	int SIZE = 4*1000*1000;
	char* buf = new char[SIZE];
	setvbuf(stderr, buf, _IOLBF, SIZE);

	mallopt(M_MMAP_THRESHOLD, 64 * 1024);

	if (argc < 2) {
		MJ::PrintInfo::PrintErr("main, %s config_filename config_keyname(opt)\n", argv[0]);
		return 1;
	}

	const char* config_filename = argv[1];
	const char* config_keyname = DEFAULT_CONFIG_KEYNAME;
	if (argc >= 3) config_keyname = argv[2];

	close(STDIN_FILENO);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, &sigterm_handler);
	signal(SIGINT, &sigint_handler);

	int ret = 0;
	MJ::PrintInfo::PrintLog("main, Server starting...");
	{
		ret = MyConfig::Init(config_filename);
		if (ret != 0) {
			MJ::PrintInfo::PrintErr("main, open configure file and key name < %s : %s > error", config_filename, config_keyname);
			return -1;
		}
		MJ::PrintInfo::SwitchDbg(MyConfig::m_needDbg, MyConfig::m_needLog, MyConfig::m_needErr, MyConfig::m_needDump);

		PrivateConfig::LoadPrivateConfig(config_filename);

		std::vector<MJ::MJDBConfig> dbConfigs;
		MJ::MJDBConfig _priDb;
		_priDb._addr = MyConfig::private_db_host;
		_priDb._acc = MyConfig::private_db_user;
		_priDb._pwd = MyConfig::private_db_passwd;
		_priDb._db = MyConfig::private_db_name;
		dbConfigs.push_back(_priDb);

		PPViewUpdate* ppviewUpdate = new PPViewUpdate();
		//热更新初始化
		ppviewUpdate->init(dbConfigs, PrivateConfig::m_monitorList, PrivateConfig::m_interval, PrivateConfig::m_stackSize);
		ppviewUpdate->setModule("PPView");
		ppviewUpdate->setThread(MyConfig::m_processorNum);

		MQQueryProcessor queryProcessor;
		MQConsumer* pMQConsumer = MQConsumer::GetInstance();

		pthread_barrier_t processorInit;
		pthread_barrier_init(&processorInit, NULL, MyConfig::m_processorNum + 1);

		pMQConsumer->RegisterQueryProcessor(&queryProcessor);

		ret = queryProcessor.Init(&processorInit);
		if (ret != 0) {
			MJ::PrintInfo::PrintErr("main, open queryProcessor error! ret:%d, thread:%d", ret, MyConfig::m_processorNum);
			return 1;
		}
		MJ::PrintInfo::PrintLog("main, Server Init OK");

		queryProcessor.activate();
		pthread_barrier_wait(&processorInit);	// wait for all queryProcessors fully initialized

		sleep(1);
		pMQConsumer->activate();
		sleep(1);
		MJ::PrintInfo::PrintLog("main, Server started");

		ppviewUpdate->run();//热更新进行中

		pause();
		pMQConsumer->stop();
		queryProcessor.stop();
		pthread_barrier_destroy(&processorInit);
	}
	MJ::PrintInfo::PrintLog("main, Server stop");

	return 0;
}

