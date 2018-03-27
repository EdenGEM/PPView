#ifndef _LYCONSTDATA_H_
#define _LYCONSTDATA_H_

#include <string>
#include <map>
#include "define.h"
#include <vector>
#include <tr1/unordered_map>
#include <set>
#include <mysql/mysql.h>
#include "PrivateConstData.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>
#include "MyRedis.h"
#include <http/MJHotUpdate.h>
#include <threads/MyThreadPool.h>
#include "MJSquareGrid.h"

class Tag;
//存储限定的固化数据到程序内存中
class LYConstData{
	friend class LoadDataWorker;
public:
	/*全局初始化函数*/
	static bool init();
	static bool initSQL(MYSQL & mysql);
	/*全局析构函数*/
	static bool destroy();

private:
	//静态数据
	//按顺序整理接口和变量
	static bool LoadData();

//	国家
private:
	static bool LoadCountryData();
	static std::tr1::unordered_map<std::string, const Country*> _cid2country;
	static const Country* GetCountry(const std::string &cid);

private:
	static std::tr1::unordered_map<std::string,LYPlace*> _id2place;
	static std::tr1::unordered_map<std::string,LYPlace*> _id2restaurant_OnlyUsedInLoading;
	static std::tr1::unordered_map<std::string,LYPlace*> _id2hotel_OnlyUsedInLoading;
	static std::tr1::unordered_map<std::string,City*> _id2city_OnlyUsedInLoading;
	static const LYPlace* getBaseLYPlace(const std::string& id);//从共有库数据中拿点
public:
	//根据ID获取对应的地点信息
	static const LYPlace* GetLYPlace(const std::string& id, const std::string& ptid);//更全局 包含私有库

//	城市
private:
	static bool LoadCityData();
	//新数据-ID对应的地点信息
	static std::tr1::unordered_map<std::string, const LYPlace*> m_parkCity;//国家公园城市~
	static const City* getCity(const std::string& id);
public:
	static std::tr1::unordered_set<std::string> m_citySet;//城市 的集合
	static MJ::MJSquareGrid m_gridMap; //公里网映射结构体

