#include "PrivateConstData.h"
#include <algorithm>
#include <cmath>
#include <string.h>
#include <limits>
#include "MJCommon.h"
#include "RouteConfig.h"
#include "TrafficData.h"
#include "LYConstData.h"
#include "ConstDataCheck.h"
#include <fstream>

int PrivateConfig::m_interval;//延迟更新
std::vector<std::string> PrivateConfig::m_monitorList;//监视的表名 格式如 (数据库A#表B)
int PrivateConfig::m_stackSize;//栈大小
const int MAX_VIEW_DIS = 50000;

int PrivateConfig::LoadPrivateConfig(const char* fileName) {
	std::ifstream fin;
	fin.open(fileName);
	if (!fin) {
		std::cerr << "Load PrivateConfig failed " << std::string(fileName) << "!" << std::endl;
		return 1;
	}
	std::string line = "";
	while(!fin.eof()) {
		getline(fin, line);
		if (line.size() == 0 || line[0] == '#')  {
			continue;
		}
		AnalyzeLine(line);
	}
	return 0;
}

int PrivateConfig::AnalyzeLine(const std::string& line){
	int pos = line.find("=");
	if (pos==std::string::npos)
		return 0;
	std::string key = line.substr(0,pos);
	std::string val = line.substr(pos+1);
	if (key == "m_monitorItem") {
		m_monitorList.push_back(val);
	} else if (key == "m_interval") {
		m_interval = atoi(val.c_str());
	} else if (key == "m_stackSize") {
		m_stackSize = atoi(val.c_str());
	}
	return 0;
}

int PrivateConstData::Show() {
	for (auto id2ticketIt = m_id2productTickets.begin(); id2ticketIt != m_id2productTickets.end(); id2ticketIt ++) {
		const TicketsFun* ticketsFun = id2ticketIt->second;
		std::cerr << ticketsFun->m_id << std::endl;
	}

	for (auto it = m_citys.begin(); it != m_citys.end(); ++it) {
		const LYPlace* place = it->second;
		std::cerr << place->_ID << std::endl;
	}
	return 0;
}

int PrivateConstData::Destroy() {
	for (auto it= m_citys.begin(); it != m_citys.end(); it++)
		if(it->second) delete it->second;
	m_citys.clear();

	for (auto it= m_hotels.begin(); it != m_hotels.end(); it++)
		if(it->second) delete it->second;
	m_hotels.clear();

	for (auto it= m_views.begin(); it != m_views.end(); it++)
		if(it->second) delete it->second;
	m_views.clear();
	m_cityViews.clear();

	for (auto it= m_shops.begin(); it != m_shops.end(); it++)
		if(it->second) delete it->second;
	m_shops.clear();
	m_cityShops.clear();

	for (auto it= m_restaurants.begin(); it != m_restaurants.end(); it++)
		if(it->second) delete it->second;
	m_restaurants.clear();
	m_cityRestaurants.clear();

	for (auto it= m_tours.begin(); it != m_tours.end(); it++)
		if(it->second) delete it->second;
	m_tours.clear();
	m_cityTours.clear();

	for (auto it = m_id2productTickets.begin(); it!= m_id2productTickets.end(); it++)
		if(it->second) delete it->second;
	m_id2productTickets.clear();

	m_shields.clear();
	m_sid2sourceName.clear();
	_ptid2tagIds.clear();

	return 0;
}

template<typename T>
int PrivateConstData::CopyAndInsert(const T* place,std::string id){
	if (place == NULL) {
		MJ::PrintInfo::PrintLog("lost my info %s", id.c_str());
		return PRIVATE_ERROR_INFO_LOST;
	}
	MJ::PrintInfo::PrintLog("copy id %s(%s)", place->_ID.c_str(), place->_name.c_str());
	LYPlace* newPlace = new T(*place);
	if (newPlace) {//没有内存块 就直接报错 返回
		if(DoInsert(newPlace)){;
			MJ::PrintInfo::PrintLog("PrivateConstData::CopyData, %s(%s) insert Failed", newPlace->_ID.c_str(),newPlace->_name.c_str());
			delete newPlace;
			newPlace = NULL;
			return PRIVATE_DO_SOMETHING_FAILED;
		}
	} else {
		MJ::PrintInfo::PrintLog("木有内存了");
		return PRIVATE_ERROR_MEMORY;
	}

	return 0;
}

int PrivateConstData::CopyData(const PrivateConstData& priData) {
	int ret=0;
	for (auto it = priData.m_citys.begin(); it != priData.m_citys.end(); it ++) {
		ret = CopyAndInsert(dynamic_cast<const City*>(it->second),it->first);
		if(ret) return ret;
	}

	for (auto it = priData.m_hotels.begin(); it != priData.m_hotels.end(); it ++ ) {
		if(it->second and it->second->_ID.find("coreHotel")!=std::string::npos) continue;
		ret = CopyAndInsert(dynamic_cast<const Hotel*>(it->second),it->first);
		if(ret) return ret;
	}

	for (auto it = priData.m_views.begin(); it != priData.m_views.end(); it++) {
		ret = CopyAndInsert(dynamic_cast<const View*>(it->second),it->first);
		if(ret) return ret;
	}

	for (auto it = priData.m_shops.begin(); it != priData.m_shops.end(); it ++) {
		ret = CopyAndInsert(dynamic_cast<const Shop*>(it->second),it->first);
		if(ret) return ret;
	}

	for (auto it = priData.m_restaurants.begin(); it != priData.m_restaurants.end(); it ++) {
		ret = CopyAndInsert(dynamic_cast<const Restaurant*>(it->second),it->first);
		if(ret) return ret;
	}

	for (auto it = priData.m_tours.begin(); it != priData.m_tours.end(); it ++) {
		if(dynamic_cast<const ViewTicket*>(it->second)){
			ret = CopyAndInsert(dynamic_cast<const ViewTicket*>(it->second),it->first);
			if(ret) return ret;
		}else {
			ret = CopyAndInsert(dynamic_cast<const Tour*>(it->second),it->first);
			if(ret) return ret;
		}
	}

	for (auto id2ticketIt = priData.m_id2productTickets.begin(); id2ticketIt != priData.m_id2productTickets.end(); id2ticketIt ++) {
		int id = id2ticketIt->first;
		const TicketsFun* ticketsFun = id2ticketIt->second;
		if (!ticketsFun) {
			MJ::PrintInfo::PrintLog("lost my info %d", id);
			return PRIVATE_ERROR_INFO_LOST;
		}
		MJ::PrintInfo::PrintLog("copy id %d(%s)", ticketsFun->m_id, ticketsFun->m_name.c_str());
		TicketsFun* tickets = new TicketsFun(*ticketsFun);
		if (tickets) {
			m_id2productTickets[tickets->m_id] = tickets;
		} else {
			MJ::PrintInfo::PrintLog("木有内存了");
			return PRIVATE_ERROR_MEMORY;
		}
	}

	m_shields = priData.m_shields;

	m_sid2sourceName = priData.m_sid2sourceName;
	_ptid2tagIds = priData._ptid2tagIds;

	return 0;
}


