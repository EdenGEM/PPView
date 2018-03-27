#include <algorithm>
#include <cmath>
#include <string.h>
#include <limits>
#include "MJCommon.h"
#include "RouteConfig.h"
#include "PlaceGroup.h"
#include "TrafficData.h"
#include "LYConstData.h"
#include "ConstDataCheck.h"
#include <sys/time.h>
#include "MJDataProc.h"
#include "http/MJHotUpdate.h"
#include "TimeIR.h"


static double kPI = atan(1) * 4;

using namespace std;

pthread_mutex_t LYConstData::mutex_locker_;
MYSQL LYConstData::_mysql;
Restaurant* LYConstData::m_attachRest;
LYPlace* LYConstData::m_segmentPlace;

int LYConstData::earthRadius = 6378137;//地球赤道半径(m)
std::tr1::unordered_map<std::string, std::string> LYConstData::_sourceId2Name;
std::tr1::unordered_map<std::string, int> LYConstData::_sourceId2Api;
std::tr1::unordered_map<int,const TicketsFun*> LYConstData::_id2ProdTickets;
std::tr1::unordered_map<std::string, std::tr1::unordered_set<int> > LYConstData::_pid2ticketsId;
std::tr1::unordered_map<std::string,LYPlace*> LYConstData::_id2place;
std::tr1::unordered_map<std::string,City*> LYConstData::_id2city_OnlyUsedInLoading;
std::tr1::unordered_map<std::string,LYPlace*> LYConstData::_id2restaurant_OnlyUsedInLoading;
std::tr1::unordered_map<std::string,LYPlace*> LYConstData::_id2hotel_OnlyUsedInLoading;
std::tr1::unordered_map<std::string, std::set<std::string> > LYConstData::_cityViews;
std::tr1::unordered_map<std::string, std::set<std::string> > LYConstData::_cityRestaurants;
std::tr1::unordered_map<std::string, std::set<std::string> > LYConstData::_cityShops;
std::tr1::unordered_map<std::string, std::set<std::string> > LYConstData::_cityTours;
MJ::MJSquareGrid LYConstData::m_gridMap; //公里网映射结构体

std::tr1::unordered_map<std::string, std::string> LYConstData::_label_name_map;
std::tr1::unordered_set<std::string> LYConstData::_baseTagIds;
//SMZ
PrivateConstData* LYConstData::m_privateConstData = NULL;

std::tr1::unordered_set<std::string> LYConstData::m_citySet;
std::tr1::unordered_map<std::string, const LYPlace*> LYConstData::m_parkCity;//国家公园城市~
//SMZ END
std::tr1::unordered_map<std::string, std::tr1::unordered_map<int, std::vector<std::string> > > LYConstData::m_lFHViewIDListMap; //<cid, <days, vidList> >
//Tag相关信息
//shyy  tagS 目前仅用于主题乐园不压缩时长
std::tr1::unordered_map<int, std::tr1::unordered_map<std::string, std::string>> LYConstData::m_poiType2smallTagNameAndId;
//shyy end

std::tr1::unordered_map<std::string, const Country*> LYConstData::_cid2country;
boost::thread_specific_ptr<MJ::MyRedis > LYConstData::_redis;
boost::thread_specific_ptr<QueryParam > LYConstData:: _queryParam;


const int MAX_VIEW_DIS = 50000;
const int MAX_HOTEL_COST = 5000000;
bool NextItemDistCmp(const NextItem* item_a, const NextItem* item_b) {
	return item_a->_dist < item_b->_dist;
}

bool NextItemHotCmp(const NextItem* item_a, const NextItem* item_b) {
	return dynamic_cast<const View*>(item_a->_next_view)->_hot_level > dynamic_cast<const View*>(item_b->_next_view)->_hot_level;
}

bool NextItemScoreCmp(const NextItem* item_a, const NextItem* item_b) {
	return item_a->_score > item_b->_score;
}

bool IntensityDurCmp(const Intensity* item_a, const Intensity* item_b) {
	return item_a->_dur < item_b->_dur;
}

bool LYConstData::destroy() {

	delete m_privateConstData;

	std::tr1::unordered_map<std::string,LYPlace*>::iterator mi;
	for (mi = _id2place.begin(); mi != _id2place.end(); mi ++) {
		delete mi->second;
	}
	_id2place.clear();
	std::tr1::unordered_map<int, const TicketsFun*>::iterator tit;
	for (tit = _id2ProdTickets.begin(); tit != _id2ProdTickets.end(); tit++) {
		delete tit->second;
	}
	_id2ProdTickets.clear();

	TrafficData::Release();

	m_poiType2smallTagNameAndId.clear();

	//shyy end
	pthread_mutex_destroy(&mutex_locker_);
	return true;
}

bool LYConstData::LoadData() {
	bool ret = true;
	if (!ret) return false;
	ret = LoadStationData();
	if (!ret) return false;

	ret = LoadCoreHotel();
	if (!ret) return false;

	ret = loadTag();
	if (!ret) return false;

	ret = loadStagData();
	if (!ret) return false;

	ret = LoadIntensity();
	if (!ret) return false;
	// 5 购物信息
	// 6 景点信息
	// 6.1 景点偏好
	// 6.2 景点数据
	ret = LoadVarPlaceData();
	if (!ret) return false;

	/////////////////////////
	//6 Tour信息  tour 关联景点信息，须先load景点
	ret = LoadTourData();
	if (!ret) return false;
	ret = LoadTicketsFun();
	if (!ret) return false;
	ret = LoadSource();
	if (!ret) return false;

	// 9 球面坐标投影到XY坐标
	ret = PointCylindrical();
	if (!ret) return false;
	ret = SetPlaceIdx();
	if (ret != 0) return false;

	return true;
}

PrivateConstData* LYConstData::GetPriDataPtr() {
	const QueryParam* qp=GetQueryParam();
	if (qp) {
		int threadId = qp->priDataThreadId;
		if ((MJ::MJHotUpdate::getSharedData(threadId))) {
			PrivateConstData* priData = (PrivateConstData*)(MJ::MJHotUpdate::getSharedData(threadId));
			if (priData) {
				return priData;
			}
		}
	}
	return m_privateConstData;
}

int LYConstData::SetPlaceIdx() {
	int idx = 0;
	char buff[100];
	for (std::tr1::unordered_map<std::string, LYPlace*>::iterator it = _id2place.begin(); it != _id2place.end(); ++it) {
		LYPlace* place = it->second;
		snprintf(buff, sizeof(buff), "%d", idx++);
		place->_sid.assign(buff);
	}
	return 0;
}


//获取城市信息
const City* LYConstData::getCity(const std::string& id){
	PrivateConstData* priData = GetPriDataPtr();
	const LYPlace *place = priData->GetLYPlace(id,"");
	if(place == NULL) {
		if (_id2city_OnlyUsedInLoading.count(id)) return dynamic_cast<const City*>(_id2city_OnlyUsedInLoading[id]);
		const LYPlace* ret = getBaseLYPlace(id);
		return dynamic_cast<const City*>(ret);
	} else {
		return dynamic_cast<const City*>(place);
	}
}

//根据ID获取对应的地点信息
const LYPlace* LYConstData::getBaseLYPlace(const std::string& id){
	//查景点
	std::tr1::unordered_map<std::string,LYPlace*>::const_iterator it = _id2place.find(id);
	if (it!=_id2place.end()){
		return it->second;
	} else {
		return NULL;
	}
}

const LYPlace* LYConstData::GetViewByViewTicket(ViewTicket* viewTicket, const std::string& ptid) {
	std::string id = viewTicket->Getm_view();
	auto place = GetLYPlace(id, ptid);
	if(place and not place->m_ptid.empty() and place->m_ptid!=ptid) place = NULL;
	return place;
}

const LYPlace* LYConstData::GetLYPlace(const std::string& id, const std::string& ptid){

	//MJ::PrintInfo::PrintLog("LYConstData::GetLYPlace, id is %s, ptid is %s", id.c_str(), ptid.c_str());
	if (id.length()==0){
		return NULL;
	}
	//先查是不是隐藏点
	PrivateConstData* priData = GetPriDataPtr();
	if (priData->IsShieldId(ptid, id)) {
		return NULL;
	}
	//是不是私有库点
	const LYPlace* place = priData->GetLYPlace(id,ptid);
	//if (place == NULL)
	//	MJ::PrintInfo::PrintLog("LYConstData::GetLYPlace, get private data fail, id is %s, ptid is %s", id.c_str(), ptid.c_str());
	//if (place != NULL)
	//	MJ::PrintInfo::PrintLog("LYConstData::GetLYPlace, get private data fail, and place is not null, place id is %s, place ptid is %s", place->_ID.c_str(), place->m_ptid.c_str());
	if (place == NULL) {
		place = getBaseLYPlace(id);
	}

	//是不是共有库点
	if (place) {
		if (place->_t == LY_PLACE_TYPE_VIEWTICKET) {
			//景点门票需要保证关联景点存在
			const ViewTicket* viewTicket = dynamic_cast<const ViewTicket*>(place);
			if (not (viewTicket && LYConstData::GetViewByViewTicket(const_cast<ViewTicket*>(viewTicket), ptid) != NULL)) {
				return NULL;
			}
		}
		return place;
	}

	return NULL;

}

const std::string LYConstData::GetRealID(const std::string& id) {
	//是不是私有库点
	return id;
}

//城市的相关信息

bool LYConstData::LoadCountryData() {
	MJ::PrintInfo::PrintLog("LYConstData::loadCountryData, Loading Country Data...");
	enum			         {id,name};
	std::string sql = "select mid,name from country where status = 'Open' or mid = 101 ";//额外加载中国
	int t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintErr("LYConstData::loadCountryData, mysql_query error: %s, error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				Country* country = new Country; // add Country in define.h
				country->_ID = row[id];
				if (row[name]) {
					country->_name = row[name];
				}
				if (_cid2country.find(country->_ID)!=_cid2country.end()){
					MJ::PrintInfo::PrintLog("LYConstData::loadCountryData, ID(%s) already exist", country->_ID.c_str());
					delete country;
					continue;
				}
				_cid2country.insert(make_pair(country->_ID, country)); // add _cid2country in LYConstData.h
				num++;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadCountryData, Load Country Num: %d", num);
		}
		mysql_free_result(res);
	}
	return true;
}

