#include "PathGenerator.h"
#include <cstdio>
#include<fstream>
#include<string.h>
#define maxx 10000000
using namespace std;


int main(){
    ifstream fin;
    Json::Reader reader;
    Json::FastWriter writer;
    fin.open("./case.txt");
//    int z=1;
    if(fin.is_open()){
        while(fin.good()&&!fin.eof()){
//            cout<<"case: "<<z<<endl;
//            z++;
            char buf[maxx];
            memset(buf,0,sizeof(buf));
            Json::Value routeJ;
            fin.getline(buf,maxx);
//            cout<<"before "<<endl;
//            cout<<buf<<endl;
            reader.parse(buf,routeJ);
            PathGenerator X(routeJ,Json::Value());
            X.DayPathExpand();
            Json::Value result=X.GetResult();
//            cout<<"after DayPathExpand"<<endl;
            cout<<writer.write(result)<<endl;
        }
    }
    fin.close();
    return 0;
}
