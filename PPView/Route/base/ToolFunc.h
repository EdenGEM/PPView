#ifndef __TOOLFUNC_H__
#define __TOOLFUNC_H__

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fstream>
#include <sys/time.h>
#include <stdarg.h>
#include <tr1/unordered_set>
#include <tr1/unordered_map>
#include <mysql/mysql.h>
#include <sys/types.h>
#include <regex.h>
#include <math.h>

namespace ToolFunc
{
/*	汇率与价格*/

//汇率类，用来转换单位
class RateExchange {
private:
public:
	static std::map<std::string, double> curUnitMap;
	static std::tr1::unordered_map<std::string, std::string> curCoiniSignMap;
	static bool loadExchange(MYSQL& _mysql);
	//货币转换，由src转为tgt
	static double curConv(double num, const std::string& src, const std::string& tgt = "CNY");
	static std::string GetCoinSign(const std::string& curCode);
};

//获取价格，与汇率相关
int getPrice(const std::string& date, const std::string& price_rule, double& cost);


/*	字符串处理*/

//裁剪
std::string& ltrim(std::string& in);
std::string& rtrim(std::string& in);

//将输入合并成字符串
std::string join2String(const std::vector<std::string>& input, const std::string& sep);
std::string join2String(const std::tr1::unordered_set<std::string>& input, const std::string& sep);
std::string joinName(std::vector<std::string>& inList);
std::string joinNameEn(std::vector<std::string>& inList);
template<typename STRING_MAP>
std::string join2String(const STRING_MAP& input, const std::string& sep) {
	std::string res("");
	for (typename STRING_MAP::const_iterator it = input.begin(); it != input.end(); ++it) {
		if (it != input.begin()) res += sep;
		res += (it->first);
	}
	return res;
}

//将字符串切割为数组
int sepString(const std::string& input, const std::string& sep, std::vector<std::string>& output);
int sepString(const std::string& input, const std::string& sep, std::vector<std::string>& output, const std::string& blk);
std::vector<std::string> sepString(const std::string& input, const std::string& sep);
int sepString(const std::string& input, const std::string& sep, std::tr1::unordered_set<std::string>& output);

//删除字符串前面的和后面的内容
std::string rmHeadTail(const std::string& str,const std::string& head,const std::string& tail);
void removeCharsFirstANDLast(std::string& strIn, const std::string& firstChars,const std::string& lastChars);

//替换字符串
bool replaceString(std::string& ori_str,const std::string& rep_from,const std::string& rep_to);


/*	vector处理相关*/

//从排序好的vector中查找是否包含一个元素
bool isfind_vec(const int& val,const std::vector<int>& vec);

// 将add_list中非ori_list的内容合并至ori_list
int MergeListUniq(const std::vector<std::string>& add_list, std::vector<std::string>& ori_list);

// 保序下对list去重
int UniqListOrder(std::vector<std::string>& in_list);


/*	时间信息判别与检查*/

//时间格式检查类，用来检查时间格式是否正确
class FormatChecker {
public:
	static int Init();
	static int CheckDate(const std::string& dateStr);
	static int CheckTime(const std::string& timeStr);
	static int CheckHM(const std::string& HMStr);
	static int CheckCoord(const std::string& coordStr);
private:
	static regex_t m_timePat;
	static regex_t m_HMPat;
	static regex_t m_datePat;
	static regex_t m_coordPat;
};

//判断两个两个时间段是否有重叠
bool HasUnionPeriod(time_t aTimeS, time_t aTimeE, time_t bTimeS, time_t bTimeE);

//根据开关门规则获取开关门时间
int getOpenCloseTime(const std::string& date, const std::string& time_rule, double time_zone, time_t& open_time, time_t& close_time);

//将dur 表示的秒数转化为小时分钟
std::string NormSeconds(int dur);
std::string NormSecondsEn(int dur);
int NormMinute(int dur);

//将timeOffset表示的s数转化为小时分钟字符串，如4500s-->01:15
int toStrTime(int timeOffset, std::string& strTime);

//将一个以冒号分隔时分的字符串转为整型时间偏移
int toIntOffset(const std::string& strTime, int& offset);

std::string itoa(int a);
/*	其他*/
int CountBit(unsigned int n);

};//namespace SOGOUCHAT
#endif /*__TOOLFUNC_H__*/