const Country* LYConstData::GetCountry(const std::string &cid) {
	if (_cid2country.find(cid)!=_cid2country.end()) {
		return _cid2country[cid];
	} else {
		MJ::PrintInfo::PrintErr("LYConstData::loadCityData, country id error: country_id(%s)", cid.c_str());
		return NULL;
	}
}

//PI:0b26ebf3	 拉斯加雷拉斯|-69.2028221,19.2819375|多米尼加共和国|2|-5.0||
bool LYConstData::LoadCityData() {
	MJ::MyTimer timer;
	timer.start();
	MJ::PrintInfo::PrintLog("LYConstData::loadCityData, Loading City Data...");
	enum					{ id,name,map_info,country_id,grade,time_zone,name_en,is_park };
	std::string sql = "select id,name,map_info,country_id,grade,time_zone,name_en,is_park  from city "; // zyc: 没有 newProduct_status 或者 status
	int t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintErr("LYConstData::loadCityData, mysql_query error: %s, error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				City* city = new City;
				city->_t = LY_PLACE_TYPE_CITY;
				city->m_custom = POI_CUSTOM_MODE_CONST;
				city->_ID = row[id];
				city->_cid_list = std::vector<std::string>(1, city->_ID);
				if (row[name]) city->_name = row[name];
				city->_poi = row[map_info] ? row[map_info] : "";
				if (row[country_id]) {
					const Country *country = LYConstData::GetCountry(row[country_id]);
					if (country != NULL) {
						city->_country = country->_name;
					} else {
						MJ::PrintInfo::PrintErr("LYConstData::loadCityData, country id error: city_id(%s), city_name(%s), country_id(%s)", row[id], row[name], row[country_id]);
					}
				}
				if (row[grade]) city->_hot = atoi(row[grade]);
				if (row[time_zone]) city->_time_zone = atof(row[time_zone]);
				city->_enname = row[name_en] ? row[name_en] : "";
				if (_id2place.find(city->_ID)!=_id2place.end()){
					MJ::PrintInfo::PrintLog("LYConstData::loadCityData, ID(%s) already exist", city->_ID.c_str());
					delete city;
					continue;
				}
				int isPark = row[is_park] ? atoi(row[is_park]) : 0;
				if (isPark)  {//城市公园
					m_parkCity.insert(make_pair(city->_ID,city));
					//MJ::PrintInfo::PrintLog("LYConstData::loadCityData, add cityPark %s(%s)", city->_ID .c_str(), city->_name.c_str());
				}
				m_citySet.insert(city->_ID);
				_id2city_OnlyUsedInLoading.insert(make_pair(city->_ID,city));
				_id2place.insert(make_pair(city->_ID,city));
				num++;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadCityData, Load City Num: %d, cost: %d ms", num, timer.cost()/1000);
		}
		mysql_free_result(res);
	}

	sql = "select cid,uppers from city_relations";
	t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintErr("LYConstData::loadCityData, mysql_query error: %s, error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (_id2city_OnlyUsedInLoading.count(row[0]))
				{
					std::string cids = row[1] ? row[1] : "";
					std::vector<std::string> cid_list;
					ToolFunc::sepString(cids, "|", cid_list, "");
					for(auto i=0; i< cid_list.size(); i++)
					{
						if(_id2city_OnlyUsedInLoading.count(cid_list[i]))
						{
							_id2city_OnlyUsedInLoading[row[0]]->_uppers.push_back(cid_list[i]);
							num++;
						}
					}
				}
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadCityData, Load city_relations Num: %d", num);
		}
		mysql_free_result(res);
	}

	return true;
}


