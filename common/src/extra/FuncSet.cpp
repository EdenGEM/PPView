#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include <iconv.h>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "FuncSet.h"
#include "../MJLog.h"

using namespace std;
using namespace MJ;

const boost::regex kTimePat("^[12]\\d{3}[01]\\d[0123]\\d_[012]\\d:[0-5]\\d$");

bool MYExchange::loadExchange(MYSQL& _mysql) {
	PrintInfo::PrintLog("MYExchange::loadExchange, loading Exchange Data...");
	std::string sql = "select currency_code,rate,coin_sign from exchange";
    int t = mysql_query(&_mysql, sql.c_str());
    if (t != 0) {
		PrintInfo::PrintErr("MYExchange::loadExchange, mysql_query error %s error sql: %s", mysql_error(&_mysql), sql.c_str());
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
			PrintInfo::PrintLog("MYExchange::loadExchange, Load Exchange Num: %d", num);
        }
        mysql_free_result(res);
    }
	return true;
}

double MYExchange::curConv(double num, const std::string &src, const std::string &tgt) {
	std::string srcUnit(src);
	std::string tgtUnit(tgt);
	if (tgt == "CNY") {  //外币转人民币
		std::map<std::string, double>::iterator it = curUnitMap.find(srcUnit);
		if (it != curUnitMap.end()) {
			return num * it->second;
		} else {
			PrintInfo::PrintErr("MYExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
			return 0.0;
		}
	} else if (src == "CNY") {
		std::map<std::string, double>::iterator it = curUnitMap.find(tgtUnit);
		if (it != curUnitMap.end()) {
			return num / it->second;
		} else {
			PrintInfo::PrintErr("MYExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
			return 0.0;
		}
	} else {
		PrintInfo::PrintErr("MYExchange::curConv, unrecognize currency unit, %s-->%s", srcUnit.c_str(), tgtUnit.c_str());
		return 0.0;
	}
}

std::string MYExchange::GetCoinSign(const std::string& curCode) {
	std::tr1::unordered_map<std::string, std::string>::iterator it = curCoiniSignMap.find(curCode);
	if (it != curCoiniSignMap.end()) {
		return it->second;
	}
	return "";
}

std::vector<std::string> toCharactor(const std::string& in){
	std::vector<std::string> v;
	for (size_t i=0;i<in.length();){
		if ((in[i] & 0x80) == 0x00){
			v.push_back(in.substr(i++,1));
		} else if ((in[i] & 0xE0) == 0xC0){
			v.push_back(in.substr(i,2));
			i+=2;
		} else if ((in[i] & 0xF0) == 0xE0){
			v.push_back(in.substr(i,3));
			i+=3;
		} else {
			v.clear();
			return v;
		}
	}
	return v;
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
		snprintf(buff, sizeof(buff), "%s and %s", boost::join(frontList, ", ").c_str(), inList.back().c_str());
	}
	return std::string(buff);
}

void removeCharsFirstANDLast(std::string& strIn, const std::string& firstChars,const std::string& lastChars)
{
	if (strIn.length()==0)
		return;
	size_t begPos = strIn.find_first_not_of(firstChars);
	size_t endPos = strIn.find_last_not_of(lastChars);
	if (begPos == std::string::npos || endPos == std::string::npos){
		strIn = "";
	} else {
		strIn = strIn.substr(begPos,endPos-begPos+1);
	}
	return;
}

std::vector<std::string> extractAlphaNumberString(const std::string& in){
	std::vector<std::string> res;
	std::string curStr = "";
	for (size_t i=0;i<in.length();i++){
		bool isfind = false;
		while (i<in.length() && (('A' <= in[i] && in[i] <= 'Z') || ('a' <= in[i] && in[i] <= 'z') || ('0' <= in[i] && in[i] <= '9')
			|| in[i] == '#' || in[i] == '*')){
				isfind = true;
				curStr += in[i++];
		}
		if (isfind){
			res.push_back(curStr);
			curStr = "";
		}
	}
	return res;
}

std::string removeVarIndex(const std::string& str){
	std::string res;
	int pos = 3;
	int lastPos = 0;
	while((pos = str.find("]",pos))!=std::string::npos){
		if (str[pos-1]>='0' && str[pos-1]<='9'){
			res += str.substr(lastPos,pos-1-lastPos);
			res += "]";
		} else {
			res += str.substr(lastPos,pos+1-lastPos);
		}
		pos++;
		lastPos = pos;
	}
	if (lastPos<str.length()){
		res += str.substr(lastPos);
	}
	return res;
}

//字符串查找算法SUNDAY
size_t SHIFT_ARRAY[256];
int find_sunday(const char* t,const char* p,const int& t_size,const int& p_size){
	int i;
	for ( i=0; i < 256; i++ )
		SHIFT_ARRAY[i] = p_size+1;
	for ( i=0; i < p_size; i++ )
		SHIFT_ARRAY[(unsigned char)(*(p+i))] = p_size-i;
	int  limit = t_size-p_size+1;
	for ( i=0; i < limit; i += SHIFT_ARRAY[(unsigned char)t[i+p_size]]){
		if ( t[i] == *p )
        {
            const char *match_text = t+i+1;
            int  match_size = 1;
            do{
                if ( match_size == p_size )
                	return i;
            }while( (*match_text++) == p[match_size++] );
        }
    }
    return std::string::npos;
}

size_t SHIFT_ARRAY_R[256];
int rfind_sunday(const char* t,const char* p,const int& t_size,const int& p_size){
	int i;
	for ( i=0; i < 256; i++ )
		SHIFT_ARRAY_R[i] = p_size+1;
	for ( i=0; i < p_size; i++ )
		SHIFT_ARRAY_R[(unsigned char)(*(p+i))] = i+1;
	int  limit = p_size-2;
	for ( i=t_size-1; i > limit; i -= SHIFT_ARRAY_R[(unsigned char)(t[i-p_size])]){
		if ( t[i] == *(p+p_size-1) )
        {
            const char *match_text = t+i-1;
            int  match_size = 1;
            do{
                if ( match_size == p_size )
                	return i-p_size+1;
            }while( (*match_text--) == p[p_size-1-match_size++] );
        }
    }
    return std::string::npos;
}

std::string removePunctuation4GBK_Full(const std::string& str){
	unsigned char c1,c2;
	std::string ret="";
	for (int i=0;i<str.length();i+=2){
		c1 = str[i];
		c2 = str[i+1];

		if (c1>=0xA1 && c1<=0xA2 && c2>=0xA1 && c2<=0xFE){
			if (c1==0xA1 && c2==0xA1){
				ret += c1;
				ret += c2;
			}
			continue;
		} else if (c1==0xA3 && ((c2>=0xA1 && c2<=0xAF)||(c2>=0xBA && c2<=0xC0)||(c2>=0xDB && c2<=0xE0)||c2>=0xFB)){
			continue;
		}
		ret += c1;
		ret += c2;
	}
	return ret;
}

void SBC2DBC_UNICODE(const char* from,char* to,int& from_len,int& to_len){
	int i;
	for (i=0;i<from_len-1;i+=2){
		unsigned char cur_q = (unsigned char)from[i];
		unsigned char cur_h = (unsigned char)from[i+1];
		if (cur_q==0xFF && cur_h<=0x5E){
			unsigned int val = cur_q*256 + cur_h;
			val -= 65248;
			to[i] = (char)(val>>8);
			to[i+1] = (char)(val&0x00FF);
		} else if (cur_q==0x30 && cur_h==0x00){
			to[i] = 0;
			to[i] = 32;
		} else if (cur_q==0x00 && cur_h==0x00){
			break;
		} else {
			to[i] = from[i];
			to[i+1] = from[i+1];
		}
	}
	from_len = i;
	to_len = i;
	to[i]='\0';
	to[i+1]='\0';
	return;
}


//字符串删除前面的和后面的内容
std::string rmHeadTail(const std::string& str,const std::string& head,const std::string& tail){
	std::string ret="";
	size_t h = str.find(head);
	if (h!=std::string::npos){
		h += head.length();
		size_t t = str.find(tail,h);
		if (t != std::string::npos){
			ret = str.substr(h,t-h);
		}
	}
	return ret;
}

// 输入
// price_rule: 成人:*-人民币
int getPrice(const std::string& date, const std::string& price_rule, double& cost) {
	cost = 0;
	if (price_rule == "") return 1;
	std::string::size_type type_pos = price_rule.find(":");
	std::string::size_type currency_pos = price_rule.rfind("-");
	if (type_pos == std::string::npos || currency_pos == std::string::npos) return 1;
	double num = atof(price_rule.substr(type_pos + 1, currency_pos - type_pos - 1).c_str());
	std::string cur_unit = price_rule.substr(currency_pos + 1);
	cost = MYExchange::curConv(num, cur_unit);
	return 0;
}
//
//// 将 秒 转成 分钟/小时
//std::string NormSeconds(int dur) {
//	char buff[100];
//	if (dur < 60) {
//		snprintf(buff, sizeof(buff), "1分钟");
//	} else if (dur < 3600) {
//		snprintf(buff, sizeof(buff), "%d分钟", dur / 60);
//	} else {
//		snprintf(buff, sizeof(buff), "%.1f小时", dur / 3600.0);
//	}
//	return buff;
//}

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

int MergeListUniq(const std::vector<std::string>& add_list, std::vector<std::string>& ori_list) {
	ori_list.insert(ori_list.end(), add_list.begin(), add_list.end());
	UniqListOrder(ori_list);
	return 0;
}
bool startswith(const std::string& ori_str, const std::string& pat_str) {
	if (ori_str.size() >= pat_str.size() && ori_str.substr(0, pat_str.size()) == pat_str) {
		return true;
	} else {
		return false;
	}
}

// 检查时间格式 20151104_22:20
int CheckTimeFormat(const std::string& timeStr) {
	bool match = boost::regex_match(timeStr, kTimePat);
	if (match) return 0;
	return 1;
}

int CountBit(unsigned int n) {
	int cnt = 0;
	while (n) {
		++cnt;
		n &= (n - 1);
	}
	return cnt;
}
