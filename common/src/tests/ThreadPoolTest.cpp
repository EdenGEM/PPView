#include "../threads/threads.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sys/time.h>
#include <functional>
#include <ext/pool_allocator.h>

#define POOLSIZE 3
#define QUEUESIZE 10

using namespace std;
using namespace MJ;

#define mt_alloc std::allocator

CountDownLatch cdl(POOLSIZE);

vector<vector<int,mt_alloc<int>> , mt_alloc<  vector<int,mt_alloc<int>> > > test;

void fuc(int i)
{
    struct timeval start, end;
    gettimeofday(&start, NULL);


    vector< vector<int,mt_alloc<int>> , mt_alloc<  vector<int,mt_alloc<int>> > > push_back_test;
    push_back_test.reserve(test.size());
    for(int i=0;i<test.size();i++)
    {
        static int num_add=0;
        push_back_test.push_back(test[i]);
    }

    gettimeofday(&end, NULL);
    int delttime = (1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec);
    cout << "子线程耗时 " << delttime << "us"<<endl;
    cdl.countDown();

}


int main(int argc, const char *argv[])
{
    ThreadPool tp(QUEUESIZE, POOLSIZE);
    tp.start();
    sleep(3);

    test.reserve(100000);

    for(int i=0;i<100000;i++)
    {
        vector<int,mt_alloc<int>> tmp;
        for(int j=i;j<i+36;j++)
            tmp.push_back(j);
        test.push_back(tmp);
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i != 10; ++i)
    {
        function<void (int)> task = bind(fuc, i);
        tp.addTask(task);
    }
    tp.stop();
    gettimeofday(&end, NULL);

    int delttime = (1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec);
    cout << "添加任务结束" << endl;
    cout << "一共耗时" << delttime << "us" << endl;

    cdl.wait();

    fuc(0);

    getchar();
    return 0;
}