//车站的相关信息
//PI:NGO  名古屋机场|136.8118399,34.8616518|||1||
bool LYConstData::LoadStationData(){
	MJ::PrintInfo::PrintLog("LYConstData::LoadStationData...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	std::string sql;
	enum		 {       id,name,map_info,city_id,name_en};
	sql = "select iata_code,name,map_info,city_id,name_en from airport ";
	loadParams["airport"] = std::make_pair(sql,LY_PLACE_TYPE_AIRPORT);

	sql = "select station_id,online_name,map_info,city_id,online_name_en from station";
	loadParams["station"] = std::make_pair(sql,LY_PLACE_TYPE_STATION);

	sql = "select terminal_id,terminal_name,map_info,city_id,terminal_name_en from sail_station where status = 'Open'";
	loadParams["sail_station"] = std::make_pair(sql,LY_PLACE_TYPE_SAIL_STATION);

	for (auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		MJ::PrintInfo::PrintLog("LYConstData::loadStationData, Loading %s Data...",it->first.c_str());
		int t = mysql_query(&_mysql, it->second.first.c_str());
		if (t != 0) {
			MJ::PrintInfo::PrintErr("LYConstData::loadStationData, mysql_query error: %s, error sql: %s", mysql_error(&_mysql), it->second.first.c_str());
			return false;
		} else {
			MYSQL_RES* res = mysql_use_result(&_mysql);
			MYSQL_ROW row;
			if (res){
				int num = 0;
				while (row = mysql_fetch_row(res)){
					if (row[id] == NULL)
						continue;
					Station* st = new Station;
					st->m_custom = POI_CUSTOM_MODE_CONST;
					st->_t = it->second.second;
					st->_ID = row[id];
					if (row[name]) st->_name = row[name];
					if (row[map_info]) st->_poi = row[map_info];
					std::string cids = row[city_id] ? row[city_id] : "";
					ToolFunc::sepString(cids, "|", st->_cid_list, "");
					ToolFunc::UniqListOrder(st->_cid_list);
					st->_enname = row[name_en] ? row[name_en] : "";
					if (_id2place.find(st->_ID) != _id2place.end()) {
						ToolFunc::MergeListUniq(st->_cid_list, _id2place[st->_ID]->_cid_list);
						delete st;
						continue;
					}
					_id2place.insert(make_pair(st->_ID,st));
					num++;
				}
				MJ::PrintInfo::PrintLog("LYConstData::loadCityData, Load %s Num: %d", it->first.c_str(),num);
			}
			mysql_free_result(res);
		}
	}

	return true;
}


//酒店的相关信息
//PI:elong_226067 斯坦林布什酒店|18.86147,-33.93871||3|||
bool LYConstData::LoadHotelData() {
	MJ::MyTimer timer;
	timer.start();
	MYSQL _hotelsql;
	bool ret = initSQL(_hotelsql);
	if (!ret) return ret;
	MJ::PrintInfo::PrintLog("LYConstData::loadHotelData, loading Hotel Data...");
	enum					{ id, name,		 name_en,	   map_info,star,city_id };
	std::string sql = "select uid,hotel_name,hotel_name_en,map_info,star,city_mid from hotel";
	int t = mysql_query(&_hotelsql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("LYConstData::loadHotelData, mysql_query error: %s error sql: %s", mysql_error(&_hotelsql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_hotelsql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				Hotel* h = new Hotel;
				h->m_custom = POI_CUSTOM_MODE_CONST;
				h->_t = LY_PLACE_TYPE_HOTEL;
				h->_ID = row[id];
				h->_name = row[name] ? row[name] : "";
				h->_enname = row[name_en] ? row[name_en] : "";
				if(h->_name=="") h->_name = h->_enname;
				if (row[map_info]) h->_poi = row[map_info];
				if (row[star]) h->_lvl = atoi(row[star]);
				std::string cids = row[city_id] ? row[city_id] : "";
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
				h->_tags = "";
				h->_viewDis = MAX_VIEW_DIS;
				if (_id2hotel_OnlyUsedInLoading.find(h->_ID) != _id2hotel_OnlyUsedInLoading.end()){
					ToolFunc::MergeListUniq(h->_cid_list, _id2hotel_OnlyUsedInLoading[h->_ID]->_cid_list);
					delete h;
					continue;
				}
				_id2hotel_OnlyUsedInLoading.insert(std::make_pair(h->_ID,h));
				num++;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadHotelData, Load Hotel Num: %d, cost: %d ms", num, timer.cost()/1000);
		}
		mysql_free_result(res);
	}
	mysql_close(&_hotelsql);
	return true;
}


//////////////
//Tour相关信息
bool LYConstData::LoadTourData() {
	MJ::PrintInfo::PrintLog("LYConstData::LoadTourData...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	std::string sql;
	//enum     {  pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,      map_info,first_img,pid_3rd, sid, tag};
	sql = "select pid,ptid,poi_mode,ref_poi, name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,'' as map_info,first_img,pid_3rd,sid, tag from view_ticket where disable = 0;";
	loadParams["view_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_VIEWTICKET);

	sql = "select pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,map_info,first_img,pid_3rd,sid, '' as  tag from play_ticket where disable = 0;";
	loadParams["play_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_PLAY);

	sql = "select pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,map_info,first_img,pid_3rd,sid, tag from activity_ticket where disable = 0;";
	loadParams["activity_ticket"] = std::make_pair(sql, LY_PLACE_TYPE_ACTIVITY);

	for (auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		if (LoadSpecifiedTypeTour(it->first, it->second.first, it->second.second) == false) {
			return false;
		}
	}
	return true;
}

bool LYConstData::LoadSpecifiedTypeTour(std::string typeT, std::string sql, int typeI) {
	enum     {  pid,ptid,poi_mode,ref_pois,name,ename,open,times,book_pre,enter_pre,jiesong_type,jiesong_poi,city_id,      map_info,first_img,     pid_3rd,sid, tag};
	MJ::PrintInfo::PrintLog("LYConstData::LoadSpecifiedTypeTour, loading Public Tour %s Data...", typeT.c_str());
	int ret = 0;
	MJ::MyTimer t;
	t.start();
	ret = mysql_query(&_mysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("LYConstData::LoadSpecifiedTypeTour, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[pid] == NULL) continue;
				if (row[ref_pois] == NULL) continue;
				std::string id = row[pid];
				Tour* tour = NULL;
				const LYPlace *vPlace = NULL;
				if (typeI != LY_PLACE_TYPE_VIEWTICKET) {
					tour = new Tour;
					//写入关联景点
					Json::Value value;
					Json::Reader jr(Json::Features::strictMode());
					jr.parse(row[ref_pois], value);
					if(value.isArray() && value.size() > 0) {
						for (int i = 0; i < value.size(); ++i) {
							vPlace = getBaseLYPlace(value[i].asString());
							if(vPlace == NULL) {
								continue;
							}
							tour->m_refPoi.insert(value[i].asString());
						}
					}
				}
				else {
					tour = new ViewTicket;
					vPlace = getBaseLYPlace(row[ref_pois]);
					if (vPlace == NULL) {
						std::cerr << "id to place error:" << row[ref_pois] << " - "<< row[pid] << std::endl;
						delete tour;
						continue;
					}
					if (dynamic_cast<ViewTicket*>(tour) != NULL) {
						(dynamic_cast<ViewTicket*>(tour))->Setm_view(row[ref_pois]);
					}
					else {
						delete tour;
						continue;
					}
				}

				tour->m_custom = POI_CUSTOM_MODE_CONST;//代表公有库字段
				tour->_ID = row[pid];
				tour->m_pid = row[pid];
				if (row[poi_mode]) tour->_t = atoi(row[poi_mode]);
				if (row[ptid]) tour->m_ptid = row[ptid];
				if (row[name]) tour->_name = row[name];
				if (row[ename]) tour->_enname = row[ename];
				if (row[map_info] && std::string(row[map_info])!="") {
					tour->_poi = row[map_info];
				}
				else if (vPlace) {
					tour->_poi = vPlace->_poi;
				}
				if (row[city_id]) tour->_cid_list.push_back(row[city_id]);
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
								_id2place.insert(std::make_pair(gatherId, place));
								tour->m_gatherLocal.insert(gatherId);
								tour->m_dismissLocal.insert(gatherId);
							}
						}
					}
				}
				if (row[book_pre]) tour->m_preBook = atoi(row[book_pre]);
				if (row[enter_pre]) tour->m_preTime = atoi(row[enter_pre]);
				if (row[first_img]) tour->_img = row[first_img];
				if (row[pid_3rd]) tour->m_pid_3rd = row[pid_3rd];
				if (row[sid]) tour->m_sid = row[sid];

				tour->m_open = Json::arrayValue;
				if (row[open]) {
					Json::Reader jReader;
					Json::Value jValue = Json::arrayValue;
					jReader.parse(row[open], jValue);
					if (jValue.isArray() && jValue.size() > 0) {
						tour->m_open = jValue;
					}
				}

				tour->m_srcTimes = Json::Value();
				Json::Value value = Json::arrayValue;
				if (row[times]) jr.parse(row[times], value);
				int dur = 1800;
				tour->m_durEmpty = false;
				if ((value.isArray() && value.size() > 0) || (value.isObject() && value.isMember("dur") && value.isMember("t"))) {
					if (!value.isArray()) {
						Json::Value new_Value = Json::arrayValue;
						new_Value.append(value);
						value = new_Value;
					}
					tour->m_srcTimes = value;
					if (value[0].isMember("dur") && value[0]["dur"].isInt()) {
						dur = value[0]["dur"].asInt();
					} else {
						dur = 3600*2;
						tour->m_durEmpty = true;
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
				if (row[tag]) {
					std::vector<std::string> tagList;
					ToolFunc::sepString(row[tag], "|", tagList);
					for (int i = 0; i < tagList.size(); i++) {
						std::string tagIdStr = tagList[i];
						int start = tagIdStr.find("tag");
						if (start == std::string::npos) continue;
						if (_baseTagIds.count(tagIdStr) == 0) continue;
						tour->m_tag.insert(tagIdStr);
					}
				}

				tour->_intensity_list.push_back(new Intensity("2-1", "简单游玩", dur, -1, false, "CNY"));
				tour->_rcmd_intensity = tour->_intensity_list[0];

				if (_id2place.find(tour->_ID) != _id2place.end()) {
					ToolFunc::MergeListUniq(tour->_cid_list, _id2place[tour->_ID]->_cid_list);
					MJ::PrintInfo::PrintLog("LYConstData::loadTour, ID(%s) already exist", tour->_ID.c_str());
					delete tour;
					continue;
				}
				std::vector<std::string>::iterator itCid;
				for (itCid = tour->_cid_list.begin(); itCid != tour->_cid_list.end();) {
					const City* city = getCity(*itCid);
					if (city == NULL) {
						//if (ConstDataCheck::m_needLog) MJ::PrintInfo::PrintLog("[DATA EXCEPTION][CITY ID] city %s not found %s(%s)", (*itCid).c_str(), tour->_ID.c_str(), tour->_name.c_str());
						itCid = tour->_cid_list.erase(itCid);
						continue;
					}
					itCid ++;
				}
				_id2place.insert(std::make_pair(tour->_ID, tour));
				for (int i = 0; i < tour->_cid_list.size(); ++i) {
					std::string cid = tour->_cid_list[i];
					Insert2CityPlaceMap(cid, tour, _cityTours);
				}
				++num;
			}
			MJ::PrintInfo::PrintLog("LYConstData::LoadSpecifiedTypeTour, Load %s Num: %d", typeT.c_str(), num);
		}
		mysql_free_result(res);
	}
	return true;
}
bool LYConstData::LoadTicketsFun() {
	MJ::PrintInfo::PrintLog("LYConstData::LoadTicketsFun...");
	enum					{ id,ticket_id,pid,name,info,ccy,book_pre,ticketType,max,min,agemin,agemax,ticket_3rd,traveller_3rd,date_price,sid };
	std::string sql = "select id,ticket_id,pid,name,info,ccy,book_pre,ticketType,max,min,agemin,agemax,ticket_3rd,traveller_3rd,date_price,sid from tickets_fun where disable = 0;";
	int ret = 0;
	ret = mysql_query(&_mysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("LYConstData::loadTicketsFun, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int tfnum = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				if (row[ticket_id] == NULL) continue;
				if (row[pid] == NULL) continue;
				TicketsFun* ticketsFun = new TicketsFun;
				ticketsFun->m_id = atoi(row[id]);
				ticketsFun->m_ticket_id = row[ticket_id];
				if (row[pid]) ticketsFun->m_pid = row[pid];
				if (row[name]) ticketsFun->m_name = row[name];
				if (row[ccy]) ticketsFun->m_ccy = row[ccy];
				if (row[ticketType]) ticketsFun->m_ticketType = atoi(row[ticketType]);
				if (row[max]) ticketsFun->m_max = atoi(row[max]);
				if (row[min]) ticketsFun->m_min = atoi(row[min]);
				if (row[agemin]) ticketsFun->m_agemin = atoi(row[agemin]);
				if (row[agemax]) ticketsFun->m_agemax = atoi(row[agemax]);
				if (row[ticket_3rd]) ticketsFun->m_ticket_3rd = row[ticket_3rd];
				if (row[traveller_3rd]) ticketsFun->m_traveller_3rd = row[traveller_3rd];
				if (row[sid]) ticketsFun->m_sid = row[sid];

				if (row[info]) {
					Json::Value jv;
					Json::Reader jr(Json::Features::strictMode());
					jr.parse(row[info], jv);
					ticketsFun->m_info = jv;
					int num = 0;
					if (jv["info"].isArray()) {
						for (int i = 0; i < jv["info"].size(); ++i) {
							if (jv["info"][i].isObject() && jv["info"][i].isMember("num") && jv["info"][i]["num"].isInt()) {
								num += jv["info"][i]["num"].asInt();
							}
						}
					} else {
						delete ticketsFun;
						ticketsFun = NULL;
						continue;
					}
					ticketsFun->m_userNum = num;
				}

				if (row[date_price]) {
					Json::Value jv = Json::arrayValue;
					Json::Reader jr(Json::Features::strictMode());
					jr.parse(row[date_price], jv);
					if (jv.isArray()) ticketsFun->m_date_price = jv;
				}

				if(_id2ProdTickets.find(atoi(row[id])) == _id2ProdTickets.end()) {
					_id2ProdTickets.insert(std::make_pair(atoi(row[id]), ticketsFun));
				}
				_pid2ticketsId[row[pid]].insert(atoi(row[id]));

				++tfnum;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadTicketsFun, Load TicketsFun Num: %d", tfnum);
		}
		mysql_free_result(res);
	}
	return true;
}

bool LYConstData::LoadSource() {
	MJ::PrintInfo::PrintLog("LYConstData::LoadSource, loading Source Data...");
	enum					 {sid,descr,isApi};
	std::string sql = "select sid,descr,isApi from source;";
	int ret = mysql_query(&_mysql, sql.c_str());
	if (ret) {
		MJ::PrintInfo::PrintLog("LYConstData::loadTourData, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[sid] == NULL) continue;
				if (row[sid] && row[descr] && row[isApi]) {
					std::string _sid = row[sid];
					std::string _sname = row[descr];
					int _isApi = atoi(row[isApi]);
					_sourceId2Name.insert(std::make_pair(_sid, _sname));
					_sourceId2Api.insert(std::make_pair(_sid, _isApi));
				}
				++num;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadPrivateSource, Load source Num: %d", num);
		}
		mysql_free_result(res);
	}
	return true;
}

//////////////
//获取小城市对应的枢纽城市

bool LYConstData::initSQL(MYSQL &mysql) {
	//初始化mysql
	mysql_init(&mysql);
	MJ::PrintInfo::PrintLog("LYConstData::init, Mysql connect to %s:%s", RouteConfig::db_host.c_str(), RouteConfig::db_name.c_str());
	if (!mysql_real_connect(&mysql,
				RouteConfig::db_host.c_str(),
				RouteConfig::db_user.c_str(),
				RouteConfig::db_passwd.c_str(),
				RouteConfig::db_name.c_str(), 0, NULL, 0)){
		MJ::PrintInfo::PrintErr("LYConstData::init, Connect to %s error: %s", RouteConfig::db_name.c_str(), mysql_error(&mysql));
		return false;
	}
	if (mysql_set_character_set(&mysql, "utf8")){
		MJ::PrintInfo::PrintErr("LYConstData::init, Set mysql characterset: %s", mysql_error(&mysql));
		return false;
	}
	return true;
}

bool LYConstData::init(){
	pthread_mutex_init(&mutex_locker_,NULL);
	//	m_searchMode = RouteConfig::search_mode;
	if(getRedisHandle() == NULL){
		MJ::PrintInfo::PrintErr("LYConstData::init, Connect Redis failed");
		return false;
	}

	m_privateConstData = new PrivateConstData();

	int init_ret = TrafficData::Init();
	if (init_ret != 0) {
		MJ::PrintInfo::PrintErr("LYConstData::init, TrafficData error:%d", init_ret);
		return false;
	}

	init_ret = PlaceGroup::InitFreq();
	if (init_ret != 0) {
		MJ::PrintInfo::PrintErr("LYConstData::init, PlaceGroup error:%d", init_ret);
		return false;
	}

	init_ret = ToolFunc::FormatChecker::Init();
	if (init_ret != 0) return false;

	bool ret = true;
	//初始化mysql
	ret = initSQL(_mysql);
	if (!ret) return false;

	// 1 读取城市信息
	ret = LoadCountryData();
	if (!ret) return false;
	LoadCityData();
	MJ::MyThreadPool* threadPool = new MJ::MyThreadPool;
	threadPool->open(2,204800000);
	threadPool->activate();
	std::vector<MJ::Worker*> jobs;
	for(int i =0; i < 2 ; i++)
	{
		LoadDataWorker * loadDataWorker = new LoadDataWorker(i);
		jobs.push_back(dynamic_cast<MJ::Worker*>(loadDataWorker));
		threadPool->add_worker(dynamic_cast<MJ::Worker*>(loadDataWorker));
	}
	LoadData();
	threadPool->wait_worker_done(jobs);

	for (int i = 0; i < jobs.size(); i++) {
		LoadDataWorker* loadDataWorker = dynamic_cast<LoadDataWorker*>(jobs[i]);
		if (!loadDataWorker->m_ret) ret = loadDataWorker->m_ret;
	}
	if (!ret) return false;

	{
		_id2city_OnlyUsedInLoading.clear();

		_id2place.insert(_id2hotel_OnlyUsedInLoading.begin(), _id2hotel_OnlyUsedInLoading.end());
		_id2hotel_OnlyUsedInLoading.clear();

		for (auto it = _id2restaurant_OnlyUsedInLoading.begin(); it!=_id2restaurant_OnlyUsedInLoading.end();it++) {

			bool ret = m_gridMap.addId2Grid(it->second->_ID, "", it->second->_poi);
			if(!ret) {
				delete it->second;
				it->second = NULL;
				continue;
			}
		}
		_id2place.insert(_id2restaurant_OnlyUsedInLoading.begin(), _id2restaurant_OnlyUsedInLoading.end());
		_id2restaurant_OnlyUsedInLoading.clear();
	}
	init_ret = ToolFunc::RateExchange::loadExchange(_mysql);
	if (!init_ret) return false;
	mysql_close(&_mysql);

	m_privateConstData->PrivateLoad();
	const QueryParam* qp=GetQueryParam();
	if (!qp) {
		MJ::MJHotUpdate::setThread(RouteConfig::thread_num);
	}

	m_segmentPlace = NULL;
	m_segmentPlace = new LYPlace();
	m_segmentPlace->_poi = "0,0";
	m_segmentPlace->_name = "多段切分";
	m_segmentPlace->_lname = "";
	m_segmentPlace->_ID = "segment";
	m_segmentPlace->_time_zone = -1;
	m_segmentPlace->_t = LY_PLACE_TYPE_BLANK;
	m_segmentPlace->m_custom = POI_CUSTOM_MODE_MAKE;
	_id2place[m_segmentPlace->_ID] = m_segmentPlace;

	m_attachRest = new Restaurant;
	m_attachRest->_poi = "0,0";
	m_attachRest->_ID = "attach";
	m_attachRest->_name = "附近就餐";
	m_attachRest->_lname = "";
	m_attachRest->_t = LY_PLACE_TYPE_RESTAURANT;
	m_attachRest->_time_zone = -1;
	m_attachRest->_hot_level = 1;
	m_attachRest->_tags = "";
	m_attachRest->_price = "";
	m_attachRest->_time_rule = "";
	m_attachRest->m_custom = POI_CUSTOM_MODE_MAKE;
	_id2place[m_attachRest->_ID] = m_attachRest;
	return ret;
}

//对开关门的规则做了修正
// 重排序开关门规则，把黑名单放前面
std::string FixOpenRule(const std::string& ori_open_rule) {
	std::string new_open_rule = ori_open_rule;
	ToolFunc::replaceString(new_open_rule, "–", "-");
	ToolFunc::replaceString(new_open_rule, "：", ":");
	std::vector<std::string> item_list;
	ToolFunc::sepString(new_open_rule, "|", item_list);
	std::vector<std::string> resort_item_list;
	std::vector<std::string> open_item_list;
	for (std::vector<std::string>::iterator it = item_list.begin(); it != item_list.end(); ++it) {
		std::string item_rule = *it;
		std::string::size_type pos = item_rule.find("不开门");
		if (pos != std::string::npos) {
			resort_item_list.push_back(item_rule);
		} else {
			open_item_list.push_back(item_rule);
		}
	}
	resort_item_list.insert(resort_item_list.end(), open_item_list.begin(), open_item_list.end());
	return ToolFunc::join2String(resort_item_list, "|");

}

//给出中心点经纬度和距离,得到相应网格范围中的poi  Eden
bool LYConstData::GetRangePlaceId(std::string mapInfo,int maxDist,std::set<std::pair<std::string, std::string>> &idList){
	m_gridMap.getRangePlace(mapInfo, maxDist, idList);
    return true;
}

// 从内存数据中取view
int LYConstData::getViewLocal(const std::string& CityID, std::vector<const LYPlace*>& placeList, const std::string& ptid) {
	std::set<std::string> idLists;
	auto it = _cityViews.find(CityID);
	if (it == _cityViews.end()) {
		MJ::PrintInfo::PrintErr("LYConstData::getViewLocal, can not find ID: %s", CityID.c_str());
	} else {
		idLists.insert(it->second.begin(), it->second.end());
	}
	PrivateConstData* priData = GetPriDataPtr();
	if (ptid != "") {
		priData->GetCityViewPrivate(CityID, idLists,ptid);
	}
	priData->ShieldPlaceList(idLists, ptid);

	placeList.clear();
	for(auto it=idLists.begin();it!=idLists.end();it++)
	{
		auto place = GetLYPlace(*it,ptid);
		if(place) placeList.push_back(place);
	}

	return 0;
}

// 从内存数据中取shop
int LYConstData::getShopLocal(const std::string& CityID, std::vector<const LYPlace*>& placeList, const std::string& ptid) {
	std::set<std::string> idLists;
	auto it = _cityShops.find(CityID);
	if (it == _cityShops.end()) {
		MJ::PrintInfo::PrintErr("LYConstData::getShopLocal, can not find ID: %s", CityID.c_str());
	} else {
		idLists.insert(it->second.begin(), it->second.end());
	}
	PrivateConstData* priData = GetPriDataPtr();
	if (ptid != "") {
		priData->GetCityShopPrivate(CityID, idLists,ptid);
	}
	priData->ShieldPlaceList(idLists, ptid);

	placeList.clear();
	for(auto it=idLists.begin();it!=idLists.end();it++)
	{
		auto place = GetLYPlace(*it,ptid);
		if(place) placeList.push_back(place);
	}

	return 0;
}

// 从内存数据中取restaurant
int LYConstData::getRestaurantLocal(const std::string& CityID, std::vector<const LYPlace*>& placeList, const std::string& ptid) {
	std::set<std::string> idLists;
	auto it = _cityRestaurants.find(CityID);
	if (it == _cityRestaurants.end()) {
		MJ::PrintInfo::PrintErr("LYConstData::getRestaurantLocal, can not find ID: %s", CityID.c_str());
	} else {
		idLists.insert(it->second.begin(), it->second.end());
	}
	PrivateConstData* priData = GetPriDataPtr();
	if (ptid != "") {
		priData->GetCityRestaurantPrivate(CityID, idLists,ptid);
	}
	priData->ShieldPlaceList(idLists, ptid);

	placeList.clear();
	for(auto it=idLists.begin();it!=idLists.end();it++)
	{
		auto place = GetLYPlace(*it,ptid);
		if(place) placeList.push_back(place);
	}

	return 0;
}


// 计算hotLevel
int LYConstData::PerfectVarPlace(std::tr1::unordered_map<std::string, std::set<std::string> >& cityPlaceMap) {
	for (auto it = cityPlaceMap.begin(); it != cityPlaceMap.end(); ++it) {
		std::string cid = it->first;
		std::vector<const LYPlace*> vPlaceList;
		for(auto _it=it->second.begin(); _it!=it->second.end(); _it++)
		{
			auto place=getBaseLYPlace(*_it);
			if(place) vPlaceList.push_back(place);
		}
		if (vPlaceList.empty()) continue;
		std::stable_sort(vPlaceList.begin(), vPlaceList.end(), varPlaceCmp());

		int nonMissThre = std::min(0.15 * vPlaceList.size(), 5.0);
		nonMissThre = std::max(nonMissThre, 1);
		for (int i = 0; i < vPlaceList.size(); ++i) {
			const VarPlace* vPlace = dynamic_cast<const VarPlace*>(vPlaceList[i]);
			VarPlace* vPlaceMod = const_cast<VarPlace*>(vPlace);
			vPlaceMod->_hot_rank = i * 1.0 / vPlaceList.size();
			if (vPlace->_t & LY_PLACE_TYPE_VIEW && i < nonMissThre) {
				vPlaceMod->_level |= VIEW_LEVEL_SYSOPT;
			}
		}
	}
	return 0;
}

int LYConstData::LoadSpecifiedTypeVarPlace(std::string tableName, std::string sqlstr, int typeI) {
	MJ::MyTimer timer;
	timer.start();
	int varDelOpenClose = 0;
	int varDelIntensity = 0;
	enum {id,name,name_en,map_info,open,city_id,nearCity,tag,tag_id,min_price,max_price,rcmd_intensity,intensity,hot_level,grade,ranking,utime};
	MYSQL _varPlaceSQL;
	if (typeI == LY_PLACE_TYPE_RESTAURANT) {
		bool ret = initSQL(_varPlaceSQL);
		if (!ret) return 1;
	} else {
		_varPlaceSQL = _mysql;
	}
	int t = mysql_query(&_varPlaceSQL, sqlstr.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("LYConstData::LoadSpecifiedTypeVarPlace, mysql_query error: %s error sql: %s", mysql_error(&_varPlaceSQL), sqlstr.c_str());
		return 1;
	} else {
		MYSQL_RES* res = mysql_use_result(&_varPlaceSQL);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			std::tr1::unordered_set<std::string, std::tr1::unordered_set<const LYPlace*> > placeSet;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				VarPlace* varPlace = NULL;
				if (typeI == LY_PLACE_TYPE_VIEW) {
					varPlace = new View;
				} else if (typeI == LY_PLACE_TYPE_RESTAURANT) {
					varPlace = new Restaurant;
				} else if (typeI == LY_PLACE_TYPE_SHOP) {
					varPlace = new Shop;
				} else continue;
				varPlace->m_custom = POI_CUSTOM_MODE_CONST;
				varPlace->_t = typeI;
				varPlace->_ID = row[id];
				varPlace->_name = row[name] ? row[name] : "";
				ToolFunc::rtrim(varPlace->_name);
				varPlace->_enname = row[name_en] ? row[name_en] : "";
				ToolFunc::rtrim(varPlace->_enname);
				varPlace->_poi = row[map_info] ? row[map_info] : "";
				if (row[map_info]) {
					if (varPlace->_t != LY_PLACE_TYPE_RESTAURANT) {
						bool ret = m_gridMap.addId2Grid(row[id], "", row[map_info]);
						if(!ret) {
							delete varPlace;
							varPlace = NULL;
							std::cerr<<"MJ::GridMap error: map_info: " << row[map_info] << " id:" << row[id] <<std::endl;
							continue;
						}
					}
                }
				varPlace->_grade = row[grade] ? atof(row[grade]) : 0;
				//hot level view==0??是否需要
				varPlace->_hot_level = row[hot_level] ? atof(row[hot_level]) : 1;
				varPlace->_ranking = row[ranking]? atoi(row[ranking]): 0;
				varPlace->_utime = row[utime] ? row[utime] : "";

				varPlace->_price = "";

				std::string time_rule = "";
				if (typeI == LY_PLACE_TYPE_VIEW) {
					time_rule = "<*><*><00:00-23:59><SURE>";
				} else if (typeI == LY_PLACE_TYPE_RESTAURANT) {
					time_rule = "<*><*><11:00-14:00><SURE>|<*><*><18:00-21:00><SURE>";
				} else if (typeI == LY_PLACE_TYPE_SHOP) {
					time_rule = "<*><*><09:00-21:00><SURE>";
				}
				if (row[open] and not std::string(row[open]).empty()) {
					std::string errStr = "";
					bool ret = ConstDataCheck::CheckOpenClose(std::string(row[open]), errStr);
					if (!ret) {
						if (ConstDataCheck::m_needLog) MJ::PrintInfo::PrintLog("[DATA EXCEPTION][OPENCLOSE] %s (%s) [%s(%s)]", std::string(row[open]).c_str(), errStr.c_str(), varPlace->_ID.c_str(), varPlace->_name.c_str());
						if (ConstDataCheck::m_delOpenClose) {
							MJ::PrintInfo::PrintLog("[DATA EXCEPTION][DEL OPENCLOSE] %s (%s) [%s(%s)]", std::string(row[open]).c_str(), errStr.c_str(), varPlace->_ID.c_str(), varPlace->_name.c_str());
							varDelOpenClose ++;
							delete varPlace;
							varPlace = NULL;
							continue;
						}
					} else {
						time_rule = row[open];
					}
				}
				varPlace->_time_rule = time_rule;
				if (TimeIR::CheckTimeRule(varPlace->_time_rule)) {
					varPlace->_time_rule = time_rule;
				}
				varPlace->_time_rule = FixOpenRule(varPlace->_time_rule);
				float _min_price = row[min_price] ? atoi(row[min_price]) : 0;
				float _max_price = row[max_price] ? atoi(row[max_price]) : 0;
				varPlace->avgPrice = (_min_price + _max_price) / 2;

				//tagId
				if (row[tag]) {
					std::vector<std::string> tagList;
					ToolFunc::sepString(row[tag], "|", tagList);
					for (int i = 0; i < tagList.size(); i++) {
						std::string tagIdStr = tagList[i];
						int start = tagIdStr.find("tag");
						if (start == std::string::npos) continue;
						if (_baseTagIds.count(tagIdStr) == 0) continue;
						varPlace->m_tag.insert(tagIdStr);
					}
				}
				varPlace->m_tagSmallStr = row[tag_id] ? row[tag_id] : "";
				if (row[city_id] && strlen(row[city_id]) > 0) {
					std::string _cid = row[city_id];
					if(_id2city_OnlyUsedInLoading.count(_cid)){
						varPlace->_cid_list = _id2city_OnlyUsedInLoading[_cid]->_uppers;
					}
					varPlace->_cid_list.push_back(_cid);
				}
				if (row[nearCity] && strlen(row[nearCity]) > 0) {
					std::vector<std::string> cid_list;
					ToolFunc::sepString(row[nearCity], "|", cid_list, "");
					varPlace->_cid_list.insert(varPlace->_cid_list.end(),cid_list.begin(),cid_list.end());
				}
				ToolFunc::UniqListOrder(varPlace->_cid_list);

				if (typeI == LY_PLACE_TYPE_VIEW) {
					std::string _rcmd_intensity = "2-1";
					if (row[rcmd_intensity]) _rcmd_intensity = row[rcmd_intensity];
					std::string _intensity = "2-1:900_-1_no";
					if (row[intensity]) _intensity = row[intensity];
					bool ret = FixViewIntensity(varPlace, _rcmd_intensity, _intensity);
					if (!ret) {
						delete varPlace;
						varPlace = NULL;
						varDelIntensity++;
						continue;
					}
				} else {
					FixRestOrShopIntensity(varPlace);
				}

				if (varPlace->_t == LY_PLACE_TYPE_RESTAURANT) {
					if (_id2restaurant_OnlyUsedInLoading.find(varPlace->_ID) != _id2restaurant_OnlyUsedInLoading.end()) {
						ToolFunc::MergeListUniq(varPlace->_cid_list, _id2restaurant_OnlyUsedInLoading[varPlace->_ID]->_cid_list);
						MJ::PrintInfo::PrintLog("LYConstData::loadvarPlaceData, ID(%s) already exist", varPlace->_ID.c_str());
						delete varPlace;
						varPlace = NULL;
						continue;
					}
					_id2restaurant_OnlyUsedInLoading.insert(std::make_pair(varPlace->_ID, varPlace));
				} else {
					if (_id2place.find(varPlace->_ID) != _id2place.end()) {
						ToolFunc::MergeListUniq(varPlace->_cid_list, _id2place[varPlace->_ID]->_cid_list);
						MJ::PrintInfo::PrintLog("LYConstData::loadvarPlaceData, ID(%s) already exist", varPlace->_ID.c_str());
						delete varPlace;
						varPlace = NULL;
						continue;
					}
					_id2place.insert(std::make_pair(varPlace->_ID, varPlace));
				}
				std::vector<std::string>::iterator itCid;
				for (itCid = varPlace->_cid_list.begin(); itCid != varPlace->_cid_list.end();) {
					const City* city = getCity(*itCid);
					if (city == NULL) {
						if (ConstDataCheck::m_needLog) MJ::PrintInfo::PrintLog("[DATA EXCEPTION][CITY ID] city %s not found %s(%s)", (*itCid).c_str(), varPlace->_ID.c_str(), varPlace->_name.c_str());
						itCid = varPlace->_cid_list.erase(itCid);
						continue;
					}
					itCid ++;
				}

				for (int i = 0; i < varPlace->_cid_list.size(); ++i) {
					std::string cid = varPlace->_cid_list[i];
					if (typeI == LY_PLACE_TYPE_VIEW) {
						Insert2CityPlaceMap(cid, varPlace, _cityViews);
					} else if (typeI == LY_PLACE_TYPE_RESTAURANT) {
						Insert2CityPlaceMap(cid, varPlace, _cityRestaurants);
					} else if (typeI == LY_PLACE_TYPE_SHOP) {
						Insert2CityPlaceMap(cid, varPlace, _cityShops);
					}
				}
				++num;
			}
			MJ::PrintInfo::PrintLog("LYConstData::LoadSpecifiedTypeVarPlace, Load type:%s, Num: %d, cost: %d ms", tableName.c_str(), num, timer.cost()/1000);
			MJ::PrintInfo::PrintLog("LYConstData::loadvarPlace, type: %d, OpenClose Num %d, Intensity %d", typeI, varDelOpenClose, varDelIntensity);
		}
		mysql_free_result(res);
	}
	if (typeI == LY_PLACE_TYPE_RESTAURANT) mysql_close(&_varPlaceSQL);
	if (typeI == LY_PLACE_TYPE_VIEW) {
		PerfectVarPlace(_cityViews);
	} else if (typeI == LY_PLACE_TYPE_RESTAURANT) {
		PerfectVarPlace(_cityRestaurants);
	} else if (typeI == LY_PLACE_TYPE_SHOP) {
		PerfectVarPlace(_cityShops);
	}
	return 0;
}