int PrivateConstData::DoConnectForMysql(const std::string& host, const std::string& user,
											const std::string& passwd, const std::string& dbName) {
	int ret = 0;
	mysql_init(&m_privateMysql);
	MJ::PrintInfo::PrintLog("PrivateConstDatat, Mysql connect to %s:%s", RouteConfig::private_db_host.c_str(), RouteConfig::private_db_name.c_str());
	if (ret == 0 && !mysql_real_connect(&m_privateMysql, host.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), 0, NULL, 0)){//检测mysql 连接
		MJ::PrintInfo::PrintErr("PrivateConstData, Connect to %s error: %s", dbName.c_str(), mysql_error(&m_privateMysql));
		ret = 1;
	}
	if (ret == 0 && mysql_set_character_set(&m_privateMysql, "utf8")) {//设置字符集
		MJ::PrintInfo::PrintErr("PrivateConstData, Set mysql characterset: %s", mysql_error(&m_privateMysql));
		ret = 1;
	}
	if (ret) {
		CloseMysql();//关sql
		MJ::PrintInfo::PrintLog("mysql 连不上了");
		return 1;
	}
	return 0;
}

int PrivateConstData::CloseMysql() {//配套调用
	mysql_close(&m_privateMysql);
	return 0;
}

int PrivateConstData::Update(const std::string& startT, const std::string& endT, const std::string& dataBase, const std::string& tableName, int* dur, int* rowCnt) {
	int ret = 0;
	ret = DoConnectForMysql(RouteConfig::private_db_host, RouteConfig::private_db_user,
							RouteConfig::private_db_passwd, dataBase);
	if (ret) {
		return 1;
	}

	ret = SwitchUpdate(startT, endT, tableName, dur, rowCnt);

	ShieldDump();

	CloseMysql();

	return ret;
}

int PrivateConstData::SwitchUpdate(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	int ret = 0;
	if (tableName == "attraction" || tableName == "shopping" || tableName == "restaurant") {
		ret = UpdateVarPlaceData(startT, endT, tableName, dur, rowCnt);
	} else if (tableName == "hotel") {
		ret = UpdateHotel(startT, endT, tableName, dur, rowCnt);
	} else if (tableName == "view_ticket" || tableName == "play_ticket" || tableName == "activity_ticket") {
		ret = UpdateTourData(startT, endT, tableName, dur, rowCnt);
	} else if (tableName == "tickets_fun") {
		ret = UpdateTicketsFun(startT, endT, tableName, dur, rowCnt);
	} else if (tableName == "city") {
		ret = UpdateCity(startT, endT, tableName, dur, rowCnt);
	} else if (tableName == "source") {
		ret = LoadPrivateSource();
	} else if (tableName == "tag") {
		ret = LoadTag();
	}

	return ret;
}

LYPlace* PrivateConstData::GetLYPlace(std::string priID,std::string ptid) {
    if (m_citys.count(priID)) {
        return m_citys[priID];
	}
	if (m_tours.count(priID)) {
		return m_tours[priID];
	}
	if(m_hotels.count(priID)){
		return m_hotels[priID];
	} else if (m_views.count(priID)){
		return m_views[priID];
	} else if (m_shops.count(priID)){
		return m_shops[priID];
	} else if (m_restaurants.count(priID)){
		return m_restaurants[priID];
    }
	priID = priID+"_"+ptid;
	if(m_hotels.count(priID)){
		return m_hotels[priID];
	} else if (m_views.count(priID)){
		return m_views[priID];
	} else if (m_shops.count(priID)){
		return m_shops[priID];
	} else if (m_restaurants.count(priID)){
		return m_restaurants[priID];
    }

	return NULL;
}

