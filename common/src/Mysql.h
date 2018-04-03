#ifndef _MJMYSQL_HPP_
#define _MJMYSQL_HPP_

#include <string>
#include <mysql/mysql.h>
#include "ServiceLog.h"


namespace MJ
{

class Mysql
{
    public:
        Mysql(const std::string& host, const std::string& db, const std::string& usr, const std::string& passwd);
        ~Mysql();

        bool connect();
        bool query(const std::string& sql);
        MYSQL_RES* use_result();
        MYSQL_ROW fetch_row(MYSQL_RES* res);
        int num_fields(MYSQL_RES* res);
        void free_result(MYSQL_RES* res);
    private:
        std::string m_host;
        std::string m_db;
        std::string m_usr;
        std::string m_passwd;

    private:
        MYSQL* mysql_;
};

}
#endif  /*_MJMYSQL_HPP_*/