bool LYConstData::FixRestOrShopIntensity(VarPlace* varPlace) {
	int minDur = 0, avgDur = 0, maxDur = 0;
	if (varPlace->_t == LY_PLACE_TYPE_RESTAURANT) {
		//餐厅三档游玩时长
		minDur = 0 * 60;
		avgDur = 1 * 3600;
		maxDur = 1 * 3600;
	} else if (varPlace->_t == LY_PLACE_TYPE_SHOP) {
		//购物三档游玩时长
		minDur = 0.5 * 3600;
		avgDur = 2 * 3600;
		maxDur = 4 * 3600;
	}
	// 不加人工标注时，按规则构造三档强度
	varPlace->_intensity_list.reserve(3);
	varPlace->_intensity_list.push_back(new Intensity("1-1", "拍照就走", minDur, -1, true, "CNY"));
	Intensity* rcmd_intensity = new Intensity("2-1", "简单游玩", avgDur, -1, false, "CNY");
	varPlace->_intensity_list.push_back(rcmd_intensity);
	varPlace->_intensity_list.push_back(new Intensity("3-1", "深度游玩", maxDur, -1, false, "CNY"));
	varPlace->_rcmd_intensity = rcmd_intensity;
	return true;
}
bool LYConstData::LoadIntensity() {
	//load intensity
	std::string sql = "select label,name from intensity";
	int t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("LYConstData::loadIntensity, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			while (row = mysql_fetch_row(res)) {
				if (row[0] == NULL || row[1] == NULL) continue;
				_label_name_map[row[0]] = row[1];
			}
		}
		mysql_free_result(res);
	}
	return true;
}
bool LYConstData::FixViewIntensity(VarPlace* varPlace, std::string rcmdIntensity, std::string intensityStr) {
	std::string errStr = "";
	int ret = ConstDataCheck::CheckIntensity(_label_name_map, rcmdIntensity, intensityStr,  errStr);
	if (!ret) {
		if (ConstDataCheck::m_needLog) MJ::PrintInfo::PrintLog("[DATA EXCEPTION][INTENSITY] %s (%s) [%s(%s)]", std::string(intensityStr).c_str(), errStr.c_str(), varPlace->_ID.c_str(), varPlace->_name.c_str());
		if (ConstDataCheck::m_delIntensity) {
			MJ::PrintInfo::PrintLog("[DATA EXCEPTION][DEL INTENSITY] %s (%s) [%s(%s)]", std::string(intensityStr).c_str(), errStr.c_str(), varPlace->_ID.c_str(), varPlace->_name.c_str());
			return false;
		}
	}
	bool is_rcmd_in_list = false;
	bool intensityErr = false;
	std::vector<std::string> item_list;
	ToolFunc::sepString(intensityStr, "|", item_list);
	for (int i = 0; i < item_list.size(); ++i) {
		std::string item = item_list[i];
		std::string::size_type pos = item.find(":");
		if (pos == std::string::npos) continue;

		std::string label = item.substr(0, pos);
		if (_label_name_map.find(label) == _label_name_map.end()) continue;
		std::string label_name = _label_name_map[label];

		std::vector<std::string> attribute_list;
		ToolFunc::sepString(item.substr(pos + 1), "_", attribute_list);
		if (attribute_list.size() == 3) {
			int dur = atoi(attribute_list[0].c_str());
			if (dur <= 0) {
				intensityErr = true;
				//MJ::PrintInfo::PrintLog("LYConstData::FixViewIntensity, internsity error %s", intensityStr.c_str());
				break;
			}
			double price = atof(attribute_list[1].c_str());
			bool ignore_open = (attribute_list[2] == "yes") ? true: false;
			Intensity* intensity = new Intensity(label, label_name, dur, price, ignore_open);
			varPlace->_intensity_list.push_back(intensity);
			if (label == rcmdIntensity) {
				varPlace->_rcmd_intensity = intensity;
				is_rcmd_in_list = true;
			}
		}
		std::stable_sort(varPlace->_intensity_list.begin(), varPlace->_intensity_list.end(), IntensityDurCmp);
	}
	if (is_rcmd_in_list == false || intensityErr) {
		MJ::PrintInfo::PrintLog("LYConstData::FixViewIntensity, ID(%s) intensity error, skip: %s %s", varPlace->_ID.c_str(), intensityStr.c_str(), rcmdIntensity.c_str());
		return false;
	}
	return true;
}
bool LYConstData::LoadRestaurant() {
	std::string dev ="test",sql;
	if (RouteConfig::mysql_status == "1") dev = "online";
	sql = "select id,name,name_en,map_info,open_time,city_id,nearCity, tag, tag_id, min_price,max_price, '' as rcmd_intensity, '' as intensity, hot_level,grade,ranking,utime from chat_restaurant where status_"+dev+" = 'Open'";
	if (LoadSpecifiedTypeVarPlace("restaurant", sql, LY_PLACE_TYPE_RESTAURANT) == 1) {
		return false;
	}
}

