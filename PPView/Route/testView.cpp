#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include "MJCommon.h"
#include "base/BagParam.h"
#include "Route.h"
#include <mysql/mysql.h>
#include <regex>
#include <sys/time.h>

using namespace std;

const regex norm_sql_pat("\\\\");
const regex norm_sql_pat1("(['\"])");

std::string dataPath = "../data/";
std::string confPath = "../conf/server.cfg";

void sigterm_handler(int signo) {}
void sigint_handler(int signo) {}

std::string NormSql(std::string& sql) {
	std::string ret;
	ret = regex_replace(sql, norm_sql_pat, "\\\\\\\\");
	ret = regex_replace(ret, norm_sql_pat1, "\\\\$1");
	return ret;
}

int ParseQuery(std::string& line, QueryParam& param, Json::Value& req) {
	std::vector<std::string> itemList;
	std::string query;
	std::string::size_type posSt = line.find("query=");
	posSt += strlen("query=");
	std::string::size_type end = line.size();
	std::string::size_type posEd = -1;
	int left = 0;
	for(std::string::size_type i = posSt ; i < end ; ++i) {
		if (line[i] == '{') {
			left ++;
		} else if (line[i] == '}') {
			left --;
		}
		if (left == 0) {
			posEd = i;
			break;
		}
	}
	if (posEd != -1) {
		std::string::size_type st = posSt;
		while(st < posEd) {
			if (line[st] == '&') {
				line.replace(st, strlen("&") , "(and)");
			}
			if (line[st] == '=') {
				line.replace(st, strlen("="), "(equal)");
			}
			++st;
		}
	}

	std::cerr << line << std::endl;

	ToolFunc::sepString(line, "&", itemList);

	for (int i = 0; i < itemList.size(); ++i) {
		std::string& item = itemList[i];
		std::string::size_type pos = item.find('=');
		if (pos == std::string::npos) continue;
		std::string key = item.substr(0, pos);
		std::string value = item.substr(pos + 1);
		if (key == "token") {
			param.token = value;
		} else if (key == "type") {
			param.type = value;
		} else if (key == "ver") {
			param.ver = value;
		} else if (key == "lang") {
			param.lang = value;
		} else if (key == "qid") {
			param.qid = value;
		} else if (key == "uid") {
			param.uid = value;
		} else if (key == "dev") {
			param.dev = atoi(value.c_str());
		} else if (key == "wid") {
			param.wid = value;
		} else if (key == "ptid") {
			param.ptid = value;
		} else if (key == "query") {
			query = value;
		}
	}
	param.Log();
	param.ReqHead();

	Json::Reader jr;
	jr.parse(query, req);
	return 0;
}

int WriteSql(Json::Value& req, QueryParam& qParam, Json::Value& resp) {
/*jjj
	MYSQL mysql;
	mysql_init(&mysql);
	if (!mysql_real_connect(&mysql, "data", "reader", "miaoji1109", "test", 0, NULL, 0)) {
		MJ::PrintInfo::PrintErr("TestNorm, Connect to %s error: %s", "data", mysql_error(&mysql));
		return 1;
	}
	if (mysql_set_character_set(&mysql, "utf8")) {
		MJ::PrintInfo::PrintErr("TestNorm, Set mysql characterset: %s", mysql_error(&mysql));
		return 1;
	}
jjj*/

	Json::FastWriter jw;
	std::string resp_str = jw.write(resp);
	std::string req_str = jw.write(req);
	ToolFunc::rtrim(resp_str);
	ToolFunc::rtrim(req_str);

	timeval tv;
	gettimeofday(&tv, NULL);
//	long long qid = ((long long)tv.tv_sec) * 1000000 + (long long)tv.tv_usec;
	char buff[1000000];
	snprintf(buff, sizeof(buff), "replace into china set qid='%s',query='%s',request='%s';", qParam.qid.c_str(), NormSql(req_str).c_str(), NormSql(resp_str).c_str());
	std::string sql(buff);
	fprintf(stdout, "\n%s\n", buff);
/*jjj
	int ret = mysql_query(&mysql, sql.c_str());
	if (ret != 0) {
		fprintf(stderr, "TestNorm, write sql failed\n");
		fprintf(stderr, "TestNorm, mysql_query error: %s, error sql: %s\n", mysql_error(&mysql), sql.c_str());
		return 1;
	} else {
		fprintf(stderr, "TestNorm, query[%s], qid: %ld\n", qParam.qid.c_str(), qid);
	}
	mysql_close(&mysql);
jjj*/
//	fprintf(stderr, "miojiParam\t%s\t%ld\n", qParam.qid.c_str(), qid);

	return 0;
}