int PrivateConstData::DelLYPlace(const std::string& priID,std::string ptid) {
	const LYPlace* place = GetLYPlace(priID,ptid);
	if (!place) { //id不存在 无法删除
		return 0;
	}

	DelGridMapPlaceId(place);    //Eden
	DelCityLYPlace(place);

	std::tr1::unordered_set<const LYPlace*> placeList;
	placeList.insert(place);
	if (place->_t & LY_PLACE_TYPE_TOURALL) {
		const Tour* tour = dynamic_cast<const Tour*>(place);
		if (tour) {
			for (auto poiId: tour->m_gatherLocal) {
				auto poi = GetLYPlace(poiId, "");
				if (poi and !placeList.count(poi)) placeList.insert(poi);
			}
			for (auto poiId: tour->m_dismissLocal) {
				auto poi = GetLYPlace(poiId, "");
				if (poi and !placeList.count(poi)) placeList.insert(poi);
			}
		}
	}
	for (auto poi: placeList) {
		if (!poi) continue;
		int ret = DelFromPlaceMap(poi);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int PrivateConstData::DelGridMapPlaceId(const LYPlace* place) { //删除GridMap中Id的映射 Eden
    std::string priID=place->_ID;
    if (place->_t == LY_PLACE_TYPE_VIEW ||place->_t == LY_PLACE_TYPE_SHOP || place->_t == LY_PLACE_TYPE_RESTAURANT) {
		std::string extraLogo = place->m_ptid;
		LYConstData::m_gridMap.delIdFromGrid(priID, extraLogo);
    }
    return 0;
}

int PrivateConstData::DelFromPlaceMap(const LYPlace* place) {
	std::string priID = place->_ID;
	if (m_tours.count(priID)) {
		m_tours.erase(priID);
//		MJ::PrintInfo::PrintLog("del id from id Map %s", priID.c_str());
	} else if (m_citys.count(priID)) {
		m_citys.erase(priID);
		priID = "coreHotelByMakeCid"+priID;
		if(m_hotels.count(priID) and m_hotels[priID]) delete m_hotels[priID];
		if (m_hotels.count(priID)) m_hotels.erase(priID);
	}
	priID = place->getUniqId();
	if (m_hotels.count(priID)) {
		m_hotels.erase(priID);
//		MJ::PrintInfo::PrintLog("del id from id Map %s", priID.c_str());
	} else if (m_views.count(priID)) {
		m_views.erase(priID);
//		MJ::PrintInfo::PrintLog("del id from id Map %s", priID.c_str());
	} else if (m_shops.count(priID)) {
		m_shops.erase(priID);
//		MJ::PrintInfo::PrintLog("del id from id Map %s", priID.c_str());
	} else if (m_restaurants.count(priID)) {
		m_restaurants.erase(priID);
//		MJ::PrintInfo::PrintLog("del id from id Map %s", priID.c_str());
	}
	if(place) delete place;

	return 0;
}

int PrivateConstData::DelCityLYPlace(const LYPlace* place) {
	for (int i = 0 ; i < place->_cid_list.size(); i++) {
		std::string cid = place->_cid_list[i];
		if (place->_t == LY_PLACE_TYPE_VIEW) {
			m_cityViews[place->m_ptid][cid].erase(place->_ID);
		} else if (place->_t == LY_PLACE_TYPE_SHOP) {
			m_cityShops[place->m_ptid][cid].erase(place->_ID);
		} else if (place->_t == LY_PLACE_TYPE_RESTAURANT) {
			m_cityRestaurants[place->m_ptid][cid].erase(place->_ID);
		} else if (place->_t & LY_PLACE_TYPE_TOURALL) {
			m_cityTours[place->m_ptid][cid].erase(place->_ID);
		}
	}

	return 0;
}

int PrivateConstData::DoInsert(LYPlace* place) {
	if (place == NULL or place->_ID.empty()) {
		return 1;
	}

	int ret = InsertLYPlace(place);
	if (ret) {
		return ret;
	}

	ret = InsertCityPlaceList(place);
	if (ret) {
		return ret;
	}

	return 0;
}


int PrivateConstData::InsertLYPlace(LYPlace* place) {
	std::string id = place->getUniqId();
	if (place->_t & LY_PLACE_TYPE_CITY) {
		if (m_citys.count(id)) {
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
		m_citys[id]=place;
		InsertCoreHotel(place);
		MJ::PrintInfo::PrintLog("PrivateConstData::InsertLYPlace, insert city id is %s, m_citys length is %d", place->_ID.c_str(), m_citys.size());
		return 0;
	}else if (place->_t & LY_PLACE_TYPE_TOURALL) { //LY_PLACE_TYPE_VIEWTICKET is LY_PLACE_TYPE_TOUR
		if(m_tours.count(id)){
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
		m_tours[id] = place;
		return 0;
	}

	if (place->_t & LY_PLACE_TYPE_HOTEL) {
		if(not m_hotels.count(id)){
			m_hotels[id] = place;
		}else{
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
	} else if (place->_t & LY_PLACE_TYPE_VIEW) {
		if(not m_views.count(id)){
			m_views[id] = place;
		}else{
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
	} else if (place->_t & LY_PLACE_TYPE_SHOP) {
		if(not m_shops.count(id)){
			m_shops[id] = place;
		}else{
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
	} else if (place->_t & LY_PLACE_TYPE_RESTAURANT) {
		if(not m_restaurants.count(id)){
			m_restaurants[id] = place;
		}else{
			MJ::PrintInfo::PrintLog("%s has already insert!", id.c_str());
			return 1;
		}
	} else {
		MJ::PrintInfo::PrintLog("%s type error!", place->_ID.c_str());
		return 1;
	}

	return 0;
}

int PrivateConstData::InsertCityPlaceList(const LYPlace* place) {
	for (int i = 0 ; i < place->_cid_list.size(); i ++) {//遍历城市
		std::string cid = place->_cid_list[i];
		if (place->_t == LY_PLACE_TYPE_VIEW) {
			m_cityViews[place->m_ptid][cid].insert(place->_ID);
		} else if(place->_t == LY_PLACE_TYPE_SHOP) {
			m_cityShops[place->m_ptid][cid].insert(place->_ID);
		} else if (place->_t == LY_PLACE_TYPE_RESTAURANT) {
			m_cityRestaurants[place->m_ptid][cid].insert(place->_ID);
		} else if (place->_t & LY_PLACE_TYPE_TOURALL) {
			m_cityTours[place->m_ptid][cid].insert(place->_ID);
		}
	}

	return 0;
}

int PrivateConstData::Insert2Shield(int type, const std::string& id, const std::string& ptid) {
	if(not ptid.empty()) m_shields[ptid].insert(id);

	return 0;
}

int PrivateConstData::FillInfo(LYPlace* place) {
	if(place->_t &(LY_PLACE_TYPE_VIEW|LY_PLACE_TYPE_SHOP|LY_PLACE_TYPE_RESTAURANT))
	{
		VarPlace * vPlace = dynamic_cast<VarPlace*>(place);
		if(vPlace)
		{
			const LYPlace* placeConst = LYConstData::getBaseLYPlace(place->_ID);//查找共有库的点
			if (placeConst) {//找到共有库的点
				const VarPlace* vPlaceConst = dynamic_cast<const VarPlace*>(placeConst);
				if (vPlaceConst) {//转换共有库的点 转换失败报错
					FillInfoByConstData(vPlaceConst, vPlace);
					return 0;
				}
			}
			FillVarPlaceInfo(vPlace);
			//MJ::PrintInfo::PrintLog("use defalult info");
		}
	}
	return 0;
}

int PrivateConstData::FillInfoByConstData(const VarPlace* vPlaceConst, VarPlace* vPlace) {
	vPlace->_hot_level = vPlaceConst->_hot_level;
	vPlace->m_tagSmallStr = vPlaceConst->m_tagSmallStr;
	vPlace->_time_rule = vPlaceConst->_time_rule;
	vPlace->_price = vPlaceConst->_price;
	vPlace->_level = vPlaceConst->_level;
	vPlace->_grade = vPlaceConst->_grade;
	vPlace->_ranking = vPlaceConst->_ranking;
	if(vPlace->_intensity_list.size()==0)
	{
		for (int i = 0 ; i < vPlaceConst->_intensity_list.size(); i ++) {
			vPlace->_intensity_list.push_back(new Intensity(*vPlaceConst->_intensity_list[i]));
		}
	}
	if(vPlace->_rcmd_intensity==NULL)
	{
		if (vPlaceConst->_rcmd_intensity) {
			vPlace->_rcmd_intensity = new Intensity(*vPlaceConst->_rcmd_intensity);
		}
	}
	return 0;
}

int PrivateConstData::FillVarPlaceInfo(VarPlace* vPlace) {
	vPlace->_hot_level = 1;
	vPlace->_time_rule =  "<*><*><00:00-23:59><SURE>";
	vPlace->_price ="<*><*><成人:0-人民币><SURE>";
	vPlace->_grade = 6.0;
	vPlace->_ranking = 999999;
	vPlace->_intensity_list.push_back(new Intensity("1-1", "拍照就走", 900, -1, true, "CNY"));
	vPlace->_rcmd_intensity = new Intensity("2-1", "简单游玩", 3600, -1, false, "CNY");
	vPlace->_intensity_list.push_back(vPlace->_rcmd_intensity);
	vPlace->_intensity_list.push_back(new Intensity("3-1", "深度游玩", 10800, -1, false, "CNY"));
	return 0;
}

// update private city
int PrivateConstData::UpdateCity(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	MJ::PrintInfo::PrintLog("PrivateConstData::UpdateCity, update Private City Data...");
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	std::string sql = "select id,name,map_info,time_zone,name_en,disable,ptid from " + tableName + " where utime >= '" + startT + "' and utime <= '" + endT + "'";
	if(startT.empty() and endT.empty()) sql = "select id,name,map_info,time_zone,name_en,disable,ptid from city";
	enum {id, name, map_info, time_zone, name_en, disable, ptid};
    ret = mysql_query(&m_privateMysql, sql.c_str());
    if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::UpdateCity, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
        return 1;
    } else {
        MYSQL_RES* res = mysql_use_result(&m_privateMysql);
        MYSQL_ROW row;
        if (res) {
        	int num = 0;
			MJ::PrintInfo::PrintLog("PrivateConstData::UpdateCity before UpdateCity, City Num: %d", m_citys.size());
            while (row = mysql_fetch_row(res)) {
                int delPoi = row[disable] ? atoi(row[disable]) : 0;
				if (delPoi == 1) {
					continue;
				}
                if (row[id] == NULL or std::string(row[id]).empty()) continue;
				if (row[ptid] == NULL or std::string(row[ptid]).empty()) continue;
				std::string cid=row[id],_ptid=row[ptid];
				DelLYPlace(cid,_ptid);//单块内存更新 比较麻烦 直接删了重来
                City* city = new City();
				city->m_custom = POI_CUSTOM_MODE_PRIVATE;//代表私有库字段 后续会加类型~
				city->_t = LY_PLACE_TYPE_CITY;
				city->_ID = row[id];
                city->m_ptid = row[ptid];
				city->_name = row[name] ? row[name] : "";
                city->_enname = row[name_en] ? row[name_en] : "";
				city->_poi = row[map_info] ? row[map_info] : "";
				city->_time_zone = row[time_zone] ? atoi(row[time_zone]) : -100;
				city->_cid_list = std::vector<std::string>(1, city->_ID);
                city->_country = "";
                city->_hot = 1;

                ret = DoInsert(city);
                if (ret == 0) ++num;
				else delete city;
            }
			MJ::PrintInfo::PrintLog("PrivateConstData::UpdateCity City Num: %d", num);
			MJ::PrintInfo::PrintLog("PrivateConstData::UpdateCity after UpdateCity, City total Num: %d", m_citys.size());
			*rowCnt = num;
        }
		mysql_free_result(res);
    }
	*dur = t.cost();
	return 0;
}

int PrivateConstData::PrivateLoad() {
	MJ::PrintInfo::PrintLog("PrivateConstData::PrivateLoad start");
	int ret = DoConnectForMysql(RouteConfig::private_db_host, RouteConfig::private_db_user,
							RouteConfig::private_db_passwd, RouteConfig::private_db_name);
	if (ret) {
		return 1;
	}

	//加载数据
	ret = LoadPrivateData();

	CloseMysql();

	return ret;
}

int PrivateConstData::LoadPrivateData() {
	int ret = 0;
    //加载城市数据
    ret = LoadPrivateCityData();
    if (ret ) return 1;
	//加载酒店基础数据
	ret = LoadPrivateHotelData();
	if (ret) return 1;
	ret = LoadTag();
	if (ret) return 1;
	//;加载景点基础数据
	//加载购物基础数据
	//加载餐馆基础数据
	ret = LoadVarPlaceData();
	if (ret) return 1;
	//加载玩乐
	ret = LoadTourData();
	if (ret) return 1;
	//加载ticketsfun
	ret = LoadTicketsFun();
	if (ret) return 1;
	ret = LoadPrivateSource();
	if (ret) return 1;

	return 0;
}

int PrivateConstData::LoadVarPlaceData() {
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadVarPlaceData...");
	std::map<std::string, std::pair<std::string, int>> loadParams;
	//view,restaurant,shop
	//[MYSQL]语句
	std::string sql = "select real_id,name,name_en,map_info,rcmd_visit_time,city_id,city,ptid,refer, disable, tag, rec_priority,utime from attraction";
	loadParams["attraction"] = std::make_pair(sql, LY_PLACE_TYPE_VIEW);

	sql = "select real_id,name,name_en,map_info,rcmd_visit_time,city_id,city,ptid,refer,disable, tag,rec_priority,utime from shopping";
	loadParams["shop"] = std::make_pair(sql, LY_PLACE_TYPE_SHOP);

	sql = "select real_id,name,name_en,map_info,0 as rcmd_visit_time,city_id,city,ptid,refer,disable,tag,rec_priority,utime from restaurant";
	loadParams["restaurant"] = std::make_pair(sql, LY_PLACE_TYPE_RESTAURANT);

	for(auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		int dur = 0;
		int rowCnt = 0;
		if (LoadSpecifiedTypeVarPlace(it->first, it->second.first, it->second.second, &dur, &rowCnt) == 1) {
			return 1;
		}
	}
	return 0;
}
int PrivateConstData::UpdateVarPlaceData(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	MJ::PrintInfo::PrintLog("PrivateConstData::UpdateVarPlaceData...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	if (tableName == "attraction") {
		std::string sql = "select real_id,name,name_en,map_info,rcmd_visit_time,city_id,city,ptid,refer,disable, tag, rec_priority, utime from " + tableName + " where utime >= '" + startT + "' and utime <= '" + endT + "'";
		loadParams[tableName] = std::make_pair(sql, LY_PLACE_TYPE_VIEW);
	}
	else if (tableName == "shopping") {
		std::string sql = "select real_id,name,name_en,map_info,rcmd_visit_time,city_id,city,ptid,refer,disable, tag, rec_priority, utime from " + tableName + " where utime >= '" + startT + "' and utime <= '" + endT + "'";
		loadParams[tableName] = std::make_pair(sql, LY_PLACE_TYPE_SHOP);
	}
	else if (tableName == "restaurant") {
		std::string sql = "select real_id,name,name_en,map_info,0 as rcmd_visit_time,city_id,city,ptid,refer,disable, tag, rec_priority, utime from " + tableName + " where utime >= '" + startT + "' and utime <= '" + endT + "'";
		loadParams[tableName] = std::make_pair(sql, LY_PLACE_TYPE_RESTAURANT);
	}

	for(auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		if (LoadSpecifiedTypeVarPlace(it->first, it->second.first, it->second.second, dur, rowCnt) == 1) {
			_INFO("hot update err... %s, sql: %s",it->first.c_str(), it->second.first);
			return 1;
		}
	}
	return 0;
}
int PrivateConstData::LoadSpecifiedTypeVarPlace(std::string typeV, std::string sql, int typeI, int* dur, int* rowCnt) {//先加载基础信息 后加入其他逻辑补全
	/*
	 * 0 real_id
	 * 1 name
	 * 2 name_en
	 * 3 map_info 坐标
	 * 4 rcmd_visit_time 推荐游玩时长
	 * 5 city_id 所属 的 城市id
	 * 6 city 城市
	 * 7 ptid 私有库的来源
	 * 8 refer 私有库引用的共有库的id
	 * 9 disable 包含 id 以及 refer 的 生效选项
	 * 10 tag 
	 * 11 rec_priority
	 * utime
	 */
	enum {real_id, name, name_en, map_info, rcmd_visit_time, city_id, city, ptid, refer, disable, tag, rec_priority, utime};
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeVarPlace, loading Private %s Data...", typeV.c_str());
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeVarPlace, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[real_id] == NULL || std::string(row[real_id]).empty()) continue;
				if (row[ptid] == NULL || std::string(row[ptid]).empty()) continue;
				std::string vid = row[real_id],_ptid=row[ptid];
				DelLYPlace(vid,_ptid);
				int delPoi = row[disable] ? atoi(row[disable]) : 0;
				std::string refer_poi = row[refer] ? row[refer] : "";
				if (not refer_poi.empty() and delPoi == 1) {
					Insert2Shield(typeI, refer_poi, _ptid);
				}
				if(not refer_poi.empty() and LYConstData::getBaseLYPlace(refer_poi)==NULL
						or delPoi == 1)
				{
					continue;
				}
				VarPlace* varPlace = NULL;
				if (typeI == LY_PLACE_TYPE_RESTAURANT) {
					varPlace = new Restaurant;
				}
				else if (typeI == LY_PLACE_TYPE_SHOP) {
					varPlace = new Shop;
				}
				else if (typeI == LY_PLACE_TYPE_VIEW) {
					varPlace = new View;
				}
				else continue;
				varPlace->m_custom = POI_CUSTOM_MODE_PRIVATE;//代表私有库字段 后续会加类型~
				varPlace->_t = typeI;
				varPlace->_ID = row[real_id];
				varPlace->_name = row[name] ? row[name] : "";
				varPlace->_enname = row[name_en] ? row[name_en] : "";
				varPlace->_poi = row[map_info] ? row[map_info] : "";//坐标~
				varPlace->m_ptid = row[ptid]? row[ptid] : "";
				varPlace->m_refer = refer_poi;
				if (row[map_info]) {
					std::string extraLogo = row[ptid];
                    bool ret=LYConstData::m_gridMap.addId2Grid(row[real_id], extraLogo, row[map_info]);
                    if(!ret) {
						std::cerr<<"GridMap error"<<std::endl;
						delete varPlace;
						varPlace = NULL;
						continue;
					}
                }
				varPlace->_utime = row[utime] ? row[utime] : ""; //更新时间
				if (varPlace->_t != LY_PLACE_TYPE_RESTAURANT) {
					int rcmd_dur = row[rcmd_visit_time] ? (atoi(row[rcmd_visit_time]) >= 900 ? atoi(row[rcmd_visit_time]) : 3600) : 3600;
					varPlace->_intensity_list.push_back(new Intensity("2-1", "简单游玩", rcmd_dur, -1, false, "CNY"));
					varPlace->_rcmd_intensity = varPlace->_intensity_list[0];
				}
				//城市id的集合 空的话 不允许
				if (row[city_id] && strlen(row[city_id]) > 0) {
					ToolFunc::sepString(row[city_id], "|", varPlace->_cid_list, "");
				}
				if (row[tag]) {
					std::vector<std::string> tagList;
					ToolFunc::sepString(row[tag], "|", tagList);
					for (int i = 0; i < tagList.size(); i++) {
						std::string tagIdStr = tagList[i];
						if (LYConstData::_baseTagIds.count(tagIdStr) == 0 and _ptid2tagIds[row[ptid]].count(tagIdStr) == 0) continue;
						varPlace->m_tag.insert(tagIdStr);
					}
				}
				varPlace->_rec_priority = row[rec_priority] ? atoi(row[rec_priority]) : 0;
				FillInfo(varPlace);
				ret = DoInsert(varPlace);
				if (ret) {
					delete varPlace;
					varPlace = NULL;
					continue;
					//return 1;
				}
				++num;
			}
			MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeVarPlace, Load %s Num: %d",typeV.c_str(), num);
			*rowCnt = num;
		}
		mysql_free_result(res);
	}
	*dur = t.cost();
	return 0;
}

int PrivateConstData::UpdateTourData(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	MJ::PrintInfo::PrintLog("PrivateConstData::UpdateTourData...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	std::string sql = "select pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id, map_info,first_img,disable,sid, tag from " + tableName + " where utime >= '" +startT + "' and utime <= '" + endT + "'";
	if (tableName == "view_ticket") {
		std::string vt_sql = "select pid,ptid,poi_mode,ref_poi,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id, '' as map_info,first_img,disable,sid, tag from " + tableName + " where utime >= '" +startT + "' and utime <= '" + endT + "'";
		loadParams[tableName] = std::make_pair(vt_sql, LY_PLACE_TYPE_VIEWTICKET);
	}
	else if (tableName == "play_ticket") {
		loadParams[tableName] = std::make_pair(sql, LY_PLACE_TYPE_PLAY);
	}
	else if (tableName == "activity_ticket"){
		loadParams[tableName] = std::make_pair(sql, LY_PLACE_TYPE_ACTIVITY);
	}

	for (auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		if (LoadSpecifiedTypeTour(it->first, it->second.first, it->second.second, dur, rowCnt) == 1) {
			_INFO("hot update err... %s, sql: %s",it->first.c_str(), it->second.first);
			continue;
			//return 1;
		}
	}
	return 0;
}
int PrivateConstData::LoadTourData() {
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadTourData...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	std::string sql = "select pid,ptid,poi_mode,ref_poi,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id, '' as map_info,first_img,disable,sid, tag from view_ticket where disable = 0;";
	loadParams["view_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_VIEWTICKET);

	sql = "select pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,map_info,first_img,disable,sid, tag from play_ticket where disable = 0;";
	loadParams["play_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_PLAY);

	sql = "select pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,map_info,first_img,disable,sid, tag from activity_ticket where disable = 0;";
	loadParams["activity_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_ACTIVITY);

	for (auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		int dur = 0;
		int rowCnt = 0;
		if (LoadSpecifiedTypeTour(it->first, it->second.first, it->second.second, &dur, &rowCnt) == 1) {
			//return 1;
			continue;
		}
	}
	return 0;
}
int PrivateConstData::LoadSpecifiedTypeTour(std::string typeT, std::string sql, int typeI, int* tdur, int* rowCnt) {
	/*
	 * 0 pid
	 * 1 ptid
	 * 2 poi_mode
	 * 3 ref_poi
	 * 4 name
	 * 5 ename
	 * 6 open
	 * 7 times json 场次相关
	 * 8 book_pre
	 * 9 enter_pre
	 * 10 jiesong_type
	 * 11 jiesong_poi
	 * 12 city_id
	 * 13 map_info
	 * 14 first_img
	 * 15 disable
	 * sid
	 * tag
	 */
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTour, loading Private %s Data...", typeT.c_str());
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTour, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		enum {pid, ptid, poi_mode, ref_poi, name, ename, open, times, book_pre, enter_pre, jiesong_type, jiesong_poi, city_id, map_info, first_img, disable, sid, tag};
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[pid] == NULL or std::string(row[pid]).empty()) continue;
				if (row[ptid] == NULL or std::string(row[ptid]).empty()) continue;
				if (row[ref_poi] == NULL) continue;
				std::string id = row[pid],_ptid=row[ptid];
				DelLYPlace(id,_ptid);//单块内存更新 比较麻烦 直接删了重来
				int delPoi = row[disable] ? atoi(row[disable]) : 0;
				if (delPoi == 1) {
					continue;
				}
				Tour* tour = NULL;
				const LYPlace *vPlace = NULL;
				if (typeI != LY_PLACE_TYPE_VIEWTICKET) {
					tour = new Tour;
					//写入关联景点
					Json::Value value;
					Json::Reader jr(Json::Features::strictMode());
					jr.parse(row[ref_poi], value);
					if(value.isArray() && value.size() > 0) {
						for (int i = 0; i < value.size(); ++i) {
							if (GetLYPlace(value[i].asString(), row[ptid]) or LYConstData::GetLYPlace(value[i].asString(), row[ptid])) {
								tour->m_refPoi.insert(value[i].asString());
							}
						}
					}
				}
				else {
					ViewTicket * vt = new ViewTicket;
					std::string refer_id = row[ref_poi];
					vPlace = GetLYPlace(refer_id, row[ptid]);
					if (vPlace == NULL) vPlace = LYConstData::GetLYPlace(refer_id, row[ptid]);
					if(vPlace){
						vt->Setm_view(row[ref_poi]);
						tour = vt;
						vt = NULL;
					}else{
						std::cerr << "id to place error:" << row[ref_poi] << " - "<< row[ptid] << std::endl;
						delete vt;
						continue;
					}
				}

				tour->m_custom = POI_CUSTOM_MODE_PRIVATE;//代表私有库字段 后续会加类型~
				tour->_t = atoi(row[poi_mode]);
				tour->_ID = row[pid];
				tour->m_pid = row[pid];
				tour->m_ptid = row[ptid];
				tour->_name = row[name];
				tour->_enname = row[ename];
				if (row[sid]) tour->m_sid = row[sid];
				if (row[map_info] && std::string(row[map_info])!="") {
					tour->_poi = row[map_info];
				}
				else if (vPlace) {
					tour->_poi = vPlace->_poi;
				}
				else {
					delete tour;
					tour = NULL;
					continue;
				}
				tour->_cid_list.push_back(row[city_id]);
				if (row[jiesong_type]) tour->m_jiesongType = atoi(row[jiesong_type]);
				Json::Reader jr;
				if (row[jiesong_poi]) {
					tour->m_srcJieSongPOI = row[jiesong_poi];
					Json::Value poi;
					jr.parse(row[jiesong_poi], poi);
					if (poi.isArray() and poi.size() > 0) {
						for (int i = 0; i < poi.size(); i++) {
							Json::Value jPoi = poi[i];
							if (jPoi.isMember("coord") and jPoi["coord"].isString()) {
								std::string gatherId = tour->_ID + "#gather#" + std::to_string(i);
								std::string gatherName = tour->_name + "#" + "gather";
								std::string gatherEname = tour->_enname + "#" + "gather";
								std::string gatherCoord = jPoi["coord"].asString();
								LYPlace* place = LYConstData::MakeView(gatherId, gatherName, gatherEname, gatherCoord, POI_CUSTOM_MODE_MAKE);
								DoInsert(place);
								tour->m_gatherLocal.insert(gatherId);
								tour->m_dismissLocal.insert(gatherId);
							}
						}
					}
				}
				tour->m_srcTimes = Json::Value();
				tour->m_preBook = atoi(row[book_pre]);
				tour->m_preTime = atoi(row[enter_pre]);
				tour->_img = row[first_img];

				tour->m_open = Json::arrayValue;
				Json::Reader jReader;
				Json::Value jValue = Json::arrayValue;
				if (row[open]) jReader.parse(row[open], jValue);
				if (jValue.isArray() && jValue.size() > 0) {
					tour->m_open = jValue;
				}

				Json::Value value;
				jr.parse(row[times], value);
				int dur = 1800;
				if (value.isArray() && value.size() > 0) {
					tour->m_srcTimes = value;
					if (value[0].isMember("dur") && value[0]["dur"].isInt()) {
						dur = value[0]["dur"].asInt();
					}
					if (dur == -1) {
						dur = 3600*4;
					}
					if (dur == -2) {
						dur = 3600*8;
					}
					if (dur == -3) {
						dur = 3600*2;
					}
				}

				tour->_intensity_list.push_back(new Intensity("2-1", "简单游玩", dur, -1, false, "CNY"));
				tour->_rcmd_intensity = tour->_intensity_list[0];
				if (row[tag]) {
					std::vector<std::string> tagList;
					ToolFunc::sepString(row[tag], "|", tagList);
					for (int i = 0; i < tagList.size(); i++) {
						std::string tagIdStr = tagList[i];
						if (LYConstData::_baseTagIds.count(tagIdStr) == 0 and _ptid2tagIds[row[ptid]].count(tagIdStr) == 0) continue;
						tour->m_tag.insert(tagIdStr);
					}
				}
				ret = DoInsert(tour);
				if (ret) {
					_INFO("load %s error", row[pid]);
					delete tour;
					tour = NULL;
					continue;
				}
				++num;
			}
			MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTour, Load %s Num: %d", typeT.c_str(), num);
			*rowCnt = num;
		}
		mysql_free_result(res);
	}
	*tdur = t.cost();
	return 0;
}

int PrivateConstData::LoadPrivateHotelData() {
	int dur = 0,rowCnt = 0;
	return UpdateHotel(std::string(),std::string(),std::string(),&dur,&rowCnt);
}

int PrivateConstData::UpdateHotel(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	MJ::MyTimer t;
	t.start();
	MJ::PrintInfo::PrintLog("PrivateConstData::UpdateHotel, loading Hotel Data...");
	enum {real_id, hotel_name, hotel_name_en, map_info, star, city_mid, rec_priority, refer, disable, ptid};
	std::string sql = "select real_id,hotel_name,hotel_name_en,map_info,star,city_mid,rec_priority,refer,disable,ptid from " + tableName + " where utime >= '" +startT + "' and utime <= '" + endT + "'";
	if(startT.empty() and endT.empty()) sql = "select real_id,hotel_name,hotel_name_en,map_info,star,city_mid,rec_priority,refer,disable,ptid from hotel";
	int ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret != 0) {
		MJ::PrintInfo::PrintLog("PrivateConstData::UpdateHotel, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[real_id] == NULL || std::string(row[real_id]).empty()) continue;
				if (row[ptid] == NULL || std::string(row[ptid]).empty()) continue;
				std::string id = row[real_id],_ptid=row[ptid];
				DelLYPlace(id,_ptid);
				int delPoi = row[disable] ? atoi(row[disable]) : 0;
				std::string refer_poi = row[refer] ? row[refer] : "";
				if(not refer_poi.empty() and LYConstData::getBaseLYPlace(refer_poi)==NULL
						or delPoi == 1)
				{
					continue;
				}
				Hotel* h = new Hotel;
				h->m_custom = POI_CUSTOM_MODE_PRIVATE;
				h->_t = LY_PLACE_TYPE_HOTEL;
				h->_ID = id;
				h->m_refer = refer_poi;
				h->m_ptid = row[ptid];
				if (row[hotel_name]) h->_name = row[hotel_name];
				h->_enname = row[hotel_name_en] ? row[hotel_name_en] : "";
				if (row[map_info]) h->_poi = row[map_info];
				if (row[star]) h->_lvl = atoi(row[star]);
				std::string cids = row[city_mid] ? row[city_mid] : "";
				ToolFunc::sepString(cids, "|", h->_cid_list, "");
				ToolFunc::UniqListOrder(h->_cid_list);
				if (h->_cid_list.size() != 0) {
					const City *c = LYConstData::getCity(h->_cid_list[0]);
					if (c) {
						h->_time_zone = c->_time_zone;
					} else {
						h->_time_zone = 8;
					}
				}
				if (row[rec_priority]) h->_rec_priority = row[rec_priority] ? atoi(row[rec_priority]) : 0;
				h->_tags = "";
				h->_viewDis = MAX_VIEW_DIS;
				ret = DoInsert(h);
				if (ret) {
					delete h;
					h = NULL;
					continue;
				}
				num++;
			}
			*rowCnt = num;
			MJ::PrintInfo::PrintLog("PrivateConstData::UpdateHotel, Load Hotel Num: %d", num);
		}
		mysql_free_result(res);
	}
	*dur = t.cost();
	return 0;
}
int PrivateConstData::UpdateTicketsFun(const std::string& startT, const std::string& endT, const std::string& tableName, int* dur, int* rowCnt) {
	std::string sql = "select id,ticket_id,pid,name,info,ccy,date_price, disable,sid from " + tableName + " where utime >= '" +startT + "' and utime <= '" + endT + "'";
	int ret = LoadSpecifiedTypeTicketsFun(sql, dur, rowCnt);
	return ret;
}
int PrivateConstData::LoadTicketsFun() {
	std::string sql = "select id,ticket_id,pid,name,info,ccy,date_price, disable,sid from tickets_fun where disable = 0;";
	int dur = 0;
	int rowCnt = 0;
	int ret = LoadSpecifiedTypeTicketsFun(sql, &dur, &rowCnt);
	return ret;
}