bool LYConstData::LoadVarPlaceData() {
	MJ::PrintInfo::PrintLog("LYConstData::LoadVarPlace...");
	std::tr1::unordered_map<std::string, std::pair<std::string, int>> loadParams;
	//view,restaurant,shop
	//[MYSQL]语句
	std::string dev ="test",sql;
	if (RouteConfig::mysql_status == "1") dev = "online";
	//enum {id,name,name_en,map_info,open,city_id,nearCity,tag,tag_id,min_price,max_price,rcmd_intensity,intensity,hot_level,grade,ranking,utime};
	sql = "select id,name,name_en,map_info,open,city_id,nearCity, tag, tag_id,'' as min_price, '' as max_price, rcmd_intensity,intensity,hot_level,grade,ranking,utime from chat_attraction where status_"+dev+" = 'Open'";
	loadParams["attraction"] = std::make_pair(sql, LY_PLACE_TYPE_VIEW);
	sql = "select id,name,name_en,map_info,open,city_id,nearCity, tag, tag_id, '' as min_price, '' as max_price, '' as rcmd_intensity, '' as intensity, hot_level, grade, ranking,utime from chat_shopping where status_"+dev+" = 'Open'";
	loadParams["shop"] = std::make_pair(sql, LY_PLACE_TYPE_SHOP);

	for(auto it = loadParams.begin(); it != loadParams.end(); it ++) {
		if (LoadSpecifiedTypeVarPlace(it->first, it->second.first, it->second.second) == 1) {
			return false;
		}
	}
	return true;
}