int TestNorm(std::vector<std::string>& qList, Route* route) {
	for (int i = 0; i < qList.size(); ++i) {
		std::string query = qList[i];

		QueryParam param;
		Json::Value req;
		Json::Value resp;
		ParseQuery(query, param, req);
		req["localTest"]=1;

		MJ::MyTimer t;
		int ret = route->DoRoute(param, req, resp);
		Json::FastWriter jw;
		MJ::PrintInfo::PrintLog("[%s]TestNorm, querystring @%s@ ret=%s", param.log.c_str(), query.c_str(), jw.write(resp).c_str());
		fprintf(stderr, "TestNorm, query[%d], ret=%d, cost=%d\n", i, ret, t.cost());
		std::cout << "resp:\n" << resp << std::endl;

		WriteSql(req, param, resp);
	}
	return 0;
}

int GetQueryList(const std::string& dirPath, std::vector<std::string>& qList) {
	std::vector<std::string> fPathList;

	struct stat info;
	stat(dirPath.c_str(), &info);
	if (S_ISDIR(info.st_mode)) {
		struct dirent* ent = NULL;
		DIR *pDir;
		pDir = opendir(dirPath.c_str());
		while (NULL != (ent = readdir(pDir))) {
			if (ent->d_type == 8 || ent->d_type == 0) {
				std::string fPath = dirPath + "/" + ent->d_name;
				fPathList.push_back(fPath);
			}
		}
	} else if (S_ISREG(info.st_mode)) {
		fPathList.push_back(dirPath);
	} else {
		fprintf(stderr, "GetQueryList, file type err: %s\n", dirPath.c_str());
		return 1;
	}
	for (int i = 0; i < fPathList.size(); ++i) {
		std::string fPath = fPathList[i];
		fprintf(stderr, "GetQueryList: file[%d] %s\n", i, fPath.c_str());
		std::ifstream fin(fPath.c_str());
		if (!fin) {
			fprintf(stderr, "GetQueryList, open file(%s) fail!\n", fPath.c_str());
			continue;
		}
		std::string line;
		while (std::getline(fin, line)) {
			if (!line.empty() && line[0] == '#') continue;
			qList.push_back(line);
		}
		fin.close();
	}
	return 0;
}

int main(int argc, char** argv) {
	close(STDIN_FILENO);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, &sigterm_handler);
	signal(SIGINT, &sigint_handler);

	if (argc < 2) {
		fprintf(stderr, "usage: %s file use_dbg \n", argv[0]);
		return 1;
	}

	std::string dirPath = argv[1];
	// 关闭dbg
	if (argc > 2 && std::string(argv[2]) == "0") {
		MJ::PrintInfo::SwitchDbg(0, 1, 1, 1);
	}

	std::vector<std::string> qList;
	GetQueryList(dirPath, qList);
	std::sort(qList.begin(), qList.end());

	bool ret = Route::LoadStatic(dataPath, confPath);
	if (ret) {
		MJ::PrintInfo::PrintDbg("[Route] loadstatic ERR");
		return 1;
	}
	Route* route = new Route;

	TestNorm(qList, route);

	delete route;
	Route::ReleaseStatic();

	return 0;
}

