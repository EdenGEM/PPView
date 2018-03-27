#ifndef __ROUTE_CONFIG_H__
#define __ROUTE_CONFIG_H__

#include <string>
#include <vector>


class RouteConfig{
public:
	RouteConfig();
	~RouteConfig();
	static bool init(const std::string& data_dir,const std::string& conf_path);
public:
	/*多线程配置参数*/
	static int thread_num;
	static int thread_stack_size;

	/*子服务相关参数*/
	static std::string cache_server_addr;
	static std::string traffic_server_addr;
	static std::string view_server_addr;
	static std::string hotel_server_addr;
    static std::string traffic_redis_addr;
	static std::string ticket_simple_server_addr;
	static std::string ticket_detail_server_addr;
	//port端口
	static int traffic_server_port;
	//超时
	static int traffic_server_timeout;
	static int view_server_timeout;
	static int hotel_server_timeout;
	static int ticket_server_timeout;

	/*onlinedb的静态数据*/
	static std::string db_host;
	static std::string db_user;
	static std::string db_passwd;
	static std::string db_name;
	//zyc
	static std::string mysql_status; // 0:status_test='Open', 1:status_online='Open'

	/*data_center redis 地址*/
	static std::string dc_redis_address;
	static std::string dc_redis_passwd;
	static int dc_redis_db;
	//zyc end
	// smz
	//private 库的实时更新数据库
	static std::string private_db_host;
	static std::string private_db_user;
	static std::string private_db_passwd;
	static std::string private_db_name;
	//smz end
	/*数据文件*/
	static std::string data_dir;
	static std::vector<std::string> freq_group_file;
	static std::vector<std::string> traffic_white_file;
	static std::vector<std::string> hot_level_file;
	static std::string lFH_file;

	/*其他*/
	static int debug_level;	//调试输出的级别
	static int runtime_error;	//运行时出现异常的处理方式 0：继续 1：exi

	static bool branch_day_limit;

	static double dist_weight;
	static double time_weight;
	static double cross_weight;

	static bool needPlanner;  // 是否需要规划类

};

#endif	//__ROUTE_CONFIG_H__



