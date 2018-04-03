/*************************************************************************
    > File Name: test_redis_expire.cpp
    > Author: yuyang
    > Mail: uangyy@gmail.com 
    > Created Time: Thu 19 Nov 2015 01:23:16 PM CST
 ************************************************************************/

#include <iostream>
#include "MyRedis.h"
#include "../json/json.h"
using namespace std;

int main(int argc, char **argv)
{
    Json::FastWriter jfw;
    Json::Reader reader;
    Json::Value json_res;
    MyRedis m_redis;
    //m_redis.init("10.10.69.158:6379", 10);
    m_redis.init("127.0.0.1:6379", 10);
    string key = "hello", res, value;
    m_redis.set(key, key);
    m_redis.expire(key, "100");
    m_redis.get(key + "abc", value);
    cout << value << endl;
    reader.parse(value, json_res);
    cout << json_res << endl;
    int ttl = 0;
    m_redis.ttl(key, &ttl);
    cout << "ttl: " << ttl << endl;
    /*
    string json_str = "{\"money\":\"10\", \"age\":1}";
    cout << json_str << endl;
    Json::Reader reader;
    Json::Value value;
    reader.parse(json_str, value);
    cout << value["money"].asString() << endl;
    cout << value["age"].asInt() << endl;
    */
    return 0;
}
