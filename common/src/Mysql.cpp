#include "Mysql.h"

using namespace std;
using namespace MJ;

Mysql::Mysql(const string& host, const string& db, const string& usr, const string& passwd)
    :m_host(host), m_db(db), m_usr(usr), m_passwd(passwd)
{
    mysql_ = (MYSQL*)malloc(sizeof(MYSQL));
    mysql_init(mysql_);
}


Mysql::~Mysql()
{
    mysql_close(mysql_);
    delete mysql_;
}


bool Mysql::connect()
{
    if (!mysql_real_connect(mysql_, m_host.c_str(), m_usr.c_str(), m_passwd.c_str(), m_db.c_str(), 0, NULL, 0))
    {
        _ERROR("[Connect to %s error: %s]", m_db.c_str(), mysql_error(mysql_));
        return false;
    }

    if (mysql_set_character_set(mysql_, "utf8"))
    {
        _ERROR("[Set mysql characterset: %s]", mysql_error(mysql_));
        return false;
    }

    return true;
}


bool Mysql::query(const string& sql)
{
    int t = mysql_query(mysql_, sql.c_str());
    if (t != 0)
    {
        _ERROR("[mysql_query error: %s] [error sql: %s]", mysql_error(mysql_), sql.c_str());
        return false;
    }

    return true;
}

MYSQL_RES* Mysql::use_result()
{
    return mysql_use_result(mysql_);
}

MYSQL_ROW Mysql::fetch_row(MYSQL_RES* res)
{
    return mysql_fetch_row(res);
}

int Mysql::num_fields(MYSQL_RES* res)
{
    return mysql_num_fields(res);
}


void Mysql::free_result(MYSQL_RES* res)
{
   mysql_free_result(res);
}


