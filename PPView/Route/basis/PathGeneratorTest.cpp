#include<string.h>
#include "PathGenerator.h"
#include<stdio.h>
#include<string>
#include<map>
#include <mysql/mysql.h>

using namespace std;

int main(){
    const string user = "root";
    const string pswd = "miaoji1109";
    const string host = "10.10.169.10";
    const string db = "ppview_core_test";
    unsigned int port = 3306;
    map<string,string> in_id;
    map<string,string> in_out;
    string fn,comment,num,id;
    getline(cin,fn);
//    getline(cin,id);
//    getline(cin,comment);
//    getline(cin,num);
//    cin>>fn>>comment>>num;
    MYSQL mysql;
    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql,host.c_str(),user.c_str(),pswd.c_str(),db.c_str(),0,NULL,0)){
        printf("Connect to %s error: %s",db.c_str(),mysql_error(&mysql));
        return 1;
    }
    else
    {
        cout<<"connect success"<<endl;
//        string sqlstr="select input,output from cases where fn='"+fn+"' and comment='"+comment+"' and disable !=1 limit "+num+";";
//        string sqlstr="select input,output from cases where fn='"+fn+"' and disable !=1 limit "+num+";";
        string sqlstr="select input,output,id from cases where fn='"+fn+"' and disable !=1 ;";
//        string sqlstr="select input,output,id from cases where fn='"+fn+"' and disable !=1 and id="+id+";";
        printf("%s\n",sqlstr.c_str());
        int t=mysql_query(&mysql,"set names utf8");
        t=mysql_query(&mysql,sqlstr.c_str());
        if(t!=0){
            printf("mysql query error: %s\nquery_string: %s\n",mysql_error(&mysql),sqlstr.c_str());
            return 1;
        }
        else{
            MYSQL_RES* res = mysql_use_result(&mysql);
            MYSQL_ROW row;
            if(res){
                while(row=mysql_fetch_row(res)){
//                    if(row[0] == NULL || row[1] == NULL)
//                        continue;
                    in_out[row[0]]=row[1];
                    in_id[row[0]]=row[2];
                }
            }
            mysql_free_result(res);
        }
    }
    mysql_close(&mysql);
    Json::Reader reader;
    Json::FastWriter fw;
	fw.omitEndingLineFeed();
    Json::Value routeJ;
    map<string,string>::iterator it;
    int z=1;
    for (it = in_out.begin();it != in_out.end();it++){
        printf("case %d:\n",z++);
        cout<<"input: "<<it->first.c_str()<<endl;
        string fin=it->first.c_str();
        reader.parse(fin,routeJ);
        PathGenerator* path = new PathGenerator(routeJ);
        string newOut;
        Json::Value out_routeJ;
        if(fn == "DayPathSearch"){
            path->DayPathSearch();
            out_routeJ=path->GetResult();
            newOut=fw.write(out_routeJ);
        }
        if(fn == "selectOpenClose"){
            path->selectOpenClose(routeJ);
            //path->DayPathExpandOpt();
            //out_routeJ=path->GetResult();
			out_routeJ = routeJ;
            newOut=fw.write(out_routeJ);
        }
        if(fn == "DayPathExpand"){
            path->DayPathExpand();
            out_routeJ=path->GetResult();
            newOut=fw.write(out_routeJ);
        }
        if(fn == "DayPathExpandOpt"){
            path->DayPathExpandOpt();
            out_routeJ=path->GetResult();
            newOut=fw.write(out_routeJ);
        }

        cout<<"origin output: "<<it->second.c_str()<<endl;
        Json::Value output;
        reader.parse(it->second.c_str(),output);
        for(int i=0;i<output.size();i++){
            Json::Value node;
            cout<<output[i]["id"].asString()<<" - ";
        }cout<<endl;
        cout<<"new output: "<<newOut.c_str()<<endl;
        for(int i=0;i<out_routeJ.size();i++){
            cout<<out_routeJ[i]["id"].asString()<<" - ";
        }cout<<endl;

		for (int i = 0; i < out_routeJ.size() && i < output.size(); i++) {
			if (out_routeJ[i] != output[i]) cout << "the index: " << i << "  is not same" << std::endl;
		}
            
        cout<<"ID: "<<in_id[it->first.c_str()].c_str()<<endl;
        if(strcmp(it->second.c_str(),newOut.c_str()) == 0){
            cout<<"result: YES, as expected..."<<endl;
        }
        else{
            cout<<"result: NO, unexpected..."<<endl;
        }
    }
    /*
	std::ifstream fin;
	Json::Reader reader;
	fin.open("./OptCase.txt");
	int z=1;
	if(fin.is_open()){
		while(fin.good()&&!fin.eof()){
			std::string line = "";
			Json::Value in_routeJ,out_routeJ;
			std::getline(fin, line);
			if (!line.empty() && line[0] == '#') continue;
			reader.parse(line,in_routeJ);
			if (in_routeJ.isNull()) continue;
			std::cout<<"case: "<<z<<std::endl;
			Json::FastWriter fw;
			std::cout<<"input_routeJ: "<<endl<<fw.write(in_routeJ)<<std::endl;
			z++;
			PathGenerator* path = new PathGenerator(in_routeJ);
            path->DayPathExpandOpt();
            out_routeJ=path->GetResult();
//            path->TimeEnrich();
//            Json::Value out_routeJ = path->GetResult();
//            path->DayPathExpand();
//            Json::Value out_routeJ = path->GetResult();
//            path->selectOpenClose(routeJ);
//            Json::Value out_routeJ=routeJ;
//			path->DayPathSearch();
//			Json::Value out_routeJ = path->GetResult();
			std::cout << "out_routeJ:" << std::endl << fw.write(out_routeJ) << std::endl;
			for (int i = 0; i < out_routeJ.size(); i++) {
				std::cout << out_routeJ[i]["id"].asString() << " - ";
			}
			std::cout << std::endl;
			delete path;
			path = NULL;
		}
	}
	fin.close();
    */
	return 0;
}
