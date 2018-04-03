#include "TimeIR.h"

/********************************************//**
 * \brief 从规则rule中检索指定日期in_date的景点开关门时间.
 *
 * \param rule 字符串, 格式为:{10:00:00-13:00:00,16:00:00-20:00:00}&TIME_RULE|NO&NO
 * \param in_date 字符串, 格式为:20140915
 * \param out_times 字符串, 格式为:10:00:00-13:00:00,16:00:00-20:00:00
 * \return bool 如果结果确定返回true, 否则返回false.
 *
 ***********************************************/
 /*
bool TimeIR::search(const std::string& rule, const std::string& in_date, std::string& out_times)
{
	// std::cout << "rule[" << rule << "]" << std::endl;
	// std::cout << "i_date[" << in_date << "]" << std::endl;

	std::vector<std::string> rule_items;
	split(rule, TIME_OPEN_CLOSE_DELIM, rule_items);
	if(rule_items.size() != 3)
	{
		// std::cout << "size[" << rule_items.size() << "]" << endl;
		std::cout << "输入规则不合法! rule[" << rule << "] in_date[" << in_date << "]" << std::endl;
		out_times = TIME_RETURN_UNKNOWN;
		return false;
	}

	bool isSure = false;
	if (rule_items[2] == "SURE")
	{
		isSure = true;
	}

	//step1. 首先检索close_rules, 如果命中，直接返回结果.
	//NOTE: close_rules只有两种类型:NO, DAY_TIME_RULE.
	std::vector<std::string> close_items;
	split(rule_items[1], TIME_RULE_TYPE_DELIM, close_items);
	std::string close_rule = close_items[0];
	std::string close_type = close_items[1];
	std::string normalized_date = in_date.substr(4);

	if(TIME_TYPE_DAY_TIME == close_type)
	{
		close_rule = getStrBetween(close_rule, "{", true, "}", false);
		std::vector<std::string> black_day_vec;
	    split(close_rule, ",", black_day_vec);
	    for(std::vector<std::string>::iterator iter = black_day_vec.begin(); iter != black_day_vec.end(); ++iter)
	    {
	    	if(normalized_date == *iter)
	    	{
	    		out_times = TIME_RETURN_CLOSE;

	    		return isSure;
	    	}
	    }
	}
	
	//step2. 检索open_rules. 不用考虑close_rules(step1已经解决).
	std::vector<std::string> open_rules;
	split(rule_items[0], TIME_OPEN_RULE_DELIM, open_rules);
	// std::cout << "open_rules size[" << open_rules.size() << "]" << endl;
	std::map<std::string, std::string> opentype_rule_map;
	for(std::vector<std::string>::iterator iter = open_rules.begin(); iter != open_rules.end(); ++iter)
	{
		std::vector<std::string> tmp_vec;
		split(*iter, TIME_RULE_TYPE_DELIM, tmp_vec);
		opentype_rule_map[tmp_vec[1]] = tmp_vec[0];
		// std::cout << tmp_vec[1] << "," << tmp_vec[0] << std::endl;
	}

	// std::cout << "------" << opentype_rule_map[TIME_TYPE_ONLY_TIME] << endl;

	//根据优先级依次进行时间规则检索.
	std::map<std::string, std::string>::const_iterator end_iter = opentype_rule_map.end();
	if (opentype_rule_map.find("ALWAYS") != end_iter)
	{
		// std::cout << "Match ALWAYS!" << std::endl;
		out_times = TIME_RETURN_OPEN;
		return isSure;
	}
	if (opentype_rule_map.find("NO") != end_iter)
	{
		std::cout << "Match NO! rule[" << rule << "]in_date:[" << in_date << "]" << std::endl;
		out_times = TIME_RETURN_CLOSE;
		return isSure;
	}
	if (opentype_rule_map.find("UNKNOWN") != end_iter)
	{
		// std::cout << "Match UNKNOWN!" << std::endl;
		out_times = TIME_RETURN_UNKNOWN;
		return isSure;
	}
	if (opentype_rule_map.find("NULL_RULE") != end_iter)
	{
		out_times = TIME_RETURN_UNKNOWN;
		return isSure;
	}
	if (opentype_rule_map.find("UNREG_RULE") != end_iter)
	{
		out_times = TIME_RETURN_UNREG;
		return isSure;
	}
	if (opentype_rule_map.find(TIME_TYPE_DAY_TIME) != end_iter)
	{
		std::cout << "checking DAY_TIME_RULE" << std::endl;
		if (checkDayTimeRule(opentype_rule_map[TIME_TYPE_DAY_TIME], in_date, out_times))
		{
			return isSure;	
		}
		
	}
	if (opentype_rule_map.find(TIME_TYPE_MONTH_WEEK_TIME) != end_iter)
	{
		std::cout << "checking TIME_TYPE_MONTH_WEEK_TIME" << std::endl;
		if (checkMonthWeekTimeRule(opentype_rule_map[TIME_TYPE_MONTH_WEEK_TIME], in_date, out_times))
		{
			return isSure;	
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_WEEK_TIME) != end_iter)
	{
		std::cout << "checking TIME_TYPE_WEEK_TIME" << std::endl;
		if (checkWeekTimeRule(opentype_rule_map[TIME_TYPE_WEEK_TIME], in_date, out_times))
		{
			return isSure;	
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_MONTH_TIME) != end_iter)
	{
		std::cout << "checking TIME_TYPE_MONTH_TIME" << std::endl;
		if (checkMonthTimeRule(opentype_rule_map[TIME_TYPE_MONTH_TIME], in_date, out_times))
		{
			return isSure;	
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_ONLY_TIME) != end_iter)
	{
		std::cout << "checking TIME_TYPE_ONLY_TIME" << std::endl;
		// std::cout << "checking TIME_ONLY" << endl;
		if (checkTimeRule(opentype_rule_map[TIME_TYPE_ONLY_TIME], in_date, out_times))
		{
			return isSure;	
		}
	}
	std::cout << "Match Nothing!";
	std::cout << "in_rule:[" << rule << "]in_date:[" << in_date << "]" << std::endl;
	out_times = TIME_RETURN_CLOSE;
	return isSure;
}*/