int PrivateConstData::LoadSpecifiedTypeTicketsFun(std::string sql, int* dur, int* rowCnt) {//先加载基础信息 后加入其他逻辑补全
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTicketsFun, update Private TicketsFun Data...");
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	enum {id,ticket_id, pid, name, info, ccy, date_price, disable, sid};
	/*
	 *  id
	 *  pid	门票产品ID
	 *  name	票务名称
	 *  info	票务信息
	 *  ccy	币种
	 *  date_price	价格
	 *  disable
	 *  sid 供应商
	 */

	ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTicketsFun, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int tnum = 0;
			Json::Reader jr;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				if (row[ticket_id] == NULL) continue;
				if (m_id2productTickets.count(atoi(row[id]))) {
					int tid = atoi(row[id]);
					if(m_id2productTickets[tid]) delete m_id2productTickets[tid];
					m_id2productTickets.erase(tid);
				}
				int delPoi = row[disable] ? atoi(row[disable]) : 0;
				if (delPoi == 1) {
					continue;
				}
				TicketsFun* ticketsFun = new TicketsFun;
				ticketsFun->m_id = atoi(row[id]);
				ticketsFun->m_ticket_id = row[ticket_id];
				ticketsFun->m_pid = row[pid];
				ticketsFun->m_name = row[name];
				ticketsFun->m_ccy = row[ccy];
				ticketsFun->m_sid = row[sid];
				Json::Value jv;
				jr.parse(row[info], jv);
				int num = 0;
				if (jv["info"].isArray()) {
					for (int i = 0; i < jv["info"].size(); ++i) {
						if (jv["info"][i].isObject() && jv["info"][i]["num"].isInt()) {
							num += jv["info"][i]["num"].asInt();
						}
					}
				}

				if (row[date_price]) {
					Json::Value jv = Json::arrayValue;
					Json::Reader jr(Json::Features::strictMode());
					jr.parse(row[date_price], jv);
					if (jv.isArray()) ticketsFun->m_date_price = jv;
				}

				ticketsFun->m_userNum = num;
				ticketsFun->m_info = jv;
				Json::Value value;
				Json::Reader jr(Json::Features::strictMode());
				jr.parse(row[info], value);
				ticketsFun->m_info = value;
				//tickets信息更新
				m_id2productTickets[atoi(row[id])] = ticketsFun;

				++tnum;
			}
			MJ::PrintInfo::PrintLog("PrivateConstData::LoadSpecifiedTypeTicketsFun, Load TicketsFun Num: %d", tnum);
			*rowCnt = tnum;
		}
		mysql_free_result(res);
	}
	*dur = t.cost();
	return 0;
}

