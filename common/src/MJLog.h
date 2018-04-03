#ifndef __MJ_LOG_H__
#define __MJ_LOG_H__

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sys/time.h>
#include <stdarg.h>

namespace MJ{


#define MAXLOGLEN 2048000

#define __MY_FILE_NAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)

#define MJ_LOG(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[%s][%s:%s:%u]" fmt "\n",\
	__strftime_buf__, __MY_FILE_NAME__, __func__, __LINE__, ##__VA_ARGS__);\
}) 

#define MJ_LOG_INFO(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[INFO][%s][%s:%s:%u]" fmt "\n",\
	__strftime_buf__, __MY_FILE_NAME__, __func__, __LINE__, ##__VA_ARGS__);\
}) 


#define MJ_LOG_ERROR(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[ERROR][%s][%s:%s:%u]" fmt "\n",\
	__strftime_buf__, __MY_FILE_NAME__, __func__, __LINE__, ##__VA_ARGS__);\
}) 


#define MJ_LOG_REQUEST(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[%s][Request MiojiOPObserver," fmt "]\n",\
	__strftime_buf__, ##__VA_ARGS__);\
}) 


#define MJ_LOG_RESPONSE(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[%s][Response MiojiOPObserver," fmt "]\n",\
	__strftime_buf__, ##__VA_ARGS__);\
}) 


#define MJ_LOG_DEBUG(fmt, ...) ({ 	\
	time_t __time_buf__;\
	tm __localtime_buf__;\
	char __strftime_buf__[18];\
	time(&__time_buf__);\
	localtime_r(&__time_buf__, &__localtime_buf__);\
	strftime(__strftime_buf__, 18, "%Y%m%d %H:%M:%S", &__localtime_buf__);\
	fprintf(stderr, "[%s][Debug MiojiOPObserver," fmt "]\n",\
	__strftime_buf__, ##__VA_ARGS__);\
})


void logMJOB_RequestClient(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& req, 
                const size_t len);
void logMJOB_ResponseClient(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& resp, 
                const int len,
                const long& cost);
void logMJOB_RequestServer(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& req);
void logMJOB_ResponseServer(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& resp, 
                const long& cost);
void logMJOB_Exception(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& debug);
void logMJOB_Reload(const long& ts,
                const std::string& ip,
                const int error_id,
                const long& cost,
                const std::string& debug);


void logMJOB_MSGSend(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const std::string& req);

void logMJOB_MSGRecv(const std::string& type, 
                const std::string& uid,
                const std::string& csuid,
                const std::string& qid,
                const long& ts,
                const std::string& ip,
                const std::string& refer_id,
                const std::string& cur_id,
                const std::string& next_id,
                const int len);


class MYLog{
    private:
        static pthread_mutex_t _locker;
        static time_t _time;
        static std::ofstream _file;
        static std::string _dir;
    public:
        static void write(const std::string& log);
        static void write(const int& log);
        static void init(const std::string& dir);
};

class PrintInfo {
    private:
        static int NEED_STD_LOG;
        static int NEED_ERR_LOG;
    public:
        static int NEED_DUMP_LOG;
        static int NEED_DBG_LOG;
        static int SwitchDbg(int need_dbg, int need_log, int need_err, int need_dump);
        static int PrintLog(const char* format, ...);
        static int PrintDbg(const char* format, ...);
        static int PrintErr(const char* format, ...);
};



}	//namespace MJ

#endif