/*bool TimeIR::search(const std::string& in_rule, const std::string& in_date, std::string& out_times, std::string& out_type)
{
	// std::cout << "rule[" << rule << "]" << std::endl;
	// std::cout << "i_date[" << in_date << "]" << std::endl;
	std::vector<std::string> rule_items;
	split(in_rule, TIME_OPEN_CLOSE_DELIM, rule_items);
	if(rule_items.size() != 2)
	{
		// std::cout << "size[" << rule_items.size() << "]" << endl;
		// std::cout << "输入规则不合法! rule[" << rule << "] in_date[" << in_date << "]" << endl;
		out_times = TIME_RETURN_UNKNOWN;
		out_type = TIME_TYPE_UNKNOWN_TIME;
		return false;
	}

	//step1. 首先检索close_rules, 如果命中，直接返回结果.
	//NOTE: close_rules只有两种类型:NO, DAY_TIME_RULE.
	std::vector<std::string> close_items;
	split(rule_items[1], TIME_RULE_TYPE_DELIM, close_items);
	std::string close_rule = close_items[0];
	std::string close_type = close_items[1];
	std::string normalized_date = in_date.substr(4);
	if(TIME_TYPE_DAY_TIME == close_type)
	{
		close_rule = getStrBetween(close_rule, "{", true, "}", false);
		std::vector<std::string> black_day_vec;
	    split(close_rule, ",", black_day_vec);
	    for(std::vector<std::string>::iterator iter = black_day_vec.begin(); iter != black_day_vec.end(); ++iter)
	    {
	    	if(normalized_date == *iter)
	    	{
	    		out_times = TIME_RETURN_CLOSE;
	    		out_type = TIME_TYPE_DAY_TIME;
	    		return true;
	    	}
	    }
	}
	
	//step2. 检索open_rules. 不用考虑close_rules(step1已经解决).
	std::vector<std::string> open_rules;
	split(rule_items[0], TIME_OPEN_RULE_DELIM, open_rules);
	// std::cout << "open_rules size[" << open_rules.size() << "]" << endl;
	std::map<std::string, std::string> opentype_rule_map;
	for(std::vector<std::string>::iterator iter = open_rules.begin(); iter != open_rules.end(); ++iter)
	{
		std::vector<std::string> tmp_vec;
		split(*iter, TIME_RULE_TYPE_DELIM, tmp_vec);
		opentype_rule_map[tmp_vec[1]] = tmp_vec[0];
		// std::cout << tmp_vec[1] << "," << tmp_vec[0] << std::endl;
	}

	// std::cout << "------" << opentype_rule_map[TIME_TYPE_ONLY_TIME] << endl;

	//根据优先级依次进行时间规则检索.
	std::map<std::string, std::string>::const_iterator end_iter = opentype_rule_map.end();
	if (opentype_rule_map.find("ALWAYS") != end_iter)
	{
		// std::cout << "Match ALWAYS!" << std::endl;
		out_times = TIME_RETURN_OPEN;
		out_type = TIME_TYPE_AO_TIME;
		return true;
	}
	if (opentype_rule_map.find("NO") != end_iter)
	{
		// std::cout << "Match NO!" << std::endl;
		out_times = TIME_RETURN_CLOSE;
		out_type = TIME_TYPE_AC_TIME;
		return true;
	}
	if (opentype_rule_map.find("UNREG_RULE") != end_iter)
	{
		out_times = TIME_RETURN_UNREG;
		out_type = TIME_TYPE_UNRECOG_TIME;
		return true;
	}
	if (opentype_rule_map.find("NULL_RULE") != end_iter)
	{
		out_times = TIME_RETURN_UNKNOWN;
		out_type = TIME_TYPE_UNKNOWN_TIME;
		return true;
	}
	if (opentype_rule_map.find(TIME_TYPE_DAY_TIME) != end_iter)
	{
		if(checkDayTimeRule(opentype_rule_map[TIME_TYPE_DAY_TIME], in_date, out_times))
		{
			// std::cout << "Match TIME_TYPE_DAY_TIME!" << std::endl;
			out_type = TIME_TYPE_DAY_TIME;
			return true;
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_MONTH_WEEK_TIME) != end_iter)
	{
		if (checkMonthWeekTimeRule(opentype_rule_map[TIME_TYPE_MONTH_WEEK_TIME], in_date, out_times))
		{
			// std::cout << "Match TIME_TYPE_MONTH_WEEK_TIME!" << std::endl;
			out_type = TIME_TYPE_MONTH_WEEK_TIME;
			return true;
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_WEEK_TIME) != end_iter)
	{
		if (checkWeekTimeRule(opentype_rule_map[TIME_TYPE_WEEK_TIME], in_date, out_times))
		{
			// std::cout << "Match TIME_TYPE_WEEK_TIME!" << std::endl;
			out_type = TIME_TYPE_WEEK_TIME;
			return true;
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_MONTH_TIME) != end_iter)
	{
		if (checkMonthTimeRule(opentype_rule_map[TIME_TYPE_MONTH_TIME], in_date, out_times))
		{
			// std::cout << "Match TIME_TYPE_MONTH_TIME!" << std::endl;
			out_type = TIME_TYPE_MONTH_TIME;
			return true;
		}
	}
	if (opentype_rule_map.find(TIME_TYPE_ONLY_TIME) != end_iter)
	{
		// std::cout << "checking TIME_ONLY" << endl;
		if (checkTimeRule(opentype_rule_map[TIME_TYPE_ONLY_TIME], in_date, out_times))
		{
			// std::cout << "Match TIME_TYPE_ONLY_TIME!" << std::endl;
			out_type = TIME_TYPE_ONLY_TIME;
			return true;
		}
	}
	// std::cout << "Match Nothing!";
	out_times = TIME_RETURN_CLOSE;
	out_type = TIME_TYPE_DAY_TIME;
	return false;
}*/

