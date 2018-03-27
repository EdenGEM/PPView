#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <algorithm>
#include "MyConfig.h"
#include "MQProducer.h"
#include "Route/base/BagParam.h"
#include "MQQueryProcessor.h"
#include "MYHotUpdate.h"
#include "Route/base/LYConstData.h"
#include "Route/base/PrivateConstData.h"

const unsigned int MAX_WAIT_TIME_MS = 2000;
const unsigned int MAX_HTTP_CONTENT_LENGTH = 1024*1024*10;

MQQueryProcessor::MQQueryProcessor() {
	pthread_mutex_init(&m_mutex, NULL);
	m_threadID = 0;
}

MQQueryProcessor::~MQQueryProcessor() {
	for (int i = 0; i < m_routeList.size(); ++i) {
		delete m_routeList[i];
	}
	m_routeList.clear();
	Route::ReleaseStatic();

	pthread_mutex_destroy(&m_mutex);
}

int MQQueryProcessor::SubmitTask(const std::string& uri) {
	Worker* worker = new Worker;
	worker->m_uri = uri;
	gettimeofday(&worker->m_recvTime, NULL);
	m_taskList.put(*worker);
	MJ::PrintInfo::PrintLog("MQQueryProcessor::SubmitTask, task len:\t%d", m_taskList.len());
	return 0;
}


int MQQueryProcessor::Init(pthread_barrier_t* processorInit) {
	MJ::PrintInfo::PrintLog("MQQueryProcessor::Init, starting...");
	m_processorInit = processorInit;

	int ret = Route::LoadStatic(MyConfig::m_dataDir, MyConfig::m_confPath);
	if (ret != 0) {
		MJ::PrintInfo::PrintErr("MQQueryProcessor::open, loadStatic Processor failed!");
		return -1;
	}

	PrivateConstData* p=new PrivateConstData();
	p->PrivateLoad();
	if (p == NULL) {
		MJ::PrintInfo::PrintErr("MQQueryProcessor::load private data failed!");
		return ret;
	}
	MJ::MJHotUpdate::initSharedData(p);

	for (int i = 0; i < MyConfig::m_processorNum; ++i) {
		Route* route = new Route;
		if (!route) return 1;
		m_routeList.push_back(route);
	}
	return TaskBase::open(MyConfig::m_processorNum, MyConfig::m_threadStackSize);
}

int MQQueryProcessor::stop() {
	m_taskList.flush();
	join();
	MJ::PrintInfo::PrintLog("MQQueryProcessor::stop, query process stop");
	return 0;
}

