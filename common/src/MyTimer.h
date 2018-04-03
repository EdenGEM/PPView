#ifndef __MY_TIMER_H__
#define __MY_TIMER_H__


namespace MJ{
//用来计时的类
class MyTimer{
private:
    struct timeval _b;
    struct timeval _e;
public:
    MyTimer(){
        gettimeofday(&_b,NULL);
    }
    void start(){
        gettimeofday(&_b,NULL);
    }
    int cost(){
        gettimeofday(&_e,NULL);
        return (_e.tv_sec-_b.tv_sec)*1000000+(_e.tv_usec-_b.tv_usec);
    }
};

}       //namespace MJ

#endif  //__MY_TIMER_H__

