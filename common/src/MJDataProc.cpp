#include "MJDataProc.h"
#include "json/json.h"


using namespace std;
using namespace MJ;

MJDataProc::MJDataProc(){}
MJDataProc::~MJDataProc(){}

//获取企业所有私有供应商ID列表
int MJDataProc::getPrivateSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData){
    retData.clear();
    string key = "SID_PRI_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}
//获取企业所有公有OTA/API SID列表
int MJDataProc::getBaseSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData){
    retData.clear();
    string key = "SID_BASE_" + ptid;
    dataRedis->smembers(key,retData);
    if (retData.size()!=0)
        return 0;

    //获取父企业ID
    key = "PTID_" + ptid;
    string ret = "";
    dataRedis->get(key,ret);
    Json::Reader jr;
    Json::Value j;
    string gid = "";
    if (jr.parse(ret,j)){
        if (j.isObject() && j["gid"].isString()){
            gid = j["gid"].asString();
        }
    }
    //获取父企业的base权限
    if (gid.length()!=0 && gid!="mjid"){
        key = "SID_BASE_DOWN_" + gid;
        std::tr1::unordered_set<std::string> retData_g;
        dataRedis->smembers(key,retData_g);
        retData.insert(retData_g.begin(),retData_g.end());
    }

    return 0;
}
//获取企业所有私有&OTA&API的SID列表
int MJDataProc::getAllSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData){
    getBaseSID(ptid,dataRedis,retData);
    std::tr1::unordered_set<std::string> retData_pri;
    getPrivateSID(ptid,dataRedis,retData_pri);
    retData.insert(retData_pri.begin(),retData_pri.end());
    return 0;
}