/********************************************//**
 * \brief 处理DAY_TIME_RULE; 检索输入日期是否在给定的in_rule中. 如果在，则返回对应的时间.
 *
 * \param in_rule 字符串, 格式为:{0515-0930:06:00:00-22:00:00},{1001-1231:06:00:00-21:30:00}
 * \param in_date 字符串, 格式为:20140915
 * \param out_times 字符串, 格式为:09:00-12:00,13:00-15:00
 *
 ***********************************************/
bool TimeIR::checkDayTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times)
{
	std::string normalized_date = in_date.substr(4);

	std::vector<std::string> time_pair_vec;
	split(in_rule, ",", time_pair_vec);
	std::string time_pair, left_date, right_date;
	for(std::vector<std::string>::iterator iter = time_pair_vec.begin(); iter != time_pair_vec.end(); ++iter)
	{
		left_date = getStrBetween(*iter, "{", true, "-", true);

		right_date = getStrBetween(*iter, "-", true, ":", true);
//		std::cout << "normalized_date[" << normalized_date << "]";
//		std::cout << "left_date[" << left_date << "]";
		// std::cout << "right_date[" << right_date << "]" << std::endl;
		// std::cout << compareDayStr(left_date, normalized_date) << std::endl;
		// std::cout << compareDayStr(normalized_date, right_date) << std::endl;
		if ((compareDayStr(left_date, normalized_date) <= 0) && (compareDayStr(normalized_date, right_date) <= 0))
		{
			out_times = getStrBetween(*iter, ":", true, "}", false);
			return true;
		}
	}
	return false;
}