	static bool IsParkCity(const std::string& cityId);//是否为国家公园点
    //给出中心点经纬度和距离,得到相应网格范围中的poi  Eden
    static bool GetRangePlaceId(std::string mapInfo, int maxDist, std::set<std::pair<std::string, std::string>> &idList);

//	车站的相关信息
private:
	static bool LoadStationData();

//	酒店的相关信息
private:
	static bool LoadHotelData();
	static bool LoadCoreHotel();
public:
	//coreHotel 虚拟的酒店
	static const LYPlace* GetCoreHotel(const std::string& cid);
	static const std::string GetCoreHotelCoord(const std::string& cid);

//	餐厅
private:
	static bool LoadRestaurantData();
	static std::tr1::unordered_map<std::string, std::set<std::string> > _cityRestaurants;	// CityID->Restaurant_list
public:
	//根据city ID获取对应的饭店信息
	static int getRestaurantLocal(const std::string& CityID, std::vector<const LYPlace*>& RestaurantList, const std::string& ptid);

//	购物
private:
	static bool LoadShopData();
	static std::tr1::unordered_map<std::string, std::set<std::string> > _cityShops;	//CityID->Shop_list
public:
	//根据city ID获取对应的商店信息
	static int getShopLocal(const std::string& CityID, std::vector<const LYPlace*>& ShopList, const std::string& ptid);

//	景点
private:
	static bool LoadViewData();
	static std::tr1::unordered_map<std::string, std::set<std::string> > _cityViews;
	static std::tr1::unordered_map<std::string, std::tr1::unordered_map<int, std::vector<std::string> > > m_lFHViewIDListMap; //<cid, <days, vidList> >
	static int PerfectVarPlace(std::tr1::unordered_map<std::string, std::set <std::string> >& cityPlaceMap);
	static std::tr1::unordered_map<std::string, std::string> _label_name_map;
	static std::tr1::unordered_set<std::string> _baseTagIds;
	static std::tr1::unordered_map<int, std::tr1::unordered_map<std::string, std::string> > m_poiType2smallTagNameAndId;
public:
	//根据city ID获取对应的景点信息
	static int getViewLocal(const std::string& CityID, std::vector<const LYPlace*>& ViewList, const std::string& ptid);

//源信息:
private:
	static bool LoadSource();
	//source id 与 source name对应关系
	static std::tr1::unordered_map<std::string, std::string> _sourceId2Name;
	static std::tr1::unordered_map<std::string, int> _sourceId2Api;
	static boost::thread_specific_ptr<MJ::MyRedis > _redis;
	static MJ::MyRedis* getRedisHandle();
public:
	static std::string GetSourceNameBySid(const std::string& sid);
	static int IsApi(const std::string& sid);
	static std::tr1::unordered_set<std::string> GetAvailableSources(const std::string & ptid);
	static bool IsSourceAvailable(const std::string & ptid,const std::string & source);
// 景点 购物 餐厅
private:
	static bool LoadVarPlaceData();
	static bool LoadRestaurant();
	static bool FixViewIntensity(VarPlace* varPlace, std::string rcmdIntensity, std::string intensityStr);
	static bool FixRestOrShopIntensity(VarPlace* varPlace);
	static bool LoadIntensity();
	static int LoadSpecifiedTypeVarPlace(std::string tableName, std::string sqlstr, int typeI);
//	玩乐
private:
	static bool LoadTourData();
	static bool LoadSpecifiedTypeTour(std::string typeT, std::string sql, int typeI);
	static std::tr1::unordered_map<std::string, std::set<std::string> > _cityTours;
	static int GetTourByType(const std::string &cityID, std::set<std::string >& TourList, int type, const std::string& ptid);
public:
	//根据cid,type获取tour
	static int GetTourListByType (const std::string &cityID, std::vector<const LYPlace*>& ViewList, int type, const std::string& ptid);
	//玩乐当前日期是否可用 不管票的限制
	static bool IsTourAvailable(const Tour* tour, const std::string& date);

//	票
private:
	static bool LoadTicketsFun();
	//门票id与门票对应关系
	static std::tr1::unordered_map<int, const TicketsFun*> _id2ProdTickets;
	//玩乐pid与其门票的对应关系
	static std::tr1::unordered_map<std::string, std::tr1::unordered_set<int> > _pid2ticketsId;
	static int GetProdTicketsListByPid(const std::string& pid, std::vector<const TicketsFun*>& ticksts);
public:
	static int GetConstTickets(const std::string pid, std::string ticketId, std::vector<std::vector<std::string> > &results);
	static int GetProdTicketsByPlaceAndId(const LYPlace* place, const int& id, const TicketsFun*& ticksts);
	static int GetProdTicketsListByPlace(const LYPlace* place, std::vector<const TicketsFun*>& ticksts);
	static int MakeTicketSource(const Tour *tour, const std::string& ticketDate, const TicketsFun *ticket, Json::Value &source);
	//获取景点门票对应的view,并对门票信息做修改
	static const LYPlace* GetViewByViewTicket(ViewTicket* viewTicket, const std::string& ptid);
	//玩乐当前日期是否有票
	static bool IsTourHasTicketofTheDate (const LYPlace* tour, const std::string& date, const TicketsFun*& ticket);
	//获取当前日期票价，date==""时,获取最低票价
	static bool GetTicketPriceOfTheDate (const TicketsFun* ticket, const std::string& date, float& low_price);
	//获取玩乐所有报价中的最低票价
	static bool GetBottomTicketAndPrice (const LYPlace* tour, const TicketsFun *& bottomTicket, float& bottomPrice);


//tag数据
private:
	static bool loadTag();
	static bool loadStagData();
	static bool SetTagBitset();
	static bool Insert2CityPlaceMap(const std::string& cid, const LYPlace* place, std::tr1::unordered_map<std::string, std::set<std::string> >& cid2PlaceList);//插入景点到城市列表 map
	static std::tr1::unordered_map<int, std::bitset<TAG_BITSET_SIZE> > m_PreferParamMap;
	static std::tr1::unordered_map<std::string, const Tag*> m_name2Tag;
	static std::tr1::unordered_map<std::string, const Tag*> m_id2Tag;
public:
	static std::string GetTagStrByName(const std::string &name);
	static bool IsThisTagType(const LYPlace *pPlace, const std::vector<int> &types);

private:
	static PrivateConstData* m_privateConstData; //私有库的 类

//辅助常量和函数
public:
	static LYPlace* m_segmentPlace;// 两个城市间的分割点
	static Restaurant* m_attachRest;  // 附着下一node 附近就餐点
	//是否为真实点的id （比如附近就餐不算真实点）
	static bool IsRealID(const std::string& id);
	//计算两点距离
	static int earthRadius;
	static double CaluateSphereDist(const LYPlace* place_a, const LYPlace* place_b);

//制造 varPlace的点类型 包含 餐馆 购物 景点
public:
	static  LYPlace* MakeRestaurant(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	static  LYPlace* MakeShop(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	static  LYPlace* MakeView(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	//制造酒店
	static LYPlace* MakeHotel(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	//自定义站点时使用
	static LYPlace* MakeStation(const std::string& id,int type, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	static LYPlace* MakeCarStore(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom);
	static LYPlace* MakeTour(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int type, int custom);
	static LYPlace* MakeNullPlace(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int type, int custom);

//其他
public:
	static PrivateConstData* GetPriDataPtr();
	static int GetPrivateData(const std::string &sql, std::vector<std::vector<std::string> > &results);
	//此处所谓Real为Refer的意思
	static const std::string GetRealID(const std::string& id);//获取景点的真实ID 如私有库存在映射关系 则返回真实点
private:
	static pthread_mutex_t mutex_locker_;
	static MYSQL _mysql;///sql
private://完数据后对_id2place的处理
	static bool PointCylindrical();
	static int SetPlaceIdx();

private:
	static boost::thread_specific_ptr<QueryParam > _queryParam;
	static const QueryParam* GetQueryParam()
	{
		return _queryParam.get();
	}
public:
	static const QueryParam* SetQueryParam(const QueryParam *queryParam = NULL){
		const QueryParam* res = GetQueryParam();
		if(queryParam) {
			_queryParam.reset(new QueryParam(*queryParam));
		}
		else _queryParam.reset(NULL);
		return res;
	}
	friend class PrivateConstData;
};

class LoadDataWorker: public MJ::Worker {
	public:
		LoadDataWorker(int idx):m_loadIdx(idx) {
			m_ret = false;
		}
	public:
		int doWork() {
			if (m_loadIdx == 1) {
				m_ret = LYConstData::LoadHotelData();
			} else {
				m_ret = LYConstData::LoadRestaurant();
			}
		}
		bool m_ret;
	private:
		int m_loadIdx;
};
#endif //_LYCONSTDATA_H_
