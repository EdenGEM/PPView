#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "RouteConfig.h"
#include "MJCommon.h"
#include "ToolFunc.h"



using namespace std;

/*多线程配置参数*/
int RouteConfig::thread_num=1;
int RouteConfig::thread_stack_size=102400000;

/*子服务相关参数*/
std::string RouteConfig::cache_server_addr;
std::string RouteConfig::traffic_server_addr;
std::string RouteConfig::hotel_server_addr;
std::string RouteConfig::view_server_addr;
std::string RouteConfig::traffic_redis_addr;
std::string RouteConfig::ticket_simple_server_addr;
std::string RouteConfig::ticket_detail_server_addr;
std::string RouteConfig::db_host;
std::string RouteConfig::db_user;
std::string RouteConfig::db_passwd;
std::string RouteConfig::db_name;
//zyc mysql_status
std::string RouteConfig::mysql_status;
//zyc end
std::string RouteConfig::dc_redis_address;
std::string RouteConfig::dc_redis_passwd;
int RouteConfig::dc_redis_db;
//smz private mysql
std::string RouteConfig::private_db_host;
std::string RouteConfig::private_db_user;
std::string RouteConfig::private_db_passwd;
std::string RouteConfig::private_db_name;

//smz end

int RouteConfig::traffic_server_port;

int RouteConfig::traffic_server_timeout;
int RouteConfig::view_server_timeout;
int RouteConfig::hotel_server_timeout;
int RouteConfig::ticket_server_timeout;

/*数据文件*/
std::string RouteConfig::data_dir;
std::vector<std::string> RouteConfig::freq_group_file;
std::vector<std::string> RouteConfig::traffic_white_file;
std::string RouteConfig::lFH_file;

int RouteConfig::debug_level;	//调试输出的级别
int RouteConfig::runtime_error;	//运行时出现异常的处理方式 0：继续 1：exit

bool RouteConfig::branch_day_limit;
double RouteConfig::dist_weight;
double RouteConfig::time_weight;
double RouteConfig::cross_weight;
bool RouteConfig::needPlanner = true;

RouteConfig::RouteConfig(){
}
RouteConfig::~RouteConfig(){
}

bool RouteConfig::init(const std::string& d_dir,const std::string& c_path){
	data_dir = d_dir;
	if (data_dir[data_dir.length() - 1] != '/')
		data_dir += "/";

	std::string fname = data_dir + "route.conf";
	ifstream fin;
	fin.open(fname.c_str());
	if (!fin){
		cerr << "can not open file " << fname<<endl;
		return false;
	}
	string line = "";
	while(!fin.eof()){
		getline(fin,line);
		if (line.length() == 0 || line[0] == '#')
			continue;
		int pos = line.find("=");
		if (pos == std::string::npos) {
			cerr << "[WARNING]:format err->" << line << endl;
			continue;
		}
		string key = line.substr(0,pos);
		string val = line.substr(pos+1);

		if (key == "freq_group_file") {
			ToolFunc::sepString(val, ";", freq_group_file);
		} else if (key == "traffic_white_file") {
			ToolFunc::sepString(val, ";", traffic_white_file);
		} else if(key =="lFH_file") {
			lFH_file = val;
		} else if (key == "branch_day_limit") {
			branch_day_limit = atoi(val.c_str());
		} else if (key == "dist_weight") {
			dist_weight = atof(val.c_str());
		} else if (key == "time_weight") {
			time_weight = atof(val.c_str());
		} else if (key == "cross_weight") {
			cross_weight = atof(val.c_str());
		} else if (key =="mysql_status") {
			mysql_status = val;
		}
	}
	fin.close();

	fname = c_path;
	fin.open(fname.c_str());
	if (!fin){
		cerr<<"can not open file "<<fname<<endl;
		return false;
	}
	while(!fin.eof()){
		getline(fin,line);
        if (line.length()==0||line[0]=='#')
        	continue;
        int pos = line.find("=");
        if (pos==std::string::npos){
        	cerr<<"[WARNING]:format err->"<<line<<endl;
        	continue;
        }
        string key = line.substr(0,pos);
        string val = line.substr(pos+1);
	if (key=="cache_server_addr")
        	cache_server_addr = val;
        else if (key=="traffic_server_addr")
        	traffic_server_addr = val;
        else if (key=="traffic_server_port")
            traffic_server_port = atoi(val.c_str());
        else if (key=="traffic_server_timeout")
            traffic_server_timeout = atoi(val.c_str());
        else if (key=="ticket_simple_server_addr")
        	ticket_simple_server_addr = val;
        else if (key=="ticket_detail_server_addr")
        	ticket_detail_server_addr = val;
        else if (key=="ticket_server_timeout")
            ticket_server_timeout = atoi(val.c_str());
        else if (key=="view_server_addr")
            view_server_addr = val;
        else if (key=="view_server_timeout")
            view_server_timeout = atoi(val.c_str());
        else if (key=="hotel_server_addr")
            hotel_server_addr = val;
        else if (key=="traffic_redis_addr")
            traffic_redis_addr = val;
        else if (key=="hotel_server_timeout")
            hotel_server_timeout = atoi(val.c_str());
        else if (key=="thread_num")
            thread_num = atoi(val.c_str());
        else if (key=="thread_stack_size")
            thread_stack_size = atoi(val.c_str());
        else if (key == "db_host")
            db_host = val;
        else if (key == "db_user")
            db_user = val;
        else if (key == "db_passwd")
            db_passwd = val;
        else if (key == "db_name")
            db_name = val;
        else if (key == "dc_redis_address")
            dc_redis_address = val;
        else if (key == "dc_redis_passwd")
            dc_redis_passwd = val;
        else if (key == "dc_redis_db")
            dc_redis_db = atoi(val.c_str());
        else if (key == "private_db_host")
            private_db_host = val;
        else if (key == "private_db_user")
            private_db_user = val;
        else if (key == "private_db_passwd")
            private_db_passwd = val;
        else if (key == "private_db_name")
            private_db_name = val;
	//else if (key == "NeedPlanner")
        //    needPlanner = atoi(val.c_str());
	else if (key == "mysql_status")
            mysql_status = val;
	}
	fin.close();

	std::cerr << "route config init success" << std::endl;
	return true;

}