int Cylindrical(double latitude, double longitude, double& x, double& y) {
	double origin_shift = 2 * kPI * 6378137 / 2.0;
	x = longitude * origin_shift / 180.0;
	y = log(tan((90 + latitude) * kPI / 360.0)) / (kPI / 180.0);
	y = y * origin_shift / 180.0;
	return 0;
}

bool LYConstData::PointCylindrical() {
	for (std::tr1::unordered_map<std::string, LYPlace*>::iterator it = _id2place.begin(); it != _id2place.end(); ++it) {
		LYPlace* place = it->second;
		std::string coord_str = place->_poi;
		std::string::size_type pos = coord_str.find(",");
		if (pos == std::string::npos) continue;
		double longitude = atof(coord_str.substr(0, pos).c_str());
		double latitude = atof(coord_str.substr(pos + 1).c_str());
		double x = 0.0;
		double y = 0.0;
		Cylindrical(latitude, longitude, x, y);
		place->_point._x = x;
		place->_point._y = y;
	}
	return true;
}


// 计算两POI间球面距离，单位m
double LYConstData::CaluateSphereDist(const LYPlace* place_a, const LYPlace* place_b) {
	std::string::size_type pos = place_a->_poi.find(",");
	if (pos == std::string::npos) return -1;
	double lon_a = atof(place_a->_poi.substr(0, pos).c_str()) * M_PI / 180;
	double lat_a = atof(place_a->_poi.substr(pos + 1).c_str()) * M_PI / 180;
	pos = place_b->_poi.find(",");
	if (pos == std::string::npos) return -1;
	double lon_b = atof(place_b->_poi.substr(0, pos).c_str()) * M_PI / 180;
	double lat_b = atof(place_b->_poi.substr(pos + 1).c_str()) * M_PI / 180;
	return std::max(0.0, earthRadius * acos(cos(lat_a) * cos(lat_b) * cos(lon_a - lon_b) + sin(lat_a) * sin(lat_b)));
}

bool LYConstData::IsRealID(const std::string& id) {
	if (id == m_segmentPlace->_ID
			|| id == m_attachRest->_ID) {
		return false;
	}
	return true;
}


bool LYConstData::LoadCoreHotel() {
	MJ::PrintInfo::PrintLog("LYConstData::LoadCoreHotel, ...");
	for (std::tr1::unordered_set<std::string>::iterator it = m_citySet.begin(); it != m_citySet.end(); it ++) {
		const City* city = getCity(*it);
		if (city == NULL) {
			continue;
			MJ::PrintInfo::PrintLog("get city fail %s", (*it).c_str());
		}
		LYPlace* coreHotel = NULL;
		std::string id = "coreHotelByMakeCid" + city->_ID;
		std::string coord = city->_poi;
		std::string name = "酒店";
		std::string lname = city->_name + "的酒店";
		coreHotel = MakeHotel(id, name, lname, coord, POI_CUSTOM_MODE_MAKE);
		if (!coreHotel) {
			MJ::PrintInfo::PrintLog("city %s(%s) MakeCoreHotel Fail", city->_ID.c_str(), city->_name.c_str());
			continue;
		}
		MJ::PrintInfo::PrintLog("city %s(%s) useMakeHotel %s(%s)", city->_ID.c_str(), city->_name.c_str(), coreHotel->_ID.c_str(), coreHotel->_name.c_str());
		_id2place.insert(std::make_pair(id, coreHotel));
	}
	return true;
}

LYPlace* LYConstData::MakeView(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	View* view = new View();
	view->_t = LY_PLACE_TYPE_VIEW;
	view->m_custom = custom;
	view->_ID = id;
	view->_name = name;
	view->_lname = lname;
	view->_poi = coord;
	view->_time_rule =  "<*><*><00:00-23:59><SURE>";
	view->_price ="<*><*><成人:0-人民币><SURE>";
	view->_grade = 6.0;
	view->_ranking = 0;
	int hot_level = 1;
	view->SetHotLevel(hot_level);
	view->_intensity_list.push_back(new Intensity("1-1", "拍照就走", 900, -1, true, "CNY"));
	view->_intensity_list.push_back(new Intensity("2-1", "简单游玩", 3600, -1, false, "CNY"));
	view->_intensity_list.push_back(new Intensity("3-1", "深度游玩", 10800, -1, false, "CNY"));
	return dynamic_cast<LYPlace*>(view);
}

LYPlace* LYConstData::MakeShop(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	Shop* shop = new Shop();
	shop->_t = LY_PLACE_TYPE_SHOP;
	shop->m_custom = custom;
	shop->_ID = id;
	shop->_name = name;
	shop->_lname = lname;
	shop->_poi = coord;
	shop->_time_rule =  "<*><*><00:00-23:59><SURE>";
	shop->_price ="<*><*><成人:0-人民币><SURE>";
	shop->_grade = 6.0;
	shop->_ranking = 0;
	int hot_level = 1;
	shop->SetHotLevel(hot_level);
	shop->_intensity_list.push_back(new Intensity("1-1", "拍照就走", 900, -1, true, "CNY"));
	shop->_intensity_list.push_back(new Intensity("2-1", "简单游玩", 3600, -1, false, "CNY"));
	shop->_intensity_list.push_back(new Intensity("3-1", "深度游玩", 10800, -1, false, "CNY"));
	return dynamic_cast<LYPlace*>(shop);
}

