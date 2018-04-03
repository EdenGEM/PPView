//这个头文件里面包含的所谓通用类,不用有通用性,不会放在对外开放的公共界面内
#ifndef __COMMONFUC_FUNC_H__
#define __COMMONFUC_FUNC_H__

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
#include <math.h>

#define MAXLOGLEN 2048000

//汇率类，用来转换单位
class MYExchange {
private:
public:
	static std::map<std::string, double> curUnitMap;
	static std::tr1::unordered_map<std::string, std::string> curCoiniSignMap;
	static bool loadExchange(MYSQL& _mysql);
	//货币转换，由src转为tgt
	static double curConv(double num, const std::string& src, const std::string& tgt = "CNY");
	static std::string GetCoinSign(const std::string& curCode);
};

int getPrice(const std::string& date, const std::string& price_rule, double& cost);

std::string NormSeconds(int dur);
std::string NormSecondsEn(int dur);
int NormMinute(int dur);
int CheckTimeFormat(const std::string& timeStr);

bool startswith(const std::string& ori_str, const std::string& pat_str);
std::string rmHeadTail(const std::string& str,const std::string& head,const std::string& tail);//字符串删除前面的和后面的内容
void removeCharsFirstANDLast(std::string& strIn, const std::string& firstChars,const std::string& lastChars);
//去掉变量末尾的序号
std::string removeVarIndex(const std::string& str);
//将字符串切割为数组
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

int CountBit(unsigned int n);

std::vector<std::string> toCharactor(const std::string& in);
std::vector<std::string> extractAlphaNumberString(const std::string& in);
std::string extractChars4GBK(const char *src,const std::string& gap);
//删除全角GBK编码中的标点符号
std::string removePunctuation4GBK_Full(const std::string& str);

//字符串正向查找算法SUNDAY
int find_sunday(const char* t,const char* p,const int& t_size,const int& p_size);
//字符串反向查找算法SUNDAY
int rfind_sunday(const char* t,const char* p,const int& t_size,const int& p_size);

// 将add_list中非ori_list的内容合并至ori_list
int MergeListUniq(const std::vector<std::string>& add_list, std::vector<std::string>& ori_list);

#endif //__COMMONFUC_FUNC_H__


