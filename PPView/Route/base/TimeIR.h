/////////////////////////////////////////////////////////////
/*Filename   : TimeIR.h                                    */
/*Create Date: 2014-07-04                                  */
/*Author     : Wang Zhiwei 王志伟                           */
/*Version	 : 0.0.1                                       */

/*changed	 : 2014-08-16 by Wang Zhiwei 王志伟             */
/*reversion  : 0.0.2                                       */
/////////////////////////////////////////////////////////////

#ifndef _TIME_IR_H_
#define _TIME_IR_H_

#include "MJCommon.h"

class Week_range {
	public:
		int s_week;
		int e_week;
		
};

class Date_range {
	public:
		int s_date;
		int e_date;
};

class Rule_node {
	public:
		std::vector<Date_range> date_range;
		std::vector<Week_range> week_range;
		std::vector<std::string> time_range;
		int type;
		/*
		int show() {
			std::cout << "date range ..." << std::endl;
			for (int i = 0; i < date_range.size(); i++) {
				Date_range& date = date_range[i];
				std::cout << date.s_date << " - " << date.e_date << std::endl;
			}
			std::cout << "week range... " << std::endl;
			for (int i = 0; i < week_range.size(); i++) {
				Week_range& week = week_range[i];
				std::cout << week.s_week << " - " << week.e_week << std::endl;
			}
			std::cout << "time_range ... " << std::endl;
			for (int i = 0; i < time_range.size(); i++) {
				std::cout << time_range[i] << std::endl;
			}
		}
		*/
};

class TimeIR
{
public:
	//对外接口
	//判断玩乐当天日期是否可用 open_rule格式需满足数据库玩乐表open字段格式
	static bool isTheDateAvailable(const Json::Value& open_rule, const std::string& date);
	//获取当天开关门时间范围 rule格式需满足基础数据open字段格式
	static bool getTheDateOpenTimeRange(const std::string &rule, const std::string &in_date, std::vector<std::string> &out_timesList);
	//判断poi.open字段格式 <xxx><xxx><xxx><xxx>
	static bool CheckTimeRule(const std::string &rule);
	//按照delim分割in_str
    static void split(const std::string& in_str, const std::string& delim, std::vector<std::string>& out_vec);
    
	static int getWeekByDate(const std::string& in_date);
    static std::string int2String(int in_number);
    static int string2int(const std::string& in_str);

private:
    static std::string getStrBetween(std::string in_str, std::string left, bool is_left_forward, std::string right, bool is_right_forward);

	static void getOpenTimeRange(const std::vector<std::string> &time_range, const std::string &in_date, std::vector<std::string> &out_times);

	//判断in_str 是否在范围内
	static bool Check_week(const Week_range &week_range, const std::string in_str);
	static bool Check_date(const Date_range &date_range, const std::string in_str);

	//根据rule构造范围
	static void Date_rule(const std::string &in_str, Rule_node &rule);
	static void Week_rule(const std::string &in_str, Rule_node &rule);
	static void Time_rule(const std::string &in_str, Rule_node &rule);
	static void Type_rule(const std::string &in_str, Rule_node &rule);

	static void getVecStrBetween(std::string in_str,
								const std::string &left,
								const std::string &right,
								std::vector<std::string> &out_vecstr);
	static bool getData(const std::string &in_str, const std::string &key, bool order, int &date);
};

#endif