int PrivateConstData::LoadPrivateSource() {
	MJ::PrintInfo::PrintLog("PrivateConstData::LoadPrivateSource, loading Private Source Data...");
	std::string sql = "select sid,descr from source;";
	/*
	 * 0 sid	sourceID
	 * 1 descr	sourceName
	 */
	int ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::LoadPrivateSource, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		m_sid2sourceName.clear();
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[0] == NULL or std::string(row[0]).empty()) continue;
				std::string sid = row[0];
				std::string sname = row[1];
				m_sid2sourceName[sid] = sname;
				++num;
			}
			MJ::PrintInfo::PrintLog("PrivateConstData::loadPrivateSource, Load source Num: %d", num);
		}
		mysql_free_result(res);
	}
	return 0;
}

//私有库城市
int PrivateConstData::LoadPrivateCityData() {//先加载基础信息 后加入其他逻辑补全
	int dur = 0,rowCnt = 0;
	return UpdateCity(std::string(),std::string(),std::string(),&dur,&rowCnt);
}


bool PrivateConstData::IsShieldId(const std::string& ptid, const std::string& Id) {
	if(not ptid.empty() and m_shields[ptid].count(Id)) return true;

	return false;
}

int PrivateConstData::GetCityViewPrivate(const std::string& cityID, std::set<std::string>& viewList,std::string ptid) {
	viewList.insert(m_cityViews[ptid][cityID].begin(), m_cityViews[ptid][cityID].end());
	return 0;
}