//获取企业拥有的私有数据（包含公转私）
int MJDataProc::getPriData_Hotel(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "HOTEL_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}

//获取企业所屏蔽的公有数据
int MJDataProc::getShieldBaseData_Hotel(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_set<string>& retData){
    string key = "HOTEL_BLACK_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}
//获取企业所有的被分销的私有基础数据(包含公转私)
int MJDataProc::getDistrPriData_Hotel(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_map<string,std::tr1::unordered_set<string> >& retData){
    //1,获取企业所有的上游分销企业
    string product = "1";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的酒店ID 
    Json::Reader jr;
    Json::Value j;
    auto it = upper_ptid.begin();
    for (;it!=upper_ptid.end();it++){
        key = "SELL_" + product + "_" + *it + "_" + ptid;
        //获取上游企业对该企业的分销房型ID
        vector<string> rooms_id;
        vector<string> rooms_info;
        dataRedis->smembers(key,rooms_id);
        //获取上游企业对该企业的分销房型信息
        dataRedis->get(rooms_id,rooms_info);
        //获取上游企业对该企业分销房型对应的酒店ID
        tr1::unordered_set<string> hotels_id;
        for(size_t i=0;i<rooms_info.size();i++){
            if (jr.parse(rooms_info[i],j)){
                if (j.isObject() && j["hid"].isString())
                    hotels_id.insert(j["hid"].asString());
            }
        }
        retData[*it] = hotels_id;
    }
    return 0;
}

//获取企业所有的被分销的私有房型
int MJDataProc::getDistrPriData_HotelRoom(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_map<string,std::tr1::unordered_set<string> >& retData){
    //1,获取企业所有的上游分销企业
    string product = "1";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的酒店ID 
    auto it = upper_ptid.begin();
    for (;it!=upper_ptid.end();it++){
        key = "SELL_" + product + "_" + *it + "_" + ptid;
        //获取上游企业对该企业的分销房型ID
        tr1::unordered_set<string> rooms_id;
        dataRedis->smembers(key,rooms_id);
        retData[*it] = rooms_id;
    }
    return 0;
}

//获取企业拥有的私有数据（包含公转私）
int MJDataProc::getPriData_View(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "VIEW_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}

//获取企业所屏蔽的公有数据
int MJDataProc::getShieldBaseData_View(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_set<string>& retData){
    string key = "VIEW_BLACK_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}
//获取企业所有的被分销的私有基础数据(包含公转私)
int MJDataProc::getDistrPriData_View(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_map<string,std::tr1::unordered_set<string> >& retData){
    
    return 0;
}

//获取企业拥有的私有数据（包含公转私）
int MJDataProc::getPriData_Shop(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "SHOP_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}

//获取企业所屏蔽的公有数据
int MJDataProc::getShieldBaseData_Shop(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_set<string>& retData){
    string key = "SHOP_BLACK_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}


//获取企业拥有的私有数据（包含公转私）
int MJDataProc::getPriData_Restaurant(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "REST_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}

//获取企业所屏蔽的公有数据
int MJDataProc::getShieldBaseData_Restaurant(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_set<string>& retData){
    string key = "REST_BLACK_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}




//获取企业拥有的商业产品数据
int MJDataProc::getProduct_Wanle_ViewTicket(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "VTICKET_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}
int MJDataProc::getProduct_Wanle_PLAY(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "PLAY_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}
int MJDataProc::getProduct_Wanle_ACT(const std::string& ptid, MyRedis* dataRedis,tr1::unordered_set<string>& retData){
    string key = "ACT_" + ptid;
    dataRedis->smembers(key,retData);
    return 0;
}

//获取企业所有的被分销的商业产品数据
int MJDataProc::getDistrProduct_Wanle(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_map<string,std::tr1::unordered_set<string> >& retData){
    //1,获取企业所有的上游分销企业
    string product = "128";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的玩乐门票ID 
    Json::Reader jr;
    Json::Value j;
    
    auto it = upper_ptid.begin();
    vector<string> tickets_id;
    vector<string> tickets_info;
    tr1::unordered_set<string> wanle_id;
    for (;it!=upper_ptid.end();it++){
        wanle_id.clear();
        //景点门票 演出赛事 特色活动
        if (1){
            key = "SELL_" + product + "_" + *it + "_" + ptid;
            tickets_id.clear();
            tickets_info.clear();
            //获取上游企业对该企业的分销玩乐票务ID
            dataRedis->smembers(key,tickets_id);
            //获取上游企业对该企业的分销玩乐票务信息
            dataRedis->get(tickets_id,tickets_info);
            //获取上游企业对该企业分销玩乐票务对应的玩乐产品ID
            for(size_t i=0;i<tickets_info.size();i++){
                if (jr.parse(tickets_info[i],j)){
                    if (j.isObject() && j["pid"].isString())
                        wanle_id.insert(j["pid"].asString());
                }
            }
        }
        retData[*it] = wanle_id;
    }
    return 0;
}


//获取企业所有的被分销的商业产品Ticket_ID  
int MJDataProc::getDistrProduct_WanleTicket(const std::string& ptid, MyRedis* dataRedis, tr1::unordered_map<string,std::tr1::unordered_set<string> >& retData){
    //1,获取企业所有的上游分销企业
    string product = "128";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的酒店ID 
    Json::Reader jr;
    Json::Value j;
    auto it = upper_ptid.begin();
    tr1::unordered_set<string> wanle_ticket_id;
    for (;it!=upper_ptid.end();it++){
        //景点门票 演出赛事 特色活动
        if (1){
            key = "SELL_" + product + "_" + *it + "_" + ptid;
            //获取上游企业对该企业的分销玩乐票务ID
            dataRedis->smembers(key,wanle_ticket_id);
        }
        retData[*it] = wanle_ticket_id;
    }
    return 0;
}

//获取企业可以使用的包车产品的公司列表
int MJDataProc::getDistrProduct_Baoche(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData){
    //1,获取企业所有的上游分销企业
    string product = "2";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的产品
    auto it = upper_ptid.begin();
    tr1::unordered_set<string> pd;
    for (;it!=upper_ptid.end();it++){
        key = "SELL_" + product + "_" + *it + "_" + ptid;
        //获取上游企业对该企业的分销产品
        dataRedis->smembers(key,pd);
        if (pd.size()>0)
            retData.insert(*it);
    }
    return 0;
}
//获取企业可以使用的餐食产品的公司列表
int MJDataProc::getDistrProduct_Food(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData){
    //1,获取企业所有的上游分销企业
    string product = "256";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的产品
    auto it = upper_ptid.begin();
    tr1::unordered_set<string> pd;
    for (;it!=upper_ptid.end();it++){
        key = "SELL_" + product + "_" + *it + "_" + ptid;
        //获取上游企业对该企业的分销产品
        dataRedis->smembers(key,pd);
        if (pd.size()>0)
            retData.insert(*it);
    }
    return 0;
}
//获取企业可以使用的导游产品的公司列表
int MJDataProc::getDistrProduct_Guide(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData){
    //1,获取企业所有的上游分销企业
    string product = "512";
    string key = "UPPER_" + product + "_" + ptid;
    tr1::unordered_set<string> upper_ptid;
    dataRedis->smembers(key,upper_ptid);
    //2,迭代获取上游分销企业对本企业分销的产品
    auto it = upper_ptid.begin();
    tr1::unordered_set<string> pd;
    for (;it!=upper_ptid.end();it++){
        key = "SELL_" + product + "_" + *it + "_" + ptid;
        //获取上游企业对该企业的分销产品
        dataRedis->smembers(key,pd);
        if (pd.size()>0)
            retData.insert(*it);
    }
    return 0;
}


