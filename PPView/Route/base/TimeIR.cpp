#include "TimeIR.h"

//检查景点open字段格式
//<1月-5月,10月-12月><周2-周6><10:00-12:30,14:00-18:30><SURE>|<1月1日,5月1日,12月25日><*><不开门><SURE>
bool TimeIR::CheckTimeRule(const std::string &rule) {
	int idx = rule.rfind(">");
	const std::string rule_t = rule.substr(0,idx+1);
	std::vector<std::string> rule_items;
	split(rule_t, "|", rule_items);
	if (rule_items.size() <= 0) {
		return false;
	}
	for (int i = 0; i < rule_items.size(); i ++) {
		std::vector<std::string> items;
		getVecStrBetween(rule_items[i], "<", ">", items);
		if (4 != items.size()) {
			return false;
		}
	}
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
        if (find_pos == std::string::npos)
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
	int left_index;
	if (is_left_forward)
	{
		left_index = in_str.find(left);
	}
	else
	{
		left_index = in_str.rfind(left);
	}

	int right_index;
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

//获取in_date的开关门范围 可能为多段
/********************************************//**
 * \brief 根据开关门规则，获取in_date的开关门时间范围
 *
 * \param rule 开关门规则 格式 <1月1日-5月1日,10月-12月><周2-周6><10:00-12:30,14:00-18:30><SURE>
 * \param in_date 日期 格式 20180101
 * \param out_timesList 输出当天的开关门时间段 
 * 			如果当前日期开门 ["20180101_10:00-20180101_12:30","20180101_14:00-20180101_18:30"]
 * 			不开门 []
 ***********************************************/
bool TimeIR::getTheDateOpenTimeRange(const std::string &rule, const std::string &in_date, std::vector<std::string> &out_timesList) {
	int idx = rule.rfind(">");
	const std::string rule_t = rule.substr(0,idx+1);
	std::string out_times = "";
	std::vector<std::string> rule_items;
	split(rule_t, "|", rule_items);
	std::vector<Rule_node> Rule;
	for(int i = 0; i < rule_items.size(); i ++) {
		//std::cout << "rule_items[i] = " << rule_items[i] << std::endl;
		std::vector<std::string> items;
		getVecStrBetween(rule_items[i], "<", ">", items);
		if(4 != items.size()) {
			out_times = in_date + "_08:00-" + in_date + "_20:00";
			out_timesList.push_back(out_times);
			return 0;
		}
		if(items[3] == "NULL") {
			out_times = in_date + "_08:00-" + in_date + "_20:00";
			out_timesList.push_back(out_times);
			return 0;
		}
		Rule_node node;
		Date_rule(items[0], node);
		Week_rule(items[1], node);
		Time_rule(items[2], node);
		Type_rule(items[3], node);

		//std::cout << "node show ..." << std::endl << node.show() << std::endl;
		Rule.push_back(node);
	}
	for(int i = 0; i < Rule.size(); i ++) {
		Rule_node &r_rule = Rule[i];
		for(int j = 0; j < r_rule.date_range.size(); j ++) {
			if(Check_date(r_rule.date_range[j], in_date)) {
				for(int k = 0; k < r_rule.week_range.size(); k ++) {
					if(Check_week(r_rule.week_range[k], in_date)) {
						getOpenTimeRange(r_rule.time_range, in_date, out_timesList);
					}
				}
			}
		}
	}
	return 0;
}


void TimeIR::getOpenTimeRange(const std::vector<std::string> &time_range, const std::string &in_date, std::vector<std::string> &out_times) {
	if (time_range.empty() || time_range[0] == "") {
		return;
	}

	for(int i = 0; i < time_range.size(); i++) {
		int index = time_range[i].find("-");
		std::string s_time = time_range[i].substr(0,index);
		if (s_time.length() < 5) {
			s_time = "0" + s_time;
		}
		std::string e_time = time_range[i].substr(index + 1);
		if (e_time.length() < 5) {
			e_time = "0" + e_time;
		}
		std::string out_time = in_date + "_"+ s_time + "-" + in_date + "_" + e_time;
		out_times.push_back(out_time);
	}
	std::sort(out_times.begin(), out_times.end());
}

bool TimeIR::Check_week(const Week_range &week_range, const std::string in_str) {
	int num_week = getWeekByDate(in_str);
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
		rule.date_range.push_back(tmp);
		return;
	}
	std::vector<std::string> date;
	split(in_str, ",", date);
	Date_range tmp;
	for(int i = 0; i < date.size(); i ++) {
		const std::string &str = date[i];
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
		rule.date_range.push_back(tmp);
	}
}