int PrivateConstData::GetCityShopPrivate(const std::string& cityID, std::set<std::string>& shopList,std::string ptid) {
	shopList.insert(m_cityShops[ptid][cityID].begin(), m_cityShops[ptid][cityID].end());
	return 0;
}

int PrivateConstData::GetCityRestaurantPrivate(const std::string& cityID, std::set<std::string>& restaurantList,std::string ptid) {
	restaurantList.insert(m_cityRestaurants[ptid][cityID].begin(), m_cityRestaurants[ptid][cityID].end());
	return 0;
}

int PrivateConstData::GetPrivateTourByType(const std::string &cityID, std::set<std::string>& TourList, int type, std::string ptid) {
	auto it = m_cityTours[ptid].find(cityID);
	if (it == m_cityTours[ptid].end()) {
		MJ::PrintInfo::PrintErr("PrivateConstData::getPrivateTourByType, can not find ID: %s", cityID.c_str());
		return 1;
	}
	auto tourList = it->second;
	for (auto _it = tourList.begin(); _it != tourList.end(); _it++) {
		std::string id= *_it;
		const LYPlace* place = GetLYPlace(id,ptid);
		if (place->_t & type) {
			TourList.insert(id);
		}
	}

	return 0;
}

int PrivateConstData::ShieldPlaceList(std::set<std::string>& placeList, const std::string& ptid) {
	ShieldDump();
	std::set<std::string> tmp;
	for (auto it = placeList.begin(); it != placeList.end(); it++) {
		if (not IsShieldId(ptid, *it)) {//隐藏
			//			MJ::PrintInfo::PrintLog("PrivateConstData::ShieldPlaceList, ptid %s -> id = %s shield ", ptid.c_str(), (*it)->_ID.c_str());
			tmp.insert(*it);
		}
	}
	placeList.swap(tmp);

	return 0;
}