int MQQueryProcessor::svc(size_t idx) {
	// ret范围:
	// >=0正常(结果串长度)，
	// -1 ParseWorker错误，
	// -2排队超时

	Worker* worker;
	Json::Reader jr;
	Json::FastWriter jw;
	jw.omitEndingLineFeed();
	MJ::MyTimer t;

	char buff[MAX_HTTP_CONTENT_LENGTH] = {0};

	pthread_mutex_lock(&m_mutex);
	int threadID = m_threadID++;
	MJ::PrintInfo::PrintLog("MQQueryProcessor::svc, thread <%d> starting...", threadID);
	pthread_mutex_unlock(&m_mutex);

	pthread_barrier_wait(m_processorInit);

	while ((worker = m_taskList.get()) != NULL) {
		int ret = 0;
		int routeCost = 0;
		std::string dbgStr;
		ret = ParseWorker(worker);

		worker->m_qParam.priDataThreadId = threadID;
		MJ::MJHotUpdate::addSharedDataPtr(threadID);

		if (ret >= 0) {
			Json::Value jReq;
			Json::Value jResp;
			jr.parse(worker->m_queryS, jReq);
			if (!jReq.isObject()) {
				MJ::PrintInfo::PrintErr("[%s]MQQueryProcessor::svc, Json Parse failed!", worker->m_qParam.log.c_str());
				ret = -4;
				dbgStr = "请求解析失败";
			}

			if (ret >= 0) {
				timeval tv;
				gettimeofday(&tv, NULL);
				long long waitCost = ((long long)tv.tv_sec) * 1000000 + (long long)tv.tv_usec - worker->m_htpTime;
				if (waitCost > MyConfig::m_runTimeOut) {
					MJ::PrintInfo::PrintErr("[%s]MQQueryProcessor::svc, Time out before DoRoute: %d!", worker->m_qParam.log.c_str(), waitCost);
					ret = -2;
					dbgStr = "排队处理超时";
				}
			}

			if (ret >= 0) {
				t.start();
				MJ::PrintInfo::PrintLog("MQQueryProcessor::svc, before PPViewUpdate::readLock");
				PPViewUpdate::readLock();
				MJ::PrintInfo::PrintLog("MQQueryProcessor::svc, after PPViewUpdate::readLock");
				m_routeList[threadID]->DoRoute(worker->m_qParam, jReq, jResp);
				PPViewUpdate::unlock();
				routeCost = t.cost();
				if (jResp.isMember("dbg") && jResp["dbg"].isString()) {
					dbgStr = jResp["dbg"].asString();
				}
				jResp["dbg"] = "";
			} else {
				if (ret == -4) {
					jResp["error"]["error_id"] = 1;
					jResp["error"]["error_reason"] = "请求解析失败";
				} else if (ret == -2) {
					jResp["error"]["error_id"] = 1;
					jResp["error"]["error_reason"] = "排队超时";
				} else {
					jResp["error"]["error_id"] = 1;
					jResp["error"]["error_reason"] = "原因未明";
				}
			}
			worker->m_retS = jw.write(jResp);
			fprintf(stderr, "[PPView Ret] %s\n", worker->m_retS.c_str());
			snprintf(buff, sizeof(buff), "type=%s&uid=%s&qid=%s&tid=%s&fd=%d&htpTime=%ld&cIdxS=%s&cNum=%d&ret=%s", worker->m_qParam.type.c_str(), worker->m_qParam.uid.c_str(), worker->m_qParam.qid.c_str(), worker->m_tid.c_str(), worker->m_fd, worker->m_htpTime, worker->m_cIdxS.c_str(), worker->m_cNum, MJ::UrlEncode(worker->m_retS.c_str()).c_str());
			fprintf(stderr, "[PPView Ret: yangshu] %s\n", buff);
			worker->m_msg.assign(buff);
		} // if

		// 日志部分
		long long timeCost = WASTE_TIME_US(worker->m_recvTime);

		snprintf(buff, sizeof(buff), "耗时%.1fms", timeCost / 1000.0);
		if (!dbgStr.empty()) {
			dbgStr += "\t";
		}
		dbgStr += std::string(buff);

		MJ::PrintInfo::PrintLog("[%s]MQQueryProcessor::svc, uid=%s,qid=%s,cost=%ld,costD=%d,ret=%d,querystring @%s%s@,worker->result=%s,Owner=OP]", worker->m_qParam.log.c_str(), worker->m_qParam.uid.c_str(), worker->m_qParam.qid.c_str(), timeCost, routeCost, worker->m_retS.size(), worker->m_qParam.ReqHead().c_str(), worker->m_queryS.c_str(), worker->m_retS.c_str());

		time_t now = time(&now);
		fprintf(stderr, "[Request MiojiOPObserver,type=%s,uid=%s,qid=%s,ts=%ld,querystring=%s]\n", worker->m_qParam.type.c_str(), worker->m_qParam.uid.c_str(), worker->m_qParam.qid.c_str(), now, worker->m_queryS.c_str());
		fprintf(stderr, "[Response MiojiOPObserver,type=%s,uid=%s,qid=%s,ts=%ld,cost=%ld,ret=%s]\n", worker->m_qParam.type.c_str(), worker->m_qParam.uid.c_str(), worker->m_qParam.qid.c_str(), now, timeCost, worker->m_retS.c_str());
		fprintf(stderr, "[Debug MiojiOPObserver,type=%s,uid=%s,qid=%s,ts=%ld,debuginfo=\"%s\"]\n", worker->m_qParam.type.c_str(), worker->m_qParam.uid.c_str(), worker->m_qParam.qid.c_str(), now, dbgStr.c_str());

		const MQProducer* pMQProducer = MQProducer::GetInstance();
		pMQProducer->Publish(worker->m_msg, worker->m_mqRetKey);
		MJ::MJHotUpdate::delSharedDataPtr(threadID);
		delete worker;
	} // while
	return 0;
}

