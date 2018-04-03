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

#include <iostream>
#include <string>
#include <map>
//#include <unordered_map>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>

const static std::string TIME_RETURN_CLOSE = "";//描述NO类型规则:如果关闭，则返回空串.
const static std::string TIME_RETURN_UNKNOWN = "09:00:00-18:00:00";//未知景点默认开门时间.
const static std::string TIME_RETURN_OPEN = "09:00:00-18:00:00";//ALWAYS类型规则默认时间.
const static std::string TIME_RETURN_UNREG = "09:00:00-18:00:00";

const static std::string TIME_OPEN_CLOSE_DELIM = "|";//规则中open_rules和close_rules之间的分隔符.
const static std::string TIME_OPEN_RULE_DELIM = "--";//open_rules中多种类型之间的分隔符.
const static std::string TIME_RULE_TYPE_DELIM = "&";//规则与类型之间的分隔符.

const static std::string TIME_TYPE_UNRECOG_TIME = "UNREG_RULE";
const static std::string TIME_TYPE_UNKNOWN_TIME = "UNKNOWN_RULE";
const static std::string TIME_TYPE_AC_TIME = "AC_RULE";
const static std::string TIME_TYPE_AO_TIME = "AO_RULE";

const static std::string TIME_TYPE_DAY_TIME = "DAY_TIME_RULE";//优先级:5
const static std::string TIME_TYPE_MONTH_WEEK_TIME = "MONTH_WEEK_TIME_RULE";//优先级:4
const static std::string TIME_TYPE_WEEK_TIME = "WEEK_TIME_RULE";//优先级:3
const static std::string TIME_TYPE_MONTH_TIME = "MONTH_TIME_RULE";//优先级:2
const static std::string TIME_TYPE_ONLY_TIME = "TIME_RULE";//优先级:1


const int TIME_MONTH_DAY[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};//每月的天数.

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
		std::vector<Date_range> time_date;
		std::vector<Week_range> time_week;
		std::vector<std::string> time_range;
		int type;
};

class TimeIR
{
public:
    static int search(const std::string& in_rule, const std::string& in_date, std::string& out_times);
    //新增接口
    //static bool search(const std::string& in_rule, const std::string& in_date, std::string& out_times, std::string& out_type);
    
public:
    //处理DAY_TIME_RULE
    static bool checkDayTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times);
    //处理MONTH_WEEK_TIME_RULE
    static bool checkMonthWeekTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times);
    //处理WEEK_TIME_RULE
    static bool checkWeekTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times);
    //处理MONTH_TIME_RULE
    static bool checkMonthTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times);
    //处理TIME_RULE
    static bool checkTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times);

public:
	static void vector2String(const std::vector<std::string>& time_pair_vec, const std::string& delim, std::string& out_str);
    static void split(const std::string& in_str, const std::string& delim, std::vector<std::string>& out_vec);
    static std::string int2String(int in_number);
    static int string2int(const std::string& in_str);
    static std::string getStrBetween(std::string in_str, std::string left, bool is_left_forward, std::string right, bool is_right_forward);
    static bool isInInterval(const std::string& in_number_str, const std::string& interval_left_str, const std::string& interval_right_str);

    static int getWDay(const time_t& t, int zone = 8);
    static time_t toTime(const std::string& s, int zone = 8);
    static int compareDayStr(const std::string& a,const std::string& b);
    static std::string getDayOfWeek(const std::string& in_date);


public:
	static void getOpenTimeRange(const std::vector<std::string> &time_range, const std::string &in_date, std::string &out_times);
	static bool Check_week(const Week_range &week_range, const std::string in_str);
	static bool Check_date(const Date_range &date_range, const std::string in_str);
	static void Date_rule(const std::string &in_str, Rule_node &rule);
	static void Week_rule(const std::string &in_str, Rule_node &rule);
	static void Time_rule(const std::string &in_str, Rule_node &rule);
	static void Type_rule(const std::string &in_str, Rule_node &rule);
	static void getVecStrBetween(std::string in_str,
								const std::string &left,
								const std::string &right,
								std::vector<std::string> &out_vecstr);
	static bool getWeek(const std::string &in_str, const std::string &key, bool order, int &date);
	static bool getData(const std::string &in_str, const std::string &key, bool order, int &date);
};


#endif