int PrivateConstData::ShieldDump() {
	for(auto it = m_shields.begin(); it!=m_shields.end(); it++)
		for(auto _it = it->second.begin(); _it!=it->second.end(); _it++)
	//		std::cerr << "		shield list" <<  (*_it) << std::endl;

	return 0;
}


int PrivateConstData::GetPrivateData(const std::string &sql, std::vector<std::vector<std::string> > &results) {
	int ret = 0;
	ret = DoConnectForMysql(RouteConfig::private_db_host, RouteConfig::private_db_user,
							RouteConfig::private_db_passwd, RouteConfig::private_db_name);
	if (ret) {
		return 1;
	}
	ret = mysql_query(&m_privateMysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("PrivateConstData::GetPrivateData, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				std::vector<std::string> result;
				for (int i = 0; i < res->field_count; ++i) {
					if (row[i]) {
						result.push_back(row[i]);
					} else {
						result.push_back("");
					}
				}
				results.push_back(result);
				num ++;
			}
			_INFO("load tickets num: %d, results.size(): %d", num, results.size());
		}
		mysql_free_result(res);
	}

	return 0;
}


int PrivateConstData::GetProdTicketsById(const int& id, const TicketsFun*& ticketsFun) {
	if (m_id2productTickets.count(id)) {
		ticketsFun = m_id2productTickets[id];
		return 0;
	}
	return 1;
}

