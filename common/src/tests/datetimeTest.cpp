#include "../time/DateTime.h"
#include "../ThreadPool/CountDownLatch.h"
#include "../ThreadPool/ThreadPool.h"
#include <boost/timer.hpp>
#include <assert.h>


using namespace std;

#define POOLSIZE 4
#define QUEUESIZE 10

CountDownLatch cdl(POOLSIZE);

void DateTime_test()
{
    {
        boost::timer t;
        for(int i=0; i<300000; i++)
        {
            DateTime dt=DateTime::Now();
            dt=DateTime::Today();
            TimeSpan ts=DateTime::Time();
            dt =DateTime(2015,9,14,0,0,0);

        }
        cout<<"DateTime creat instances cost time "<<t.elapsed()<<"s"<<endl;
    }
    cdl.countDown();

}

int main()
{
    DateTime dt;
    dt.SetTime(TimeSpan(12,0,0));
    assert(!DateTime::TryParse_s(string("0001-01-01T00:00:00"),"yyyy'-'MM'-'dd'T'HH':'mm':'ss", dt));
    cout<<"TryParse_s is safe!"<<"\n";
    return 0;
    {
        boost::timer t;
        for(int i=0; i<1000000; i++)
            DateTime dt=DateTime::Now();
        cout<<"DateTime::Now() cost time "<<t.elapsed()<<"s"<<endl;
    }

    {
        boost::timer t;
        for(int i=0; i<1000000; i++)
            DateTime dt=DateTime::Today();
        cout<<"DateTime::Today() cost time "<<t.elapsed()<<"s"<<endl;
    }

    {
        boost::timer t;
        for(int i=0; i<1000000; i++)
            TimeSpan ts=DateTime::Time();
        cout<<"TimeSpan::Time() cost time "<<t.elapsed()<<"s"<<endl;
    }

    {
        boost::timer t;
        for(int i=0; i<1000000; i++)
            DateTime dt =DateTime(2015,9,14,0,0,0);
        cout<<"DateTime::DateTime(int dwYear, int dwMonth, int dwDay) cost time "<<t.elapsed()<<"s"<<endl;
    }

    {
        boost::timer t;
        for(int i=0; i<1000000; i++)
            tzset();
        cout<<"tzset() cost time "<<t.elapsed()<<"s"<<endl;
    }

    {
        boost::timer t;
        for(int i=0; i<3000; i++)
        {
            DateTime dt=DateTime::Now();
            dt=DateTime::Today();
            TimeSpan ts=DateTime::Time();
            dt =DateTime(2015,9,14,0,0,0);
            if(i==0)
            {
                cout<<dt.ToString("s")<<endl;
            }

        }
        cout<<"DateTime creat instances cost time "<<t.elapsed()<<"s"<<endl;
    }

    ThreadPool tp(QUEUESIZE, POOLSIZE);
    tp.start();

    for (int i = 0; i < POOLSIZE; ++i)
    {
        std::function<void (int)> task = bind(DateTime_test);
        tp.addTask(task);
    }

    cdl.wait();

    tp.stop();

    return 0;
}
