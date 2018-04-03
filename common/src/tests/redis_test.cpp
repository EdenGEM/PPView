#include <iostream>
#include <vector>

#include "../common/MyRedis.h"

using namespace std;

int main()
{
    MyRedis redis;
    redis.init("127.0.0.1:6379", 15);
    redis.set("redis_test_key", "redis_test_value");
    redis.set("redis test key", "redis test value");
    
    string val;
    redis.get("redis test key", val);
    cout << val << endl;

    vector<string> keys;
    redis.hkeys("redis_test_hset", keys);
    for (size_t i = 0; i < keys.size(); i++)
        cout << keys[i] << endl;

    redis.hget("redis_test_hset", "key", val);
    cout << val << endl;

    return 0;

}