int MQQueryProcessor::ParseWorker(Worker* worker) {
	fprintf(stderr, "[GET QUEYR:[yangshu]]: %s\n", worker->m_uri.c_str());
	std::vector<std::pair<std::string, std::string> > itemList;
	ParseUri(worker->m_uri, itemList);

	for (int i = 0; i < itemList.size(); ++i) {
		std::string& keyS = itemList[i].first;
		std::string& valueS = itemList[i].second;
		if (keyS == "query") {
			worker->m_queryS = MJ::UrlDecode(valueS);
		} else if (keyS == "fd") {
			worker->m_fd = atoi(valueS.c_str());
		} else if (keyS == "htpTime") {
			worker->m_htpTime = strtoul(valueS.c_str(), (char**)NULL, 10);
		} else if (keyS == "cIdxS") {
			worker->m_cIdxS = valueS.c_str();
		} else if (keyS == "cNum") {
			worker->m_cNum = atoi(valueS.c_str());
		} else if (keyS == "tid") {
			worker->m_tid = valueS;
		} else if (keyS == "token") {
			worker->m_qParam.token = valueS;
		} else if (keyS == "type") {
			worker->m_qParam.type = valueS;
		} else if (keyS == "ver") {
			worker->m_qParam.ver = valueS;
		} else if (keyS == "lang") {
			worker->m_qParam.lang = valueS;
		} else if (keyS == "qid") {
			worker->m_qParam.qid = valueS;
		} else if (keyS == "uid") {
			worker->m_qParam.uid = valueS;
		} else if (keyS == "ccy") {
			worker->m_qParam.ccy = valueS;
		} else if (keyS == "dev") {
			worker->m_qParam.dev = atoi(valueS.c_str());
		} else if (keyS == "ptid") {
			worker->m_qParam.ptid = valueS;
		} else if (keyS == "csuid") {
			worker->m_qParam.csuid = valueS;
		} else if (keyS == "refer_id") {
			worker->m_qParam.refer_id = valueS;
		} else if (keyS == "cur_id") {
			worker->m_qParam.cur_id = valueS;
		} else if (keyS == "mqRetKey") {
			worker->m_mqRetKey = valueS;
		}
	}
	worker->m_qParam.wid = worker->m_wid;
	worker->m_qParam.Log();
	worker->m_qParam.ReqHead();

	MJ::PrintInfo::PrintLog("[%s]MQQueryProcessor::ParseWorker, Recv Query:%s%s", worker->m_qParam.log.c_str(), worker->m_qParam.ReqHead().c_str(), worker->m_queryS.c_str());
	return 0;
}

int MQQueryProcessor::ParseUri(const std::string& uri, std::vector<std::pair<std::string, std::string> >& retList) {
	std::string::size_type posE = std::string::npos;
	std::string::size_type posB = uri.rfind("&", posE);
	while (posB != std::string::npos) {
		std::string itemS = uri.substr(posB + 1, posE - posB - 1);
		std::string::size_type pos = itemS.find("=");
		if (pos != std::string::npos) {
			retList.push_back(std::pair<std::string, std::string>(itemS.substr(0, pos), itemS.substr(pos + 1)));
			posE = posB;
			posB = uri.rfind("&", posE - 1);
		} else {
			posB = uri.rfind("&", posB - 1);
		}
	}
	std::string itemS = uri.substr(0, posE);
	std::string::size_type pos = itemS.find("=");
	if (pos != std::string::npos) {
		retList.push_back(std::pair<std::string, std::string>(itemS.substr(0, pos), itemS.substr(pos + 1)));
	}
	return 0;
}

bool MQQueryProcessor::IsReady() {
	return (m_taskList.len() <= m_threadID * 3);
}