/********************************************//**
 * \brief 处理MONTH_WEEK_TIME_RULE; 检索输入日期是否在给定的in_rule中. 如果在，则返回对应的时间.
 *
 * \param in_rule 字符串, 格式为:{10-12_1-5:08:00:00-18:00:00},{1-2_1-5:08:00:00-18:00:00}
 * \param in_date 字符串, 格式为:20140915
 * \param out_times 字符串, 格式为:09:00-12:00,13:00-15:00
 *
 ***********************************************/
bool TimeIR::checkMonthWeekTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times)
{
	std::vector<std::string> vec;
	split(in_rule, ",", vec);
	std::string normalized_date = in_date.substr(4);
	std::string in_month = normalized_date.substr(0, normalized_date.length() / 2);
	std::string in_weekday = getDayOfWeek(in_date);

	std::string weekday_pair;
	std::string month_left, month_right;
	for(std::vector<std::string>::iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{
		month_left = getStrBetween(*iter, "{", true, "-", true);
		month_right = getStrBetween(*iter, "-", true, "_", true);
		if (isInInterval(in_month, month_left, month_right))
		{
			weekday_pair = getStrBetween(*iter, "_", true, ":", true);
			std::vector<std::string> number_vec;
			split(weekday_pair, "-", number_vec);
			if (isInInterval(in_weekday, number_vec[0], number_vec[1]))
			{
				out_times = getStrBetween(*iter, ":", true, "}", false);
				return true;
			}
		}
	}

	return false;
}

/********************************************//**
 * \brief 处理WEEK_TIME_RULE; 检索输入日期是否在给定的in_rule中. 如果在，则返回对应的时间.
 *
 * \param in_rule 字符串, 格式为:{1-1:09:00-18:00},{2-2:09:00-18:00},{3-3:09:00-18:00},{4-4:09:00-18:00}
 * \param in_date 字符串, 格式为:20140915
 * \param out_times 字符串, 格式为:09:00-12:00,13:00-15:00
 *
 ***********************************************/
bool TimeIR::checkWeekTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times)
{
	std::string in_weekday = getDayOfWeek(in_date);
	std::vector<std::string> vec;
	split(in_rule, ",", vec);
	std::string weekday_left, weekday_right;
	for (std::vector<std::string>::iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{
		weekday_left = getStrBetween(*iter, "{", true, "-", true);
		weekday_right = getStrBetween(*iter, "-", true, ":", true);
		if(isInInterval(in_weekday, weekday_left, weekday_right))
		{
			out_times = getStrBetween(*iter, ":", true, "}", false);
			return true;
		}
	}

	return false;
}

/********************************************//**
 * \brief 处理MONTH_TIME_RULE; 检索输入日期是否在给定的in_rule中. 如果在，则返回对应的时间.
 *
 * \param in_rule 字符串, 格式为:{2-2:09:00-18:00},{3-3:09:00-18:00},{4-4:09:00-18:00}
 * \param in_date 字符串, 格式为:20140915
 * \param out_times 字符串, 格式为:09:00-12:00,13:00-15:00
 *
 ***********************************************/
bool TimeIR::checkMonthTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times)
{
	std::string normalized_date = in_date.substr(4);
	std::string in_month = normalized_date.substr(0, normalized_date.size() / 2);
	std::vector<std::string> vec;
	split(in_rule, ",", vec);
	std::string month_left, month_right;
	for(std::vector<std::string>::iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{	
		month_left = getStrBetween(*iter, "{", true, "-", true);
		month_right = getStrBetween(*iter, "-", true, ":", true);
		// std::cout << "month_left[" << month_left << "]month_right[" << month_right << "]in_month[" << in_month << "]in_date[" << in_date <<"]" << endl;
		if (isInInterval(in_month, month_left, month_right))
		{
			out_times = getStrBetween(*iter, ":", true, "}", false);
			return true;
		}
	}

	return false;
}

/********************************************//**
 * \brief 处理TIME_RULE; 检索输入日期是否在给定的in_rule中. 如果在，则返回对应的时间.
 *
 * \param in_rule 字符串, 格式为:{09:00:00-17:00:00}
 * \param in_date 字符串, 格式为:20140915, 输入日期暂时没有使用.
 * \param out_times 字符串, 格式为:09:00-12:00,13:00-15:00
 *
 ***********************************************/
bool TimeIR::checkTimeRule(const std::string& in_rule, const std::string& in_date, std::string& out_times)
{
	out_times = getStrBetween(in_rule, "{", true, "}", false);
	return true;
}

/********************************************//**
 * \brief 将给定字符串in_str按照分隔符delim进行分割，结果存储在out_vec中
 *
 * \param in_str 给定的字符串
 * \param delim 分隔符
 * \param out_vec 存储使用分隔符分割后的子串
 *
 ***********************************************/
void TimeIR::split(const std::string& in_str, const std::string& delim, std::vector<std::string>& out_vec)
{
	out_vec.clear();
    
    std::string::size_type pos = 0;
    std::string::size_type len = in_str.length();
    std::string::size_type delim_len = delim.length();
    if (0 == len || 0 == delim_len)
    {
    	return;
    }
    while (pos < len)
    {
        std::string::size_type find_pos = in_str.find(delim, pos);
        //std::cout << in_str.substr(pos, len - pos) << std::endl;
        if (find_pos == -1)
        {
            out_vec.push_back(in_str.substr(pos, len - pos));
            break;
        }
        out_vec.push_back(in_str.substr(pos, find_pos - pos));
        pos = find_pos + delim_len;
    }
}

std::string TimeIR::int2String(int in_number)
{
    std::stringstream ss;
	ss << in_number;
    std::string number_str;
	ss >> number_str;
    
	return number_str;
}

int TimeIR::string2int(const std::string& in_str)
{
    std::stringstream ss;
	ss << in_str;
	int number;
	ss >> number;
    
	return number;
}

std::string TimeIR::getStrBetween(std::string in_str, std::string left, bool is_left_forward, std::string right, bool is_right_forward)
{
	std::size_t left_index;
	if (is_left_forward)
	{
		left_index = in_str.find(left);
	}
	else
	{
		left_index = in_str.rfind(left);
	}

	std::size_t right_index;
	if (is_right_forward)
	{
		right_index = in_str.find(right);
	}
	else
	{
		right_index = in_str.rfind(right);
	}

	return in_str.substr(left_index + left.length(), right_index - left_index - right.length());
}

bool TimeIR::isInInterval(const std::string& in_number_str, const std::string& interval_left_str, const std::string& interval_right_str)
{
	int in_number = string2int(in_number_str);
	int left = string2int(interval_left_str);
	int right = string2int(interval_right_str);

	return left <= in_number && in_number <= right;
}

time_t TimeIR::toTime(const std::string& s, int zone)
{
	const char* s_c = s.c_str();
	char* pstr;
	long year, month, day, hour, min, sec;
	year = strtol(s_c, &pstr, 10);
	month = (year / 100) % 100;
	day = (year % 100);
	year = year / 10000;
	hour = min = sec = 0;
	if(*pstr)
	{
		hour = strtol(++pstr, &pstr, 10);
		if(*pstr)
		{
			min = strtol(++pstr, &pstr, 10);
			if (*pstr)
			{
				sec = strtol(++pstr, &pstr, 10);
			}
		}
	}
	//printf("%d %d %d\n", hour,min,sec);

    int leap_year = (year - 1969) / 4;	//year-1-1968
    int i = (year - 2001) / 100;		//year-1-2000
    leap_year -= ((i / 4) * 3 + i % 4);
    int day_offset = 0;
    for (i = 0; i < month - 1; i++)
    	day_offset += TIME_MONTH_DAY[i];
    bool isLeap = ((year % 4 == 0 && year % 100 != 0)||(year % 400 == 0));
    if (isLeap && month>2)
    	day_offset += 1;
    day_offset += (day-1);
    day_offset += ((year - 1970) * 365 + leap_year);
    int hour_offset = hour - zone;
    time_t ret = day_offset * 86400 + hour_offset * 3600 + min * 60 + sec;

    return ret;
}


int TimeIR::compareDayStr(const std::string& a, const std::string& b)
{
	if (a == b)
		return 0;

	time_t a_t = toTime("2014" + a + "_00:00", 0);
	time_t b_t = toTime("2014" + b + "_00:00", 0);

	return (a_t - b_t) / 86400;
}

int TimeIR::getWDay(const time_t& t, int zone)
{
	struct tm TM;
	time_t nt = t + zone * 3600;
	gmtime_r(&nt, &TM);
	int ret = (TM.tm_wday + 6) % 7 + 1;
	return ret;
}

std::string TimeIR::getDayOfWeek(const std::string& in_date)
{
	
	time_t tt = TimeIR::toTime(in_date+"_00:00");
    int wd = TimeIR::getWDay(tt);

	return int2String(wd);
}



int TimeIR::search(const std::string &rule, const std::string &in_date, std::string &out_times) {
	int idx = rule.rfind(">");
	const std::string rule_t = rule.substr(0,idx+1);
	std::vector<std::string> rule_items;
	split(rule_t, "|", rule_items);
	std::vector<Rule_node> Rule;
	//std::cout << "rule = " << rule << std::endl;
	for(int i = 0; i < rule_items.size(); i ++) {
		//std::cout << "rule_items[i] = " << rule_items[i] << std::endl;
		std::vector<std::string> items;
		getVecStrBetween(rule_items[i], "<", ">", items);
		if(4 != items.size()) {
//			std::cout << "输入规则不合法! rule[" << rule << "] in_date[" << in_date << "]" << std::endl;
			out_times = in_date + "_08:00-" + in_date + "_20:00";
			return 0;
		}
		if(items[3] == "NULL") {
			out_times = in_date + "_08:00-" + in_date + "_20:00";
			return 0;
		}
		//std::cout << items[0] << "-" << items[1] << "-" << items[2] << "-" << items[3] << std::endl;
		Rule_node node;
		Date_rule(items[0], node);
		Week_rule(items[1], node);
		Time_rule(items[2], node);
		Type_rule(items[3], node);
		Rule.push_back(node);
	}
	/*std::cout << "**********************************************" << std::endl;
	for(int i = 0; i < Rule.size(); i ++) {
		for(int j = 0; j < Rule[i].time_date.size(); j ++) {
			std::cout << Rule[i].time_date[j].s_date << "-" << Rule[i].time_date[j].e_date << " ";
		}
		for(int j = 0; j < Rule[i].time_week.size(); j ++) {
			std::cout << Rule[i].time_week[j].s_week << "-" << Rule[i].time_week[j].e_week << " ";
		}
		for(int j = 0; j < Rule[i].time_range.size(); j ++) {
			std::cout << Rule[i].time_range[j] << "-";
		}
		std::cout << std::endl;
	}*/
	for(int i = 0; i < Rule.size(); i ++) {
		Rule_node &r_rule = Rule[i];
		for(int j = 0; j < r_rule.time_date.size(); j ++) {
			if(Check_date(r_rule.time_date[j], in_date)) {
				//std::cout << "AAA" << std::endl;
				for(int k = 0; k < r_rule.time_week.size(); k ++) {
					if(Check_week(r_rule.time_week[k], in_date)) {
						//std::cout << "AAA" << std::endl;
						getOpenTimeRange(r_rule.time_range, in_date, out_times);
						return r_rule.type;
					}
				}
			}
		}
	}
	return 0;
}

void TimeIR::getOpenTimeRange(const std::vector<std::string> &time_range, const std::string &in_date, std::string &out_times) {

	if (time_range.empty() || time_range[0] == "") {
		out_times = "";
		return;
	}
	int Max = time_range.size() - 1;
	
	std::size_t index = time_range[0].find("-");
	std::string s_time = time_range[0].substr(0,index);
	if(s_time.length() < 5) {
		s_time = "0" + s_time;
	}
	index = time_range[Max].find("-");
	std::string e_time = time_range[Max].substr(index + 1);
	if(e_time.length() < 5) {
		e_time = "0" + e_time;
	}
	//std::cout << "in_date = " << in_date << std::endl;
	out_times = in_date + "_"+ s_time + "-" + in_date + "_" + e_time;
}

bool TimeIR::Check_week(const Week_range &week_range, const std::string in_str) {
	std::string week = getDayOfWeek(in_str);
	int num_week = (week[0] - '0');
	//std::cout << "week = " << num_week << std::endl;
	//std::cout << "num_week = " << num_week << std::endl;
	if(week_range.s_week <= week_range.e_week) {
		if(week_range.s_week <= num_week && num_week <= week_range.e_week) {
			return true;
		}
	}
	else {
		if(week_range.s_week <= num_week || num_week <= week_range.e_week) {
			return true;
		}
	}
	return false;
}

bool TimeIR::Check_date(const Date_range &date_range, const std::string in_str) {
	int str_time = 0;
	for(int i = 4; i < 8; i ++) {
		str_time *= 10;
		str_time += (in_str[i] - '0');
	}
	if(date_range.s_date <= date_range.e_date) {
		if(str_time >= date_range.s_date && str_time <= date_range.e_date) {
			return true;
		}
	}
	else {
		if(str_time >= date_range.s_date || str_time <= date_range.e_date) {
			return true;
		}
	}
	return false;
}

void TimeIR::Date_rule(const std::string &in_str, Rule_node &rule) {
	if(in_str == "*") {
		Date_range tmp;
		tmp.s_date = 101;
		tmp.e_date = 1231;
		rule.time_date.push_back(tmp);
		return;
	}
	std::vector<std::string> date;
	split(in_str, ",", date);
	Date_range tmp;
	for(int i = 0; i < date.size(); i ++) {
		std::string &str = date[i];
		int date_tmp = 0;
		if(getData(str, "月", true, date_tmp)) {
			tmp.s_date = date_tmp * 100;
		}
		else {
//			std::cout << "rule is error" << std::endl;
		}
		if(getData(str, "日", true, date_tmp)) {
			tmp.s_date += date_tmp;
		}
		else {
			tmp.s_date += 1;
		}

		if(getData(str, "月", false, date_tmp)) {
			tmp.e_date = date_tmp * 100;
		}
		else {
//			std::cout << "rule is error" << std::endl;
		}
		if(getData(str, "日", false, date_tmp)) {
			tmp.e_date += date_tmp;
		}
		else {
			tmp.e_date += 31;
		}
		rule.time_date.push_back(tmp);
	}
}

void TimeIR::Week_rule(const std::string &in_str, Rule_node &rule) {
	if(in_str == "*") {
		Week_range tmp;
		tmp.s_week = 1;
		tmp.e_week = 7;
		rule.time_week.push_back(tmp);
		return;
	}
	std::vector<std::string> date;
	split(in_str, ",", date);
	Week_range tmp;
	for(int i = 0; i < date.size(); i ++) {
		std::string &str = date[i];
		int date_tmp = 0;
		if(getWeek(str, "周", true, date_tmp)) {
			tmp.s_week = date_tmp;
		}
		else {
//			std::cout << "rule is error" << std::endl;
		}

		if(getWeek(str, "周", false, date_tmp)) {
			tmp.e_week = date_tmp;
		}
		else {
//			std::cout << "rule is error" << std::endl;
		}
		rule.time_week.push_back(tmp);
	}
}

void TimeIR::Time_rule(const std::string &in_str, Rule_node &rule) {
	if(in_str == "*") {
		std::string tmp = "00:00-24:00";
		rule.time_range.push_back(tmp);
		return;
	}
	if(in_str == "全天") {
		std::string tmp = "00:00-24:00";
		rule.time_range.push_back(tmp);
		return;
	}
	if(in_str == "不开门") {
		std::string tmp = "";
		rule.time_range.push_back(tmp);
		return;
	}
	std::vector<std::string> date;
	split(in_str, ",", date);
	//sort(date.begin(),date.end());
	for(int i = 0; i < date.size(); i ++) {
		rule.time_range.push_back(date[i]);
	}
}

void TimeIR::Type_rule(const std::string &in_str, Rule_node &rule) {
	if(in_str == "SURE") {
		rule.type = 1;
	}
	else if(in_str == "NOT_SURE") {
		rule.type = 2;
	}
}

void TimeIR::getVecStrBetween(std::string in_str,
								const std::string &left,
								const std::string &right,
								std::vector<std::string> &out_vecstr) {
	std::size_t left_index;
	std::size_t right_index;
	while(1) {
		left_index = in_str.find(left);
		right_index = in_str.find(right);
		out_vecstr.push_back(in_str.substr(left_index + left.length(), right_index - right.length()));
		if(right_index == in_str.length() - 1)
			break;
		in_str = in_str.substr(right_index + right.length());
	}
}

bool TimeIR::getWeek(const std::string &in_str, const std::string &key, bool order, int &date) {
	date = 0;
	if(order) {
		int index = in_str.find(key);
		if(index != std::string::npos) {
			for(int i = index + key.length(); i < in_str.length(); i ++) {
				if(in_str[i] >= '0' && in_str[i] <= '9') {
					date += (in_str[i] - '0');
				}
				else {
					break;
				}
			}
		}
		else {
			return false;
		}
	}
	else {
		int index = in_str.rfind(key);
		if(index != std::string::npos) {
			for(int i = index + key.length(); i < in_str.length(); i ++) {
				if(in_str[i] >= '0' && in_str[i] <= '9') {
					date += (in_str[i] - '0');
				}
				else {
					break;
				}
			}
		}
		else {
			return false;
		}
	}
	return true;
}

bool TimeIR::getData(const std::string &in_str, const std::string &key, bool order, int &date) {
	date = 0;
	if(order) {
		int index = in_str.find(key);
		int t = 1;
		if(index != std::string::npos) {
			for(int i = index - 1; i >= 0; i --) {
				if(in_str[i] >= '0' && in_str[i] <= '9') {
					date += (in_str[i] - '0') * t;
					t *= 10;
				}
				else {
					break;
				}
			}
		}
		else {
			return false;
		}
	}
	else {
		int index = in_str.rfind(key);
		int t = 1;
		if(index != std::string::npos) {
			for(int i = index - 1; i >= 0; i --) {
				if(in_str[i] >= '0' && in_str[i] <= '9') {
					date += (in_str[i] - '0') * t;
					t *= 10;
				}
				else {
					break;
				}
			}
		}
		else {
			return false;
		}
	}
	return true;
}


char str[12][12] = {"20150103","20150228","20150311","20150411","20150511","20150611",
					"20150711","20150811","20150911","20151011","20151225","20151224"};



int main() {
//	std::string in_rule = "<9月20日-2月19日><周6-周3><9:30-16:00><SURE>|<2月-4月,9月20日-12月19日><周7><10:30-16:00><SURE>|<5月1日-9月19日><周1,周7><9:30-18:00><SURE>|<5月1日-9月19日><周2-周5><9:30-20:00><SURE>";
//	std::string in_rule = "<12月25日><*><不开门><SURE>|<1月1日><*><12:00-18:00><SURE>|<11月1日-3月15日><*><10:00-18:00><SURE>|<3月16日-10月31日><*><10:00-19:00><SURE>";
	std::string in_rule = "<12月25日><*><不开门><SURE>|<1月1日><*><12:00-18:00><SURE>|<11月1-3月15日><*><10:00-18:00><SURE>|<3月16-10月31日><*><10:00-19:00><SURE>";
	for(int i = 0; i < 12; i ++) {
		std::string in_date = str[i];
		std::cout << in_date << std::endl;
		std::string out_times = "";
		std::string week = TimeIR::getDayOfWeek(in_date);
			//int num_week = (week[0] - '0');
		std::cout << "week = " << week << std::endl;
		TimeIR::search(in_rule, in_date, out_times);
		std::cout <<  "out_times = " << out_times << std::endl;
	}
	return 0;
}

/*
int main() {
	std::ifstream fin;
	fin.open("new2.txt");
	std::string line = "";
	std::vector<std::string> items;
	int cnt = 1;
	while(!fin.eof()) {
		getline(fin,line);
		TimeIR::split(line, "	", items);
		std::string in_rule = items[3];
		std::cout << "cnt = " << cnt << std::endl;
		for(int i = 0; i < 12; i ++) {
			std::string in_date = str[i];
			std::string out_times = "";
			std::string week = TimeIR::getDayOfWeek(in_date);
			//int num_week = (week[0] - '0');
			std::cout << "week = " << week << std::endl;
			TimeIR::search(in_rule, in_date, out_times);
			std::cout <<  "out_times = " << out_times << std::endl;
		}
		cnt ++;
	}
	return 0;
}*/
