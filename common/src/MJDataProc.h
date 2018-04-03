#ifndef __MJ_DATA_PROC__
#define __MJ_DATA_PROC__

#include <tr1/unordered_set>
#include <string>
#include <vector>
#include "MyRedis.h"

namespace MJ{

/*线程安全*/
class MJDataProc
{
public:
    MJDataProc();
    ~MJDataProc();
public:
    /* 权限 */
    //获取企业所有私有供应商ID列表
    int getPrivateSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所有公有OTA/API SID列表
    int getBaseSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所有私有&OTA&API的SID列表
    int getAllSID(const std::string& ptid,MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);


    /*  酒店  */
    //获取企业拥有的私有酒店（包含公转私）
    int getPriData_Hotel(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所屏蔽的公有酒店
    int getShieldBaseData_Hotel(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
    //获取企业所有的被分销的酒店ID(包含公转私)
    int getDistrPriData_Hotel(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> >& retData);
    //获取企业所有的被分销的酒店房型
    int getDistrPriData_HotelRoom(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> >& retData);

    /*  景点  */
    //获取企业拥有的私有景点（包含公转私）
    int getPriData_View(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所屏蔽的公有景点
    int getShieldBaseData_View(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);    
    //获取企业所有的被分销的私有景点(包含公转私)
    int getDistrPriData_View(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> >& retData);
    
    /*  购物  */
    //获取企业拥有的私有购物（包含公转私）
    int getPriData_Shop(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所屏蔽的公有购物
    int getShieldBaseData_Shop(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
    
    /*  餐馆  */
    //获取企业拥有的私有餐馆（包含公转私）
    int getPriData_Restaurant(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所屏蔽的公有餐馆
    int getShieldBaseData_Restaurant(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
    
    /*  玩乐  */
    //获取企业拥有的景点门票产品
    int getProduct_Wanle_ViewTicket(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业拥有的演出赛事产品
    int getProduct_Wanle_PLAY(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业拥有的特色活动产品
    int getProduct_Wanle_ACT(const std::string& ptid, MyRedis* dataRedis,std::tr1::unordered_set<std::string>& retData);
    //获取企业所有的被分销的商业产品PID  
    int getDistrProduct_Wanle(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> >& retData);
    //获取企业所有的被分销的商业产品Ticket_ID 
    int getDistrProduct_WanleTicket(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> >& retData);
    
    /*  车餐导  */
    //获取企业可以使用的包车产品的公司列表
    int getDistrProduct_Baoche(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
    //获取企业可以使用的餐食产品的公司列表
    int getDistrProduct_Food(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
    //获取企业可以使用的导游产品的公司列表
    int getDistrProduct_Guide(const std::string& ptid, MyRedis* dataRedis, std::tr1::unordered_set<std::string>& retData);
};

}       //namespace MJ


#endif //__MJ_DATA_PROC__