LYPlace* LYConstData::MakeRestaurant(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	Restaurant* restaurant = new Restaurant();
	restaurant->_t = LY_PLACE_TYPE_RESTAURANT;
	restaurant->m_custom = custom;
	restaurant->_ID = id;
	restaurant->_name = name;
	restaurant->_lname = lname;
	restaurant->_poi = coord;
	restaurant->_time_rule =  "<*><*><00:00-23:59><SURE>";
	restaurant->_price ="<*><*><成人:0-人民币><SURE>";
	restaurant->_grade = 6.0;
	restaurant->_ranking = 0;
	int hot_level = 1;
	restaurant->SetHotLevel(hot_level);
	restaurant->_intensity_list.push_back(new Intensity("1-1", "拍照就走", 900, -1, true, "CNY"));
	restaurant->_intensity_list.push_back(new Intensity("2-1", "简单游玩", 3600, -1, false, "CNY"));
	restaurant->_intensity_list.push_back(new Intensity("3-1", "深度游玩", 10800, -1, false, "CNY"));
	return dynamic_cast<LYPlace*>(restaurant);
}
LYPlace* LYConstData::MakeHotel(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	Hotel* hotel = new Hotel();
	hotel->_t = LY_PLACE_TYPE_HOTEL;
	hotel->m_custom = custom;
	hotel->_ID = id;
	hotel->_name = name;
	hotel->_lname = lname;
	hotel->_poi = coord;
	return dynamic_cast<LYPlace*>(hotel);
}

LYPlace* LYConstData::MakeStation(const std::string& id,int type, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	Station* station = new Station();
	station->_t = type;
	station->m_custom = custom;
	station->_ID = id;
	station->_name = name;
	station->_lname = lname;
	station->_poi = coord;
	return dynamic_cast<LYPlace*>(station);
}

LYPlace* LYConstData::MakeCarStore(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int custom) {
	CarStore* carStore = new CarStore();
	carStore->_t = LY_PLACE_TYPE_CAR_STORE;
	carStore->m_custom = custom;
	carStore->_ID = id;
	carStore->_name = name;
	carStore->_lname = lname;
	carStore->_poi = coord;
	return dynamic_cast<LYPlace*>(carStore);
}

LYPlace* LYConstData::MakeTour(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int type, int custom) {
	Tour* tour = new Tour();
	tour->_t = type;
	tour->m_custom = custom;
	tour->_ID = id;
	tour->_name = name;
	tour->_lname = lname;
	tour->_poi = coord;
	return dynamic_cast<LYPlace*>(tour);
}

LYPlace* LYConstData::MakeNullPlace(const std::string& id, const std::string& name, const std::string& lname, const std::string& coord, int type, int custom) {
	LYPlace* place = new LYPlace();
	place->_t = type;
	place->m_custom = custom;
	place->_ID = id;
	place->_name = name;
	place->_lname = lname;
	place->_poi = coord;
	return place;
}

const LYPlace* LYConstData::GetCoreHotel(const std::string& cid) {
	PrivateConstData* priData = GetPriDataPtr();
	std::string id = "coreHotelByMakeCid" + cid;
	const LYPlace * hotel = getBaseLYPlace(id);
	if (!hotel) {
		//私有城市
		MJ::PrintInfo::PrintLog("LYConstData::GetCoreHotel getCoreHotel begin for private city %s", cid.c_str());
		hotel = priData->GetLYPlace(id,"");
		if (hotel)
			MJ::PrintInfo::PrintLog("LYConstData::GetCoreHotel getCoreHotel success for private city %s", cid.c_str());

	}
	return hotel;
}


const std::string LYConstData::GetCoreHotelCoord(const std::string& cid) {
	const LYPlace* hotel = GetCoreHotel(cid);
	if (hotel) {
		return hotel->_poi;
	}
	return "";
}

bool LYConstData::IsParkCity(const std::string& cityId) {
	if (m_parkCity.find(cityId) != m_parkCity.end()) {
		return true;
	}
	return false;
}

bool LYConstData::Insert2CityPlaceMap(const std::string& cid, const LYPlace* place, std::tr1::unordered_map<std::string, std::set<std::string>>& cid2PlaceList) {
	cid2PlaceList[cid].insert(place->_ID);

	return true;
}

bool LYConstData::loadStagData(){
	MJ::PrintInfo::PrintLog("LYConstData::loadStagData...");
	enum					 {id,type,name,name_en};
	string attr_sql = "select id,'" + std::to_string(LY_PLACE_TYPE_VIEW) + "' as type, tag, tag_en from chat_attraction_tagS where id is not null";
	string shop_sql = "select id,'" + std::to_string(LY_PLACE_TYPE_SHOP) + "' as type, tag, tag_en from chat_shopping_tagS where id is not null";
	string rest_sql = "select id,'" + std::to_string(LY_PLACE_TYPE_RESTAURANT) + "' as type, tag, tag_en from chat_restaurant_tagS where id is not null";
	string sql = attr_sql + " union " + shop_sql + " union " + rest_sql;

	int t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("LYConstData::loadStagData, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[name] == NULL) continue;
				int poiType = atoi(row[type]);
				if (m_poiType2smallTagNameAndId[poiType].find(row[name]) != m_poiType2smallTagNameAndId[poiType].end()) {
					MJ::PrintInfo::PrintErr("LYConstData::loadStagData id :%s name:%s enname:%s type:%d ", row[id], row[name], row[name_en], poiType);
					continue;
				}
				m_poiType2smallTagNameAndId[poiType][row[name]] = row[id];
				++num;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadStagData,Num: %d", num);
		}
		mysql_free_result(res);
	}
	return true;
}

std::string LYConstData::GetTagStrByName(const std::string &name) {
	int type = LY_PLACE_TYPE_VIEW;
	auto ii = m_poiType2smallTagNameAndId[type].find(name);
	if (ii != m_poiType2smallTagNameAndId[type].end()) {
		return ii->second;
	} else {
		return "";
	}
}

//加载Tag相关信息
bool LYConstData::loadTag() {
	MJ::PrintInfo::PrintLog("LYConstData::loadTag, loading Tag Data...");
	enum					{ id, tag_id};
	std::string sql = "select id,tag_id from tag";
	int t = mysql_query(&_mysql, sql.c_str());
	if (t != 0) {
		MJ::PrintInfo::PrintLog("LYConstData::loadTagData, mysql_query error: %s error sql: %s", mysql_error(&_mysql), sql.c_str());
		return false;
	} else {
		MYSQL_RES* res = mysql_use_result(&_mysql);
		MYSQL_ROW row;
		if (res) {
			int num = 0;
			while (row = mysql_fetch_row(res)) {
				if (row[id] == NULL) continue;
				if (row[tag_id] == NULL) continue;
				std::string tagIdStr = row[tag_id];
				if (tagIdStr.find("tag") == std::string::npos) continue;
				_baseTagIds.insert(tagIdStr);
				num++;
			}
			MJ::PrintInfo::PrintLog("LYConstData::loadTagData, Load tag Num: %d", num);
		}
		mysql_free_result(res);
	}
	return true;
}

int LYConstData::GetTourByType(const std::string &cityID, std::set<std::string> &TourList, int type, const std::string& ptid) {
	auto it = _cityTours.find(cityID);
	if (it == _cityTours.end()) {
		MJ::PrintInfo::PrintErr("LYConstData::getTourByType, can not find ID: %s", cityID.c_str());
		return 1;
	}
	auto tourList = it->second;
	for (auto _it = tourList.begin(); _it != tourList.end(); _it++) {
		std::string id = *_it;
		const LYPlace* place = getBaseLYPlace(id);
		if (place->_t & type) {
			TourList.insert(id);
		}
	}
	return 0;
}

int LYConstData::GetTourListByType (const std::string &cityID, std::vector<const LYPlace*>& TourList, int type, const std::string& ptid) {
	std::set<std::string> baseTourIds, priTourIds, totalIds;
	GetTourByType(cityID, baseTourIds, type, ptid);
	totalIds.insert(baseTourIds.begin(), baseTourIds.end());
	//ShieldPlaceList 在privateConstData中调用
	const QueryParam* qp=GetQueryParam();
	if(qp){
		PrivateConstData* priData = GetPriDataPtr();
		priData->GetPrivateTourByType(cityID, priTourIds, type, qp->ptid);
		totalIds.insert(priTourIds.begin(), priTourIds.end());

		MJ::MJDataProc tDP;
		std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string> > retData;
		tDP.getDistrProduct_Wanle(qp->ptid,getRedisHandle(),retData);
		for(auto it = retData.begin(); it != retData.end(); it++)
		{
			totalIds.insert(it->second.begin(),it->second.end());
		}
	}
	for(auto it = totalIds.begin(); it!=totalIds.end(); it++)
	{
		auto place = GetLYPlace(*it,ptid);
		if(place and place->_t & type and count(place->_cid_list.begin(),place->_cid_list.end(),cityID))
		{
			TourList.push_back(place);
		}
	}

	return 0;
}

int LYConstData::GetConstTickets(const std::string pid, std::string ticketId, std::vector<std::vector<std::string> > &results) {
	if (ticketId != "") {
		int tId = std::stoi(ticketId);
		if (_id2ProdTickets.find(tId) != _id2ProdTickets.end()) {
			const TicketsFun* tf = _id2ProdTickets[tId];
			std::string id, info;
			std::stringstream sstream;
			sstream << tf->m_id;
			sstream >> id;
			Json::FastWriter jw;
			info = jw.write(tf->m_info);
			std::string tfList[] = {id,tf->m_ticket_id,tf->m_pid,tf->m_name,info,tf->m_ccy,tf->m_ticket_3rd};
			std::vector<std::string> result(tfList, tfList+sizeof(tfList)/sizeof(string)) ;
			results.push_back(result);
		}
	} else {
		if (_pid2ticketsId.find(pid) != _pid2ticketsId.end()) {
			std::tr1::unordered_set<int>& ticketsSet = _pid2ticketsId[pid];
			for(auto it = ticketsSet.begin(); it != ticketsSet.end(); it++) {
				if (_id2ProdTickets.find(*it) != _id2ProdTickets.end()) {
					const TicketsFun* tf = _id2ProdTickets[*it];
					std::string id, info;
					std::stringstream sstream;
					sstream << tf->m_id;
					sstream >> id;
					Json::FastWriter jw;
					info = jw.write(tf->m_info);
					std::string tfList[] = {id,tf->m_ticket_id,tf->m_pid,tf->m_name,info,tf->m_ccy,tf->m_ticket_3rd};
					std::vector<std::string> result(tfList, tfList+sizeof(tfList)/sizeof(string)) ;
					results.push_back(result);
				}
			}
		}
	}
	return 0;
}
int LYConstData::GetPrivateData(const std::string &sql, std::vector<std::vector<std::string> > &results) {
	PrivateConstData* priData = GetPriDataPtr();
	return priData->GetPrivateData(sql, results);
}

