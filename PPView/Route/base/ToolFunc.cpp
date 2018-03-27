#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include <iconv.h>
#include <algorithm>
#include "ToolFunc.h"
#include "MJCommon.h"

using namespace std;

namespace ToolFunc
{
	
std::map<std::string, double> RateExchange::curUnitMap;
std::tr1::unordered_map<std::string, std::string> RateExchange::curCoiniSignMap;

regex_t FormatChecker::m_timePat;
regex_t FormatChecker::m_HMPat;
regex_t FormatChecker::m_datePat;
regex_t FormatChecker::m_coordPat;

/*	RateExchange相关*/
bool RateExchange::loadExchange(MYSQL& _mysql) {
	MJ::PrintInfo::PrintLog("RateExchange::loadExchange, loading Exchange Data...");
	std::string sql = "select currency_code,rate,coin_sign from exchange";
    int t = mysql_query(&_mysql, sql.c_str());
    if (t != 0) {
		MJ::PrintInfo::PrintErr("RateExchange::loadExchange, mysql_query error %s error sql: %s", mysql_error(&_mysql), sql.c_str());
        mysql_close(&_mysql);
        return false;
    } else {
        MYSQL_RES* res = mysql_use_result(&_mysql);
        MYSQL_ROW row;
        if (res) {
        	int num = 0;
            while (row = mysql_fetch_row(res)) {
                if (row[0] == NULL)
                    continue;
				if (row[0] == "" || row[0] == "null") continue;
				std::string currency_code = row[0];
				double rate = row[1] ? atof(row[1]) : 1.0;
				curUnitMap[currency_code] = rate;
				if (row[2] != NULL && row[2] != "" && row[2] != "NULL") {
//					std::cerr << currency_code <<" Coin Sign  :" <<row[2] << std::endl;
					curCoiniSignMap[currency_code] = row[2];
				}
				++num;
            }
			MJ::PrintInfo::PrintLog("RateExchange::loadExchange, Load Exchange Num: %d", num);
        }
        mysql_free_result(res);
    }
	return true;
}

double RateExchange::curConv(double num, const std::string &src, const std::string &tgt) {
	std::string srcUnit(src);
	std::string tgtUnit(tgt);
	if (tgt == "CNY") {  //外币转人民币
		std::map<std::string, double>::iterator it = curUnitMap.find(srcUnit);
		if (it != curUnitMap.end()) {
			return num * it->second;
		} else {
			MJ::PrintInfo::PrintErr("RateExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
			return 0.0;
		}
	} else if (src == "CNY") {
		std::map<std::string, double>::iterator it = curUnitMap.find(tgtUnit);
		if (it != curUnitMap.end()) {
			return num / it->second;
		} else {
			MJ::PrintInfo::PrintErr("RateExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
			return 0.0;
		}
	} else {
		MJ::PrintInfo::PrintErr("RateExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
		return 0.0;
	}
}

std::string RateExchange::GetCoinSign(const std::string& curCode) {
	std::tr1::unordered_map<std::string, std::string>::iterator it = curCoiniSignMap.find(curCode);
	if (it != curCoiniSignMap.end()) {
		return it->second;
	}
	return "";
}

// price_rule: 成人:*-人民币
int getPrice(const std::string& date, const std::string& price_rule, double& cost) {
	cost = 0;
	if (price_rule == "") return 1;
	std::string::size_type type_pos = price_rule.find(":");
	std::string::size_type currency_pos = price_rule.rfind("-");
	if (type_pos == std::string::npos || currency_pos == std::string::npos) return 1;
	double num = atof(price_rule.substr(type_pos + 1, currency_pos - type_pos - 1).c_str());
	std::string cur_unit = price_rule.substr(currency_pos + 1);
	cost = RateExchange::curConv(num, cur_unit);
	return 0;
}

/*	字符串处理相关*/
std::string& ltrim(std::string& in) {
	std::string::size_type pos = in.find_first_not_of(" \t\r\n");
	if (pos != std::string::npos) in.erase(0, pos);
	return in;
}

std::string& rtrim(std::string& in) {
	std::string::size_type pos = in.find_last_not_of(" \t\r\n");
	if (pos != std::string::npos) in.erase(pos + 1);
	return in;
}

std::string join2String(const std::vector<std::string>& input, const std::string& sep) {
	std::string res("");
	for (std::vector<std::string>::const_iterator it = input.begin(); it != input.end(); ++it) {
		if (it != input.begin()) res += sep;
		res += *it;
	}
	return res;
}

std::string joinName(std::vector<std::string>& inList) {
	return ToolFunc::join2String(inList, "、").c_str();
}

std::string joinNameEn(std::vector<std::string>& inList) {
	if (inList.empty()) return "";
	if (inList.size() == 1) return inList.front();

	char buff[10000];
	if (inList.size() == 2) {
		snprintf(buff, sizeof(buff), "%s and %s", inList.front().c_str(), inList.back().c_str());
	} else {
		std::vector<std::string> frontList(inList);
		frontList.pop_back();
		snprintf(buff, sizeof(buff), "%s and %s", ToolFunc::join2String(frontList, ", ").c_str(), inList.back().c_str());
	}
	return std::string(buff);
}

std::string join2String(const std::tr1::unordered_set<std::string>& input, const std::string& sep) {
	std::string res("");
	for (std::tr1::unordered_set<std::string>::const_iterator it = input.begin(); it != input.end(); ++it) {
		if (it != input.begin()) res += sep;
		res += *it;
	}
	return res;
}

int sepString(const std::string& input, const std::string& sep, std::vector<std::string>& output) {
	std::string tmp_str = input;
	while (tmp_str.length()) {
		std::string::size_type pos = tmp_str.find(sep);
		if (pos != std::string::npos) {
			std::string tmp_str_sub = tmp_str.substr(0, pos);
			output.push_back(tmp_str_sub);
			tmp_str = tmp_str.substr(pos + sep.length());
			if (tmp_str.length() == 0) {
				output.push_back("");
			}
		} else {
			output.push_back(tmp_str);
			break;
		}
	}
	return 0;
}

int sepString(const std::string& input, const std::string& sep, std::vector<std::string>& output, const std::string& blk) {
	sepString(input, sep, output);
	std::vector<std::string>::iterator it = output.begin();
	while (it != output.end()) {
		if (*it == blk) {
			it = output.erase(it);
		} else {
			++it;
		}
	}
	return 0;
}

std::vector<std::string> sepString(const std::string& input, const std::string& sep) {
	std::vector<std::string> output;
	sepString(input, sep, output);
	return output;
}

int sepString(const std::string& input, const std::string& sep, std::tr1::unordered_set<std::string>& output) {
	std::vector<std::string> tmp_list;
	sepString(input, sep, tmp_list);
	output.clear();
	output.insert(tmp_list.begin(), tmp_list.end());
	return 0;
}

void removeCharsFirstANDLast(std::string& strIn, const std::string& firstChars,const std::string& lastChars)
{
	if (strIn.length()==0)
		return;
	int begPos = strIn.find_first_not_of(firstChars);
	int endPos = strIn.find_last_not_of(lastChars);
	if (begPos == std::string::npos || endPos == std::string::npos){
		strIn = "";
	} else {
		strIn = strIn.substr(begPos,endPos-begPos+1);
	}
	return;
}

bool replaceString(std::string& ori_str,const std::string& rep_from,const std::string& rep_to)
{
	int pos = 0;
	while(true)
	{
		pos = ori_str.find(rep_from,pos);
		if (rep_from.length()==0)
			return false;
		if (pos != std::string::npos)
		{
			ori_str.replace(pos,rep_from.length(),rep_to);
			pos += rep_to.length();
		}
		else
			break;
	}
	return true;
}

/*	vector处理相关*/
//从排序好的vector中查找是否包含一个元素
bool isfind_vec(const int& val,const vector<int>& vec){
	if (val>vec[vec.size()-1]
		||val<vec[0])
		return false;
	int i,b,e;
	b = 0;
	e = vec.size()-1;
	for (i=(b+e)/2;b<=e;i=(b+e)/2){
		if (val==vec[i])
			return true;
		else if (val>vec[i])
			b = i+1;
		else
			e = i-1;
	}
	return false;
}

template<class T>
bool ContainVector(const std::vector<T>& ori,const std::vector<T>& dest){
	int i,j;
	i = j = 0;
	while(i<ori.size() && j<dest.size()){
		if (ori[i]==dest[j]){
			i++;
			j++;
		} else {
			i++;
		}
	}
	if (j<dest.size())
		return false;
	else
		return true;
}

template<class T>
void RemoveVector(std::vector<T>& from,const std::vector<T>& remove){
	int i,j;
	i = j = 0;
	while(i<from.size() && j<remove.size()){
		if (from[i]==remove[j]){
			from.erase(from.begin()+i);
			j++;
		} else if (from[i]<remove[j]){
			i++;
		} else {
			j++;
		}
	}
	return;
}

int MergeListUniq(const std::vector<std::string>& add_list, std::vector<std::string>& ori_list) {
	ori_list.insert(ori_list.end(), add_list.begin(), add_list.end());
	UniqListOrder(ori_list);
	return 0;
}

int UniqListOrder(std::vector<std::string>& in_list) {
	std::tr1::unordered_set<std::string> add_set;
	for (std::vector<std::string>::iterator it = in_list.begin(); it != in_list.end();) {
		if (add_set.find(*it) == add_set.end()) {
			add_set.insert(*it);
			++it;
		} else {
			it = in_list.erase(it);
		}
	}
	return 0;
}

/*	对时间信息判别处理相关*/
// time_rule: 20140827_09:00-20140827_18:00
int getOpenCloseTime(const std::string& date, const std::string& time_rule, double time_zone, time_t& open_time, time_t& close_time) {
	if (time_rule == "") return 1;
	std::vector<std::string> time_items;
	ToolFunc::sepString(time_rule, "-", time_items);
	if (time_items.size() != 2) return 1;
	open_time = MJ::MyTime::toTime(time_items[0], time_zone);
	close_time = MJ::MyTime::toTime(time_items.back(), time_zone);
	return 0;
}

// 将 秒 转成 *小时*分钟
std::string NormSeconds(int dur) {
	int ret_len = 100;
	char ret[ret_len];
	int add_len = 0;

//	int dur_norm = dur + 30;  // 超半分钟算一分钟
	int dur_norm = dur;
	int hour_cnt = dur_norm / 3600;
	dur_norm -= hour_cnt * 3600;
	int minute_cnt = dur_norm / 60;
	dur_norm -= minute_cnt * 60;
	int second_cnt = dur_norm;

	if (hour_cnt > 0) {
		add_len += snprintf(ret + add_len, ret_len - add_len, "%d小时", hour_cnt);
	}
	if (minute_cnt > 0) {
		add_len += snprintf(ret + add_len, ret_len - add_len, "%d分钟", minute_cnt);
	}
	if (hour_cnt <= 0 && minute_cnt <= 0) {
		if (second_cnt > 30) {
			snprintf(ret, ret_len, "1分钟");
		} else {
			snprintf(ret, ret_len, "0分钟");
		}
	}
	return ret;
}

std::string NormSecondsEn(int dur) {
	int ret_len = 100;
	char ret[ret_len];
	int add_len = 0;

//	int dur_norm = dur + 30;  // 超半分钟算一分钟
	int dur_norm = dur;
	int hour_cnt = dur_norm / 3600;
	dur_norm -= hour_cnt * 3600;
	int minute_cnt = dur_norm / 60;
	dur_norm -= minute_cnt * 60;
	int second_cnt = dur_norm;

	if (hour_cnt > 0) {
		add_len += snprintf(ret + add_len, ret_len - add_len, "%dh", hour_cnt);
	}
	if (minute_cnt > 0) {
		add_len += snprintf(ret + add_len, ret_len - add_len, "%dmin", minute_cnt);
	}
	if (hour_cnt <= 0 && minute_cnt <= 0) {
		if (second_cnt > 30) {
			snprintf(ret, ret_len, "1min");
		} else {
			snprintf(ret, ret_len, "0min");
		}
	}
	return ret;
}

int NormMinute(int dur) {
	if (dur > 0 && dur < 60 || dur < 0 && dur > -60) return 0;
	return (dur - dur % 60);
}

//将timeOffset表示的s数转化为小时分钟字符串，如4500s-->01:15
int toStrTime(int timeOffset, std::string& strTime) {
	if (timeOffset < 0) {
		fprintf(stderr,"ToolFunc::toStrTime: timeOffset cann't be negative\n");
		return 1;
	}
	char buf[256];
	int len = 0;
	int hour = timeOffset / 3600;
	int minute = (timeOffset - 3600 * hour) / 60 ;
	int res = 0;
	if (hour >= 10) {
		res = snprintf(buf, sizeof(buf), "%d", hour);
	}
	 else {
		res = snprintf(buf, sizeof(buf), "0%d", hour);
	}
	if (res == -1) {
		fprintf(stderr,"ToolFunc::toStrTime: timeOffset to hour failed\n");
		return 1;
	}
	len += res;
	if (minute >= 10) {
		res = snprintf(buf + len, sizeof(buf) - len, ":%d", minute);
	} else {
		res = snprintf(buf + len, sizeof(buf) - len, ":0%d", minute);
	}
	if (res == -1) {
		fprintf(stderr,"ToolFunc::toStrTime: timeOffset to minute failed\n");
		return 1;
	}
	strTime = buf;
    return 0;
}

//将一个以冒号分隔时分的字符串转为整型时间偏移
int toIntOffset(const std::string& strTime, int& offset) {
	std::string::size_type pos = strTime.find(":");
	if (pos == std::string::npos) {
		fprintf(stderr, "ToolFunc::toIntOffset: time format err, time %s\n",strTime.c_str());
		return 1;
	}
	std::string hour = strTime.substr(0,pos);
	std::string minute = strTime.substr(pos+1);
	offset = atoi(hour.c_str()) * 3600 + atoi(minute.c_str()) * 60;
	return 0;
}

// 检查日期格式 20160711
int FormatChecker::CheckDate(const std::string& dateStr) {
	if (regexec(&m_datePat, dateStr.c_str(), 0, 0, 0) == 0) return 0;
	return 1;
}

// 检查时间格式 20151104_22:20
int FormatChecker::CheckTime(const std::string& timeStr) {
	if (regexec(&m_timePat, timeStr.c_str(), 0, 0, 0) == 0) return 0;
	return 1;
}

// 检查时间格式 23:59
int FormatChecker::CheckHM(const std::string& HMStr) {
	if (regexec(&m_HMPat, HMStr.c_str(), 0, 0, 0) == 0) return 0;
	return 1;
}

int FormatChecker::CheckCoord(const std::string& coordStr) {
	if (regexec(&m_coordPat, coordStr.c_str(), 0, 0, 0) == 0) return 0;
	return 1;
}

int FormatChecker::Init() {
	regcomp(&m_timePat, "^[12][0-9]{3}[01][0-9][0123][0-9]_[012][0-9]:[0-5][0-9]$", REG_EXTENDED);
	regcomp(&m_HMPat, "^[012][0-9]:[0-5][0-9]$", REG_EXTENDED);
	regcomp(&m_datePat, "^[12][0-9]{3}[01][0-9][0123][0-9]$", REG_EXTENDED);
	regcomp(&m_coordPat, "^-?[0-9.]+[ ]*,[ ]*-?[0-9.]+$", REG_EXTENDED);
	return 0;
}

bool HasUnionPeriod(time_t aTimeS, time_t aTimeE, time_t bTimeS, time_t bTimeE) {
	if (aTimeS > bTimeE) return false;
	if (aTimeE < bTimeS) return false;
	return true;
}

/*	其他*/
int CountBit(unsigned int n) {
	int cnt = 0;
	while (n) {
		++cnt;
		n &= (n - 1);
	}
	return cnt;
}

std::string itoa(int a) {
	char s[200] = {0};
	snprintf(s, sizeof(s), "%d", a);
	return std::string(s);
}
}//namespace SOGOUCHAT

