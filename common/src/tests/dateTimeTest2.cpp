#include "../time/DateTime.h"
#include <assert.h>

using namespace std;

//定义三态布尔变量
enum tribool{
            _TRUE          =  1,
            _INDETERMINATE = -1,
            _FALSE         =  0,
};

tribool travel_time_cost_check(const string& dept_time, const int dept_zone, const string& dest_time, const int dest_zone, const double  min_seconds_history, const int tolerance=10)
{
    static DateTime tmp_d;
    static TimeSpan tmp_t;
    DateTime dept_dt,dest_dt;

    dept_dt.SetTime(TimeSpan(12,0,0));
    if(!tmp_d.TryParse(dept_time,"yyyy'-'MM'-'dd HH':'mm':'ss",dept_dt))
        return _INDETERMINATE;
    if(dept_zone>12 || dept_zone <-12)
        return _INDETERMINATE;
    dept_dt=dept_dt-TimeSpan(dept_zone,0,0);


    dest_dt.SetTime(TimeSpan(12,0,0));
    if(!tmp_d.TryParse(dest_time,"yyyy'-'MM'-'dd HH':'mm':'ss",dest_dt))
        return _INDETERMINATE;
    if(dest_zone>12 || dest_zone <-12)
        return _INDETERMINATE;
    dest_dt=dest_dt-TimeSpan(dest_zone,0,0);

    if(min_seconds_history<=0)
        return _INDETERMINATE;

    /* TimeSpan ts_1=dest_dt-dept_dt;
    TimeSpan ts_2=tmp_t.FromSeconds(min_seconds_history)-TimeSpan(0,0,10);
    cout << ts_1.GetTotalSeconds()<<" "<<ts_2.GetTotalSeconds()<<"\n"; */
    if((dest_dt-dept_dt)>=(tmp_t.FromSeconds(min_seconds_history)-TimeSpan(0,0,tolerance)))
        return _TRUE;
    else
        return _FALSE;
}

int main()
 {
    cout<<travel_time_cost_check("2015-08-04 12:00:00",1,"2015-08-04 14:00:00",2,3600.123)<<" 1\n";
    cout<<travel_time_cost_check("2015-08-04 12:00:00",1,"2015-08-04 14:00:00",2,3580.123)<<" 1\n";
    cout<<travel_time_cost_check("2015-08-04 12:00:00",1,"2015-08-04 14:00:00",2,3700.123)<<" 0\n";
    cout<<travel_time_cost_check("2015-08-04 12:34:00",-7,"2015-08-05 03:34:56",8,30)<<" 1\n";

    cout<<travel_time_cost_check("2015-08-04 12:34:t0",-7,"2015-08-05 03:34:56",8,30)<<" -1\n";
    cout<<travel_time_cost_check("2015-08-04 12:34:00",-13,"2015-08-05 03:34:56",8,30)<<" -1\n";
    cout<<travel_time_cost_check("2015-08-04 12:34:00",-7,"2015-08-05 03:3n:56",8,30)<<" -1\n";
    cout<<travel_time_cost_check("2015-08-04 12:34:00",-7,"2015-08-05 03:34:56",100,30)<<" -1\n";
    cout<<travel_time_cost_check("2015-08-04 12:34:00",-7,"2015-08-05 03:34:56",8,-30)<<" -1\n";


    DateTime tmp;

        DateTime dest_dt;
   dest_dt.SetTime(TimeSpan(12,0,0));
   if(!DateTime::TryParse_s(string("2015-2-29T16:35:00"),"yyyy'-'MM'-'dd'T'HH':'mm':'ss",dest_dt))
       cout<<"TryParse_s is safe!"<<"\n";

   if(DateTime::TryParse(string("2015-8-10T16:35:00"),"yyyy'-'MM'-'dd'T'HH':'mm':'ss",dest_dt))
       cout<<dest_dt.GetDayOfWeek()<<"\n";

   if(DateTime::TryParse(string("2015-8-13T16:35:00"),"yyyy'-'MM'-'dd'T'HH':'mm':'ss",dest_dt))
       cout<<dest_dt.GetDayOfWeek()<<"\n";
   if(DateTime::TryParse(string("2015-8-08T16:35:00"),"yyyy'-'MM'-'dd'T'HH':'mm':'ss",dest_dt))
       cout<<dest_dt.GetDayOfWeek()<<"\n";
    return 0;
 }