int PrivateConstData::GetProdTicketsListByPid(const std::string& pid, std::vector<const TicketsFun*>& ticketsList) {
	std::tr1::unordered_map<int, TicketsFun*>::const_iterator cit;
	for(cit = m_id2productTickets.begin(); cit != m_id2productTickets.end(); cit++) {
		if (cit->second == NULL) {
			MJ::PrintInfo::PrintErr("PrivateConstData::GetProdTicketsListByPid, no product ticket, ticket id: %s", cit->first);
			continue;
		}
		if(cit->second->m_pid == pid) {
			ticketsList.push_back(cit->second);
		}
	}
	return 0;
}


int PrivateConstData::InsertCoreHotel(const LYPlace* city)
{
    if (!city)
        return 1;
    std::string cityID = city->_ID;
    if (ConstDataCheck::m_needLog) MJ::PrintInfo::PrintLog("PrivateConstData::InsertCoreHotel private city %s(%s) make core hotel begin", city->_ID.c_str(), city->_name.c_str());
    LYPlace* coreHotel = NULL;
    std::string id = "coreHotelByMakeCid" + city->_ID;
    std::string coord = city->_poi;
    std::string name = "酒店";
    std::string lname = city->_name + "的酒店";
    coreHotel = LYConstData::MakeHotel(id, name, lname, coord, POI_CUSTOM_MODE_MAKE);
    coreHotel->m_ptid = city->m_ptid;
	m_hotels[id]=coreHotel;

    return 0;
}


std::string PrivateConstData::GetSourceNameBySid(std::string sid) {
	if (m_sid2sourceName.count(sid)) {
		return m_sid2sourceName[sid];
	}
	return sid;
}

int PrivateConstData::LoadTag() {
	MJ::PrintInfo::PrintLog("PrivateConstData::loadTag, loading Tag Data...");
	enum					{ id, tag_id, ptid};
	std::string sql = "select id, tag_id, ptid from tag where disable = 0";
	int t = mysql_query(&m_privateMysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("PrivateConstData::loadTagData, mysql_query error: %s error sql: %s", mysql_error(&m_privateMysql), sql.c_str());
		return 1;
	} else {
		_ptid2tagIds.clear();
		MYSQL_RES* res = mysql_use_result(&m_privateMysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				if (row[tag_id] == NULL) continue;
				if (row[ptid] == NULL) continue;
				std::string tagIdStr = row[tag_id];
				_ptid2tagIds[row[ptid]].insert(tagIdStr);
				num++;
			}
			MJ::PrintInfo::PrintLog("PrivateConstData::loadTagData, Load Tag Num: %d", num);
		}
		mysql_free_result(res);
	}
	return 0;
}