void TimeIR::Week_rule(const std::string &in_str, Rule_node &rule) {
	if(in_str == "*") {
		Week_range tmp;
		tmp.s_week = 1;
		tmp.e_week = 7;
		rule.week_range.push_back(tmp);
		return;
	}
	std::vector<std::string> date;
	split(in_str, ",", date);
	for(int i = 0; i < date.size(); i ++) {
		Week_range tmp;
		std::string &str = date[i];
		std::string zhou = "周";
		int date_tmp_s = str.find(zhou);
		int date_tmp_e = str.rfind(zhou);
		if (date_tmp_s != std::string::npos and date_tmp_s+zhou.length()+1 <= str.length()) {
			std::string weekS = str.substr(date_tmp_s+zhou.length(),1);
			tmp.s_week = string2int(weekS);
		} else {
			continue;
		}
		if (date_tmp_e != std::string::npos and date_tmp_e+zhou.length()+1 <= str.length()) {
			std::string weekE = str.substr(date_tmp_e+zhou.length(),1);
			tmp.e_week = string2int(weekE);
		} else {
			continue;
		}

		if (tmp.s_week > tmp.e_week
				or tmp.s_week > 7 or tmp.s_week < 1
				or tmp.e_week > 7 or tmp.e_week < 1)
			continue;
		rule.week_range.push_back(tmp);
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
	int left_index;
	int right_index;
	while(1) {
		left_index = in_str.find(left);
		right_index = in_str.find(right);
		out_vecstr.push_back(in_str.substr(left_index + left.length(), right_index - right.length()));
		if(right_index == in_str.length() - 1)
			break;
		in_str = in_str.substr(right_index + right.length());
	}
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

//当前日期是否可用
/********************************************//**
 * \brief 处理DAY_TIME_RULE; 检索输入日期是否满足给定的open_rule (满足日期范围及周几). 如果满足，则返回true.
 *
 * \param open_rule json数组(格式同数据库中玩乐相关表open字段)
 *				格式为:门票适用的日期 精确到天和周
 *    				   from: string 起始日期
 *         			   to: string 截止日期
 *             		   week: int数组 可用的星期 [1-7]
 * \param in_date 字符串, 格式为:20140915
 *
 ***********************************************/

bool TimeIR::isTheDateAvailable(const Json::Value& open_rule, const std::string& date) {
	if (!open_rule.isArray() or open_rule.size() == 0) {
		return false;
	}
	for (int i = 0; i < open_rule.size(); i++) {
		const Json::Value& jOpen = open_rule[i];
		std::string from = "", to = "";
		if (jOpen.isMember("from") && jOpen["from"].isString()
				&& jOpen.isMember("to") && jOpen["to"].isString()
				&& jOpen.isMember("week") && jOpen["week"].isArray()) {
			from = jOpen["from"].asString();
			to = jOpen["to"].asString();
			if (from <= date && date <= to) {
				int wd = getWeekByDate(date);
				auto it = std::find(jOpen["week"].begin(), jOpen["week"].end(), wd);
				if (it != jOpen["week"].end()) {
					return true;
				}
			}
		}
	}
	return false;
}
int TimeIR::getWeekByDate(const std::string& in_date) {
	time_t tt = MJ::MyTime::toTime(in_date,0);
	struct tm TM;
	gmtime_r(&tt, &TM);
	int ret = (TM.tm_wday + 6) % 7 + 1;
	return ret;
}

char str[12][12] = {"20150103","20150228","20150311","20150411","20150511","20150611",
					"20150711","20150811","20150911","20151011","20151225","20151224"};



int main() {
	std::string in_rule = "<12月25日><*><不开门><SURE>|<1月1日><*><12:00-18:00><SURE>|<11月1-3月15日><*><10:00-18:00><SURE>|<3月16-10月31日><*><10:00-19:00><SURE>";
	for(int i = 0; i < 12; i ++) {
		std::string in_date = str[i];
		std::cout << in_date << std::endl;
		std::vector<std::string> out_times;
		int week = TimeIR::getWeekByDate(in_date);
		std::cout << "week = " << week << std::endl;
		//TimeIR::getTheDateOpenTimeRange(in_rule, in_date, out_times);
		//for (auto it = out_times.begin(); it != out_times.end(); it++) {
		//	std::cout <<  "out_times = " << *it << std::endl;
		//}
	}
	return 0;
}
