#ifndef __MY_TIME_H__
#define __MY_TIME_H__


#include <string>
#include <iostream>
#include <math.h>
#include <stdlib.h>

namespace MJ{

const int mon_day[]={31,28,31,30,31,30,31,31,30,31,30,31};
//时间转换
//该时间类不应该再额外被使用,故其没有对外声明
class MyTime {
    public:
        //根据当前时间获取指定时区的年月日时间等信息
        //"%Y":年 "%M":月 "%D":日 "%h":小时 "%m":分钟
        static int get(const time_t& t,const std::string& mod,double zone=8){
            struct tm TM;
            time_t nt = t + zone * 3600;
            gmtime_r(&nt,&TM);
            if (mod.length()<1){
                return -1;
            }
            int ret = 0;
            for (size_t i=0;i<mod.size();i++){
                switch(mod[i]){
                    case 'Y':
                        ret = ret*10000 + TM.tm_year+1900;
                        break;
                    case 'M':
                        ret = ret*100 + TM.tm_mon + 1;
                        break;
                    case 'D':
                        ret = ret*100 + TM.tm_mday;
                        break;
                    case 'w':
                        ret = (TM.tm_wday+6)%7+1;
                        break; 
                    case 'h':
                        ret = ret*100 +  TM.tm_hour;
                        break;
                    case 'm':
                        ret = ret*100 +  TM.tm_min;
                        break;
                    default:
                        break;
                }
            }
            return ret;
        }

        static time_t getNow(){
            time_t ret;
            time(&ret);
            return ret;
        }

        static double getHour(const time_t& t,double zone=8){
            return fmod(t/3600+zone,24);
        }


        static std::string toString(const time_t& t,double zone = 8,const char* disp="%Y%m%d_%R"){
            time_t tz = t + static_cast<int>(zone * 3600);
            struct tm TM;
            gmtime_r(&tz,&TM);
            char buff[16];
            strftime(buff,16,disp,&TM);
            buff[14] = '\0';
            return (std::string)buff;
        }


        static time_t toTime(const std::string& s/*20140708_18:40*/, double zone = 8) {
            const char* s_c = s.c_str();
            char* pstr;
            long year,month,day,hour,min,sec;
            hour = min = sec = 0;
            year = strtol(s_c,&pstr,10);
            month = (year/100)%100;
            day = (year%100);
            year = year/10000;
            if (*pstr != '\0'){
                hour = strtol(++pstr,&pstr,10);
                if (*pstr != '\0'){
                    min = strtol(++pstr,&pstr,10);
                }
            }

            int leap_year = (year-1969)/4;	//year-1-1968
            int i = (year-2001)/100;		//year-1-2000
            leap_year -= ((i/4)*3+i%4);
            int day_offset = 0;
            for (i=0;i<month-1;i++)
                day_offset += mon_day[i];
            bool isLeap = ((year%4==0&&year%100!=0)||(year%400==0));
            if (isLeap && month>2)
                day_offset += 1;
            day_offset += (day-1);
            day_offset += ((year-1970)*365 + leap_year);
            double hour_offset = hour - zone;
            time_t ret = day_offset*86400 + static_cast<int>(hour_offset * 3600) + min*60 + sec;
            return ret;
        }

	static time_t toTime(const std::string& s, const std::string& fmt/*="%Y%m%d_%H:%M"*/, double zone=8){
		struct tm tb = {0};
		time_t offset = 3600*zone;
		if (strptime(s.c_str(), fmt.c_str(), &tb) != NULL) {
			return ::timegm(&tb)-offset;
		}else{
			fprintf(stderr,"[ERROR:toTime() fomat invalid]%s -> %s\n",s.c_str(),fmt.c_str());
			return -1;
		}
	}

        static int compareDayStr(const std::string& a,const std::string& b){
            if (a==b)
                return 0;
            time_t a_t = toTime(a+"_00:00",0);
            time_t b_t = toTime(b+"_00:00",0);
            return (b_t-a_t)/86400;
        }
        static int compareTimeStr(const std::string& a,const std::string& b){
            time_t a_t = toTime(a,0);
            time_t b_t = toTime(b,0);
            return (b_t-a_t);
        }
        static std::string datePlusOf(const std::string& day,int num){
            time_t day_time = toTime(day.substr(0,8)+"_00:00",0);
            day_time += num*86400;
            return toString(day_time,0).substr(0,8);
        }
        static std::string datePlusOfHour(const std::string &date, int num) {
            time_t day_time = toTime(date,0);
            day_time += num*3600;
            return toString(day_time,0);
        }
        // timeP: 格式 **:**
        static int getOffsetHM(const std::string& timeP) {
            std::string::size_type pos = timeP.find(":");
            std::string::size_type rPos = timeP.rfind(":");
            if (pos == std::string::npos || pos != rPos) {
                fprintf(stderr, "MyTime::getOffsetHM, format error: %s!", timeP.c_str());
                return 0;
            }
            std::string hourS = timeP.substr(0, pos);
            std::string hourM = timeP.substr(pos + 1);
            int ret = atoi(hourS.c_str()) % 24 * 3600 + atoi(hourM.c_str()) % 60  * 60;
            return ret;
        }
};

}       //namespace MJ

#endif  //__MY_TIME_H__
