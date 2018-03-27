#ifndef _PRIVATE_CONST_DATA_H_
#define _PRIVATE_CONST_DATA_H_

#include <string>
#include <map>
#include "define.h"
#include <vector>
#include <tr1/unordered_map>
#include <set>
#include <mysql/mysql.h>
#include <fstream>
#include<boost/noncopyable.hpp>


enum PRIVATE_ERROR{
	PRIVATE_ERROR_MEMORY,
	PRIVATE_ERROR_INFO_LOST,
	PRIVATE_DO_SOMETHING_FAILED
};

class PrivateConfig {
public:
	static int LoadPrivateConfig(const char* fileName);//加载字段
	static int AnalyzeLine(const std::string& line);//分析字段
public:
	static int m_interval;//延迟更新
	static std::vector<std::string> m_monitorList;//监视的表名 格式如 (数据库A#表B)
	static int m_stackSize;//栈大小
};


class PrivateConstData:boost::noncopyable{
public:
	~PrivateConstData() {
		Destroy();
	}
private:
	int LoadPrivateData(); //内部调用的整体逻辑
	int CopyData(const PrivateConstData& priData);
	template<typename T>
		int CopyAndInsert(const T* place,std::string id);
	int Destroy();//相当于包了一层的析构函数

    int LoadPrivateCityData();//城市
	int LoadPrivateHotelData(); //酒店
	int LoadVarPlaceData(); //景点、购物、餐厅
	int LoadTourData(); //玩乐
	int LoadTicketsFun(); //票
	int LoadSpecifiedTypeTicketsFun(std::string sql, int* dur, int* rowCnt);
	int LoadSpecifiedTypeVarPlace(std::string typeV, std::string sql, int typeI, int* dur, int* rowCnt);
	int LoadSpecifiedTypeTour(std::string typeT, std::string sql, int typeI, int* dur, int* rowCnt);
	int LoadPrivateSource();//私有源
	int LoadTag();

	//内部写操作
	int DoInsert(LYPlace* place);//插入数据
	int InsertLYPlace(LYPlace* place);//插入相应的点
	int InsertCityPlaceList(const LYPlace* place);//进行城市索引补全
    int InsertCoreHotel(const LYPlace* city);    //构造私有城市的coreHotel
	//一些其他函数 补全私有库的东西
	int FillInfo(LYPlace* place);//添加 信息 判定补全方式
	int FillInfoByConstData(const VarPlace* vPlaceConst, VarPlace* vPlace);//抄共有库的信息
	int FillVarPlaceInfo(VarPlace* vPlace);//设置景点 购物 和餐厅的默认信息

	//shield的处理比较特别只有insert没有delete
	int Insert2Shield(int type, const std::string& id, const std::string& ptid);//私有库针对共有库的删除操作
	int ShieldDump();

	int SwitchUpdate(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);
	int UpdateCity(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);
	int UpdateHotel(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);
	int UpdateVarPlaceData(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);
	int UpdateTourData(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);
	int UpdateTicketsFun(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt);

	int DelLYPlace(const std::string& priID,const std::string ptid);//删除景点
    int DelGridMapPlaceId(const LYPlace* place); //删除Grid的映射 Eden
	int DelFromPlaceMap(const LYPlace* place);//私有库id2place的map删除 防止二次删除
	int DelCityLYPlace(const LYPlace* place);//删除city的映射

    int Show();

	int DoConnectForMysql(const std::string& host, const std::string& user, const std::string& passwd, const std::string& dbName);
	int CloseMysql();//配套调用
public:
	//加载数据
	int PrivateLoad();

	//增量更新的新方法
	int Update(const std::string& startT, const std::string& endT, const std::string& dataBase, const std::string& tableName, int* dur, int* rowCnt);
	int Copy(const PrivateConstData* priData) {
		CopyData(*priData);
	}

//获取私有数据及其关联信息,下面这些函数只会被LYConstData使用
public:
	LYPlace* GetLYPlace(std::string priID,std::string ptid);//根据id获取poi点
	int GetCityViewPrivate(const std::string& cityID, std::set<std::string>& viewList,std::string ptid);//城市景点
	int GetCityShopPrivate(const std::string& cityID, std::set<std::string>& shopList,std::string ptid);//购物
	int GetCityRestaurantPrivate(const std::string& cityID, std::set<std::string>& restaurantList,std::string ptid);//餐馆
	int GetPrivateTourByType(const std::string &cityID, std::set<std::string>& tourList, int type,std::string ptid);

	int GetProdTicketsListByPid(const std::string& pid, std::vector<const TicketsFun*>& ticketsList);//只被LYConstData调用
	int GetProdTicketsById(const int& id, const TicketsFun*& ticketsFun);

	int ShieldPlaceList(std::set<std::string>& placeList, const std::string& ptid);//过滤列表
	bool IsShieldId(const std::string& ptid, const std::string& constId);//判断静态点是不是要隐藏

	std::string GetSourceNameBySid(std::string sid);

	//读取私有库数据
	int GetPrivateData(const std::string &sql, std::vector<std::vector<std::string> > &results);

private:
	std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> > _ptid2tagIds;
	std::tr1::unordered_map<std::string,LYPlace*> m_citys,m_hotels,m_views,m_shops,m_restaurants,m_tours;
	std::tr1::unordered_map<int, TicketsFun*> m_id2productTickets; //门票id对应门票信息

	typedef std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> > CITYPLACES;
	std::tr1::unordered_map<std::string, CITYPLACES> m_cityViews,m_cityShops,m_cityRestaurants,m_cityTours;
	std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> > m_shields;//企业制定删除的点不再被显示,key为ptid
	std::tr1::unordered_map<std::string, std::string> m_sid2sourceName; //源id对应源name信息

	MYSQL m_privateMysql;//私有库数据库
};

#endif //_PRIVATE_CONST_DATA_