int LYConstData::GetProdTicketsByPlaceAndId(const LYPlace* place, const int& id, const TicketsFun*& tickets) {
	if (place->m_custom == POI_CUSTOM_MODE_CONST) {
		if (_id2ProdTickets.find(id) != _id2ProdTickets.end()) {
			tickets = _id2ProdTickets[id];
			return 0;
		}
	} else {
		PrivateConstData* priData = GetPriDataPtr();
		return priData->GetProdTicketsById(id, tickets);
	}
	return 1;
}

int LYConstData::GetProdTicketsListByPlace(const LYPlace* place, std::vector<const TicketsFun*>& tickets) {
	std::string pid = place->_ID;
	if (place->m_custom == POI_CUSTOM_MODE_CONST) {
		GetProdTicketsListByPid(pid,tickets);
	} else {
		PrivateConstData* priData = GetPriDataPtr();
		//现阶段玩乐产品所属的企业和票所属的企业是一致的
		priData->GetProdTicketsListByPid(pid, tickets);
	}
	const QueryParam* qp=GetQueryParam();
	if(qp)
	{
		auto sources = GetAvailableSources(qp->ptid);
		auto pidSources = GetAvailableSources(place->m_ptid);
		sources.insert(pidSources.begin(),pidSources.end());
		for(auto it=tickets.begin();it!=tickets.end();)
		{
			if(not sources.count((*it)->m_sid))
			{
				it=tickets.erase(it);
			}
			else it++;
		}
	}
	return 0;
}

int LYConstData::GetProdTicketsListByPid(const std::string& pid, std::vector<const TicketsFun*>& tickets) {
	std::tr1::unordered_map<int,const TicketsFun*>::const_iterator cit;
	for(cit = _id2ProdTickets.begin(); cit != _id2ProdTickets.end(); cit++) {
		if (cit->second == NULL) {
			MJ::PrintInfo::PrintErr("LYConstData::GetProdTicketsListByPid, no product ticket, ticket id: %s", cit->first);
			continue;
		}
		if(cit->second->m_pid == pid) {
			tickets.push_back(cit->second);
		}
	}
	return 0;
}


int LYConstData::MakeTicketSource(const Tour *tour, const std::string& ticketDate, const TicketsFun *ticket, Json::Value &source) {
	if (tour == NULL || ticket == NULL) {
		return 1;
	}
	source = Json::Value();
	//通用必填字段
	if (tour->m_custom == POI_CUSTOM_MODE_CONST) {
		//if (IsApi(tour->m_sid)) {
		//	source["sourceType"] = 2;
		//} else {
		//	source["sourceType"] = 1;
		//}
		source["sourceType"] = 2;
	} else if (tour->m_custom == POI_CUSTOM_MODE_PRIVATE) {
		source["sourceType"] = 3;
	}
	source["utime"] = int(MJ::MyTime::getNow());
	source["product"] = 0x80;
	source["sid"] = tour->m_sid;
	source["sname"] = GetSourceNameBySid(tour->m_sid);
	source["miojiBuy"] = 0;
	source["refund"] = 1;
	source["apiAcc"] = 1;

	//玩乐通用字段 unionkey为通用必填字段
	source["pid"] = tour->m_pid;
	source["name"] = tour->_name;
	source["lname"] = tour->_enname;
	if (tour->m_pid_3rd != "") source["pid_3rd"] = tour->m_pid_3rd;
	source["book_pre"] = tour->m_preBook;
	source["times"] = tour->m_srcTimes;
	Json::FastWriter jw;
	std::string date = "";
	if (tour->m_open.isArray() && tour->m_open.size()>0) date = jw.write(tour->m_open);
	source["date"] = date;
	source["disable"] = 0;

	source["t_apply"] = ticket->m_info;
	int num = 0;
	for (int k = 0; k < source["t_apply"]["info"].size(); ++k) {
		if (source["t_apply"]["info"][k]["num"].isInt()) {
			num += source["t_apply"]["info"][k]["num"].asInt();
		}
	}
	//保证unionkey唯一
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long timeofusec = tv.tv_sec*1000000 + tv.tv_usec;
	srand(time(NULL));

	source["t_num"] = num;
	source["ticketId"] = ticket->m_id;
	source["ticket_id"] = ticket->m_ticket_id;
	source["t_name"] = ticket->m_name;
	source["realPrice"]["ccy"] = ticket->m_ccy;
	source["unionkey"] = tour->m_pid + "#" + source["ticketId"].asString() + "#" + std::to_string(timeofusec) + "#" + std::to_string(rand());
	source["ticket_3rd"] = ticket->m_ticket_3rd;

	float price = 0;
	if (GetTicketPriceOfTheDate(ticket, ticketDate, price)) {
		std::stringstream s;
		s << std::setiosflags(std::ios::fixed)<<std::setprecision(2) << price;
		std::string str;
		s >> str;
		source["realPrice"]["val"] = str;
	} else {
		source["realPrice"]["val"] = "-1";
	}

	return 0;
}

std::string LYConstData::GetSourceNameBySid(const std::string& sid) {
	if (_sourceId2Name.find(sid) != _sourceId2Name.end()) {
		return _sourceId2Name[sid];
	}
	PrivateConstData* priData = GetPriDataPtr();
	return priData->GetSourceNameBySid(sid);
}

int LYConstData::IsApi(const std::string& sid) {
	if (_sourceId2Api.find(sid) != _sourceId2Api.end()) {
		return _sourceId2Api[sid];
	}
	return 0;
}

MJ::MyRedis* LYConstData::getRedisHandle(){
	if(_redis.get()==NULL)
	{
		_redis.reset(new MJ::MyRedis);
		if(not _redis.get()->init(RouteConfig::dc_redis_address,RouteConfig::dc_redis_passwd,RouteConfig::dc_redis_db))
		{
			return NULL;
		}
	}
	return _redis.get();
}

std::tr1::unordered_set<std::string> LYConstData::GetAvailableSources(const std::string & ptid){
	QueryParam* qp=const_cast<QueryParam *>(GetQueryParam());
	std::string key = qp->qid+"_"+ptid;
	if(qp and qp->available_sources.count(key)) return qp->available_sources[key];
	MJ::MyRedis* redisCon = getRedisHandle();
	std::tr1::unordered_set<std::string> sources;
	MJ::MJDataProc tDP;
	tDP.getAllSID(ptid,redisCon,sources);
	if(qp) qp->available_sources[key]=sources;

	return sources;
}

bool LYConstData::IsSourceAvailable(const string & ptid,const string & source){
	auto sources =GetAvailableSources(ptid);
	return sources.count(source);
}


//判断玩乐当前日期是否有票
bool LYConstData::IsTourHasTicketofTheDate (const LYPlace* tour, const std::string& date, const TicketsFun*& ticketFun) {
	if (tour == NULL) return false;
	std::vector<const TicketsFun* > tickets;
	GetProdTicketsListByPlace(tour, tickets);
	float low_price = std::numeric_limits<float>::max();
	for (int i = 0; i < tickets.size(); i++) {
		const TicketsFun* ticket = tickets[i];
		float price = 0;
		if (GetTicketPriceOfTheDate(ticket, date, price)) {
			if (ticket->m_ccy != "CNY") price = ToolFunc::RateExchange::curConv(price, ticket->m_ccy);
			if (price < low_price) {
				low_price = price;
				ticketFun = ticket;
			}
		}
	}
	if (ticketFun != NULL) {
		return true;
	}
	return false;
}
//最低票价的票 bottomTicket 及 最低price
bool LYConstData::GetBottomTicketAndPrice (const LYPlace* tour, const TicketsFun *& bottomTicket, float& bottomPrice) {
	bottomTicket = NULL;
	if (tour == NULL) return false;
	std::vector<const TicketsFun* > tickets;
	GetProdTicketsListByPlace(tour, tickets);
	float low_price = std::numeric_limits<float>::max();
	bottomPrice = low_price;
	//中间比较的价格
	float tmp_bottomPrice = low_price;
	float tmp_low_price = low_price;
	std::string date = "";
	for (int i = 0; i < tickets.size(); i++) {
		const TicketsFun* ticket = tickets[i];
		if (GetTicketPriceOfTheDate(ticket, date, low_price)) {
			tmp_low_price = ToolFunc::RateExchange::curConv(low_price, ticket->m_ccy);
			if (tmp_bottomPrice > tmp_low_price) {
				bottomPrice = low_price;
				bottomTicket = ticket;
				tmp_bottomPrice = tmp_low_price;
			}
		}
	}
	if (bottomTicket) return true;
	return false;
}
//date=="" 拿到的是最低票价
//玩乐当前日期的报价 无报价时不会返回票种
bool LYConstData::GetTicketPriceOfTheDate (const TicketsFun* ticket, const std::string& date, float& low_price) {
	bool hasTicket = false;
	if (ticket == NULL) return hasTicket;
	for (int j = 0; j < ticket->m_date_price.size(); j++) {
		const Json::Value& jPrice = ticket->m_date_price[j];
		if (jPrice.isMember("f") && jPrice["f"].isString()
				&& jPrice.isMember("t") && jPrice["t"].isString()
				&& jPrice.isMember("p") && jPrice["p"].isDouble()) {
			if (date <= jPrice["t"].asString() && date >= jPrice["f"].asString()) {
				if (jPrice.isMember("w") && jPrice["w"].isArray()) {
					int wd = TimeIR::getWeekByDate(date);
					auto it = std::find(jPrice["w"].begin(), jPrice["w"].end(), wd);
					if (it != jPrice["w"].end()) {
						low_price = jPrice["p"].asDouble();
						hasTicket = true;
					}
				} else {
					low_price = jPrice["p"].asDouble();
					hasTicket = true;
				}
			} else if (date == "") {
				if (low_price > jPrice["p"].asDouble()) {
					low_price = jPrice["p"].asDouble();
				}
				hasTicket = true;
			}
		}
	}
	return hasTicket;
}
//玩乐当前日期可用
bool LYConstData::IsTourAvailable(const Tour* tour, const std::string& date) {
	if (!tour->m_open.isArray()) return false;
	return TimeIR::isTheDateAvailable(tour->m_open, date);
}

