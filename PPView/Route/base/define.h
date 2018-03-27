#ifndef __LY_DEFINE_H__
#define __LY_DEFINE_H__

#define LAST_DEST_PLACE 10000
#define CITY_LAST_TIME 255
#define IMPOSSIBLE_PATH_SCORE -9999999
#define ERR_THRES 0.00001

extern int __debug_level__;
extern int __runtime_error__;

#define RUNMODE_USER 0
#define RUNMODE_TEST 1
#define RUNMODE_MANU 2
#define RUNMODE_TEST_3002 3

#define USER_PREF_INTENSITY_HIGH 3
#define USER_PREF_INTENSITY_NORMAL 2
#define USER_PREF_INTENSITY_LOW 1


#define MULTIUSE_TYPE_MULT 1
#define MULTIUSE_TYPE_UNIQ 0

#include <string>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <iomanip>
#include <map>
#include <set>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <cmath>
#include "MJCommon.h"
#include "ToolFunc.h"
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <queue>
#include <bitset>

//定义固化数据的前缀
const std::string CDATA_PREFIX_CD = "CD:";
const std::string CDATA_PREFIX_PI = "PI:";
const std::string CDATA_PREFIX_TI = "TI:";
const std::string CDATA_PREFIX_JD = "JD:";

typedef std::bitset<160> bit160;

const int LY_PLACE_TYPE_NULL = 0x00000000;
const int LY_PLACE_TYPE_CITY = 0x00000001;
const int LY_PLACE_TYPE_VIEW = 0x00000002;
const int LY_PLACE_TYPE_HOTEL = 0x00000004;
const int LY_PLACE_TYPE_RESTAURANT = 0x00000008;
const int LY_PLACE_TYPE_AIRPORT = 0x00000010;
const int LY_PLACE_TYPE_STATION = 0x00000020;
const int LY_PLACE_TYPE_CAR_STORE = 0x00000040;
const int LY_PLACE_TYPE_BUS_STATION = 0x00000080;
const int LY_PLACE_TYPE_SAIL_STATION = 0x00002000;//没有数字用了····
const int LY_PLACE_TYPE_ARRIVE = LY_PLACE_TYPE_AIRPORT | LY_PLACE_TYPE_STATION | LY_PLACE_TYPE_BUS_STATION | LY_PLACE_TYPE_SAIL_STATION;
const int LY_PLACE_TYPE_SHOP = 0x00000100;
const int LY_PLACE_TYPE_BLANK = 0x00001000;
const int LY_PLACE_TYPE_VIEWTICKET = 0x00004000;	//景点门票
const int LY_PLACE_TYPE_PLAY = 0x00008000;			//演出
const int LY_PLACE_TYPE_ACTIVITY = 0x00010000;	//活动
const int LY_PLACE_TYPE_TOURALL = 0x00004000|0x00008000|0x00010000;

const int LY_PLACE_TYPE_VAR_PLACE = LY_PLACE_TYPE_VIEW | LY_PLACE_TYPE_RESTAURANT | LY_PLACE_TYPE_SHOP | LY_PLACE_TYPE_TOURALL;
const int LY_PLACE_TYPE_ALL = 0xFFFFFFFF;
const int LY_PLACE_TYPE_VIEWSHOP = LY_PLACE_TYPE_SHOP | LY_PLACE_TYPE_VIEW;

const int RESTAURANT_TYPE_NULL = 0x00000000;  // 按自定义开关门，视为景点对待
const int RESTAURANT_TYPE_BREAKFAST = 0x00000001; // 受早餐时间限制，类推
const int RESTAURANT_TYPE_LUNCH = 0x00000002;
const int RESTAURANT_TYPE_AFTERNOON_TEA = 0x00000004;
const int RESTAURANT_TYPE_SUPPER = 0x00000008;
const int RESTAURANT_TYPE_COUNT = 0x00000010;
const int RESTAURANT_TYPE_ALL = RESTAURANT_TYPE_BREAKFAST | RESTAURANT_TYPE_LUNCH | RESTAURANT_TYPE_AFTERNOON_TEA | RESTAURANT_TYPE_SUPPER;

const int ATTACH_TYPE_NULL = RESTAURANT_TYPE_NULL;
const int ATTACH_TYPE_LUNCH = RESTAURANT_TYPE_LUNCH;
const int ATTACH_TYPE_SUPPER = RESTAURANT_TYPE_SUPPER;
const int ATTACH_TYPE_ALL = ATTACH_TYPE_LUNCH | ATTACH_TYPE_SUPPER;

const int TIMEOUT = 500000000;

// m_ValidHotel状态，>0正常，0初始值
const int HOTEL_NONEXIST = -1;  // id不存在
const int HOTEL_TIMEOUT = -2;  // 请求接口超时
const int HOTEL_NODATA = -3;  // 接口无结果
const int HOTEL_OK = 1;  // hotel正常

// 景点level 需与线下做数据值统一
const int VIEW_LEVEL_NULL = 0;
const int VIEW_LEVEL_SYSOPT = 1;  // 前15%

const int VIEW_BAN_NULL = 0x00000000;
const int VIEW_BAN_USER = 0x00000001;  // 用户指定不选
const int VIEW_BAN_ATTACHED = 0x00000002;  // 附属不选
const int VIEW_BAN_FAR = 0x00000004;  // 偏远不选
const int VIEW_BAN_CLOSE = 0x00000008;  // 关门不选
const int VIEW_BAN_LACK_TIME = 0x00000010;  // 时间不够不选
const int VIEW_BAN_MUTEX = 0x00000020;  // 景点互斥

const int TAG_BITSET_SIZE = 0x00000200;//tag的容量

const int FAR_DIST_LIMIT = 40000;

enum VISITED_TYPE {
	VISITED_BAN,
	VISITED_VALID,
	VISITED_OCCUR
};

// 交通方式
enum TRAF_MODE {
	TRAF_MODE_TAXI,  // 0 打车
	TRAF_MODE_WALKING,  // 1 步行
	TRAF_MODE_BUS,  // 2 公共交通
	TRAF_MODE_UBER,  // 3 uber
	TRAF_MODE_DRIVING  // 4 自驾(租车)
};

enum TRAF_STAT {
	TRAF_STAT_STATIC,  // 0 静态交通
	TRAF_STAT_REAL_TIME  // 1 实时交通
};

enum SHOP_INTENSITY_TYPE {
	SHOP_INTENSITY_NULL,  // 不购物
	SHOP_INTENSITY_LOOK_AROUND,  // 随便看看
	SHOP_INTENSITY_SHOPAHOLIC  // 购物狂
};

// 主从区分
enum GROUP_TYPE {
	GROUP_TYPE_MASTER,
	GROUP_TYPE_SLAVE
};

// 查询来源
enum QUERY_SOURCE_TYPE {
	QUERY_SOURCE_WEB,
	QUERY_SOURCE_IOS,
	QUERY_SOURCE_ANDORID
};

// 时间强度
enum TIME_INTENSITY {
	TIME_INTENSITY_LOOSE,
	TIME_INTENSITY_SUITABLE,
	TIME_INTENSITY_TIGHT,
	TIME_INTENSITY_INSUFFISCIENT
};
//行程类型 0 常规 1 包车
enum TRAVAL_MODE {
	TRAVAL_MODE_NORMAL,
	TRAVAL_MODE_CHARTER_CAR
};

enum POI_CUSTOM_MODE {
	POI_CUSTOM_MODE_CONST, //共有库
	POI_CUSTOM_MODE_CUSTOM, //自定义
	POI_CUSTOM_MODE_MAKE, //自制点
	POI_CUSTOM_MODE_PRIVATE // 私有库
};

const int REST_PREFER_NULL = 0x00000000;  // 无饭店
const int REST_PREFER_ATTACH = 0x01000000;  // 附近就餐
const int REST_PREFER_SNACK = 0x00000001;  // 特色小馆
const int REST_PREFER_FEAST = 0x00000002;  // 美味大餐
const int REST_PREFER_MICHELIN = 0x00000004;  // 米其林
const int REST_PREFER_AI = REST_PREFER_ATTACH | REST_PREFER_SNACK | REST_PREFER_FEAST | REST_PREFER_MICHELIN;

const int TRAF_PREFER_NULL = 0x00000000;
const int TRAF_PREFER_TAXI = 0x00000001;  // 打车
const int TRAF_PREFER_WALKING = 0x00000002;  // 步行
const int TRAF_PREFER_BUS = 0x00000004;  // 公共交通
const int TRAF_PREFER_UBER = 0x00000008;  // uber
const int TRAF_PREFER_DRIVING = 0x00000010;  // 自驾
const int TRAF_PREFER_CAR = TRAF_PREFER_TAXI | TRAF_PREFER_UBER | TRAF_PREFER_DRIVING;
const int TRAF_PREFER_AI = TRAF_PREFER_CAR | TRAF_PREFER_WALKING | TRAF_PREFER_BUS;

const int SHOP_PREFER_NULL = 0x00000000;  // 无购物
const int SHOP_PREFER_OUTLETS = 0x00000001;  // 奥特莱斯
const int SHOP_PREFER_MALL = 0x00000002;  // 大型商场
const int SHOP_PREFER_MARKET = 0x00000004;  // 当地市集
const int SHOP_PREFER_AI = SHOP_PREFER_OUTLETS | SHOP_PREFER_MALL | SHOP_PREFER_MARKET;

// node功能
const unsigned int NODE_FUNC_NULL = 0x00000000;
//景点 购物 餐厅 类型不能超过8位 在bag中使用
const uint8_t NODE_FUNC_PLACE_REST_BREAKFAST = 0x00000001;
const uint8_t NODE_FUNC_PLACE_REST_LUNCH = 0x00000002;
const uint8_t NODE_FUNC_PLACE_REST_AFTERNOON_TEA = 0x00000004;
const uint8_t NODE_FUNC_PLACE_REST_SUPPER = 0x00000008;
const uint8_t NODE_FUNC_PLACE_RESTAURANT = NODE_FUNC_PLACE_REST_BREAKFAST | NODE_FUNC_PLACE_REST_LUNCH | NODE_FUNC_PLACE_REST_AFTERNOON_TEA | NODE_FUNC_PLACE_REST_SUPPER;
const uint8_t NODE_FUNC_PLACE_VIEW = 0x00000010;
const uint8_t NODE_FUNC_PLACE_SHOP = 0x00000020;
const uint8_t NODE_FUNC_PLACE_VIEW_SHOP = NODE_FUNC_PLACE_VIEW | NODE_FUNC_PLACE_SHOP;
const unsigned int NODE_FUNC_PLACE_TOUR = 0x00000040;
const unsigned int NODE_FUNC_PLACE_GETCAR_STORE = 0x00000100;
const unsigned int NODE_FUNC_PLACE_RETURNCAR_STORE = 0x00000200;
const unsigned int NODE_FUNC_PLACE_CAR_STORE = NODE_FUNC_PLACE_GETCAR_STORE | NODE_FUNC_PLACE_RETURNCAR_STORE;
const unsigned int NODE_FUNC_PLACE = NODE_FUNC_PLACE_VIEW | NODE_FUNC_PLACE_SHOP | NODE_FUNC_PLACE_RESTAURANT | NODE_FUNC_PLACE_TOUR;

const unsigned int NODE_FUNC_KEY_ARRIVE = 0x00001000;
const unsigned int NODE_FUNC_KEY_DEPART = 0x00002000;
const unsigned int NODE_FUNC_KEY_STATION = NODE_FUNC_KEY_ARRIVE | NODE_FUNC_KEY_DEPART;
const unsigned int NODE_FUNC_KEY_HOTEL_SLEEP = 0x00010000;
const unsigned int NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE = 0x00020000;
const unsigned int NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE = 0x00040000;
const unsigned int NODE_FUNC_KEY_HOTEL_LUGGAGE = NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE | NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE;
const unsigned int NODE_FUNC_KEY_HOTEL = NODE_FUNC_KEY_HOTEL_SLEEP | NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE | NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE;
const unsigned int NODE_FUNC_KEY_SEGMENT = 0x001000000;
const unsigned int NODE_FUNC_KEY = NODE_FUNC_KEY_STATION | NODE_FUNC_KEY_SEGMENT | NODE_FUNC_KEY_HOTEL;

const unsigned int NODE_FUNC_PLACE_HOTEL_SLEEP = 0x00080000; //不作为keyNode的酒店点 (目前无用)

const unsigned int NODE_FUNC_ALL = 0xFFFFFFFF;

const unsigned int REAL_TRAF_NULL = 0x00000000;
const unsigned int REAL_TRAF_ADVANCE = 0x00000001;  // 打TrafPair阶段获取
const unsigned int REAL_TRAF_REPLACE = 0x00000002;  // PathView出来后后补

class ViewType {
public:
	ViewType(const std::string& id, const std::string& name, int code) {
		_id = id;
		_name = name;
		_code = code;
	}
public:
	std::string _id;
	std::string _name;
	int _code;
};

// 平面坐标点
class Point {
public:
	Point() {
		_x = 0.0;
		_y = 0.0;
	}
	Point(double x, double y) {
		_x = x;
		_y = y;
	}
public:
	double _x;
	double _y;
};

// 景点强度
class Intensity {
public:
	Intensity() {
		_label = "default";
		_name = "默认";
		_dur = 3600;
		_price = 100;
		_ignore_open = false;
	}
	Intensity(const std::string& label, const std::string& name, int dur, double price, bool ignore_open, const std::string& currency_code="CNY") {
		_label = label;
		_name = name;
		_dur = dur;
		_price = price;
		_ignore_open = ignore_open;
	}
	~Intensity() {
	}
	int SetDur(int dur) {
		_dur = dur;
	}
public:
	std::string _label;
	std::string _name;
	int _dur;
	double _price;
	bool _ignore_open;
};

class LYPlace {
public:
	LYPlace() {
		_time_zone = 0;
		_dur = 0;
		_t = LY_PLACE_TYPE_NULL;//类型 1:city 2:景点 3:酒店 4:车站（包括机场 火车站, 码头）
		m_custom = POI_CUSTOM_MODE_CONST;
		m_mode = -1;//无效值
		_rawPlace = NULL;
		_rec_priority = 0; //yc
	}
	virtual std::string getUniqId() const{
		return _ID;
	}
	virtual ~LYPlace() {
	}
public:
	std::string _img;	//图片
	std::string _poi;
	std::string _name;
	std::string _enname;
	std::string _lname;		//local name
	std::string _ID;
	std::string _pid;
	std::string _unionkey;
	std::string _corp;
	double _time_zone;
	int _t;		//类型 1:city 2:景点 3:酒店 4:车站（包括机场 火车站, 码头）
	std::vector<std::string> _cid_list;
	std::tr1::unordered_set<std::string> m_tag;
	std::string m_tagSmallStr;
	Point _point;  // 映射到谷歌平面坐标系上的坐标
	std::string _sid;
	int m_custom;
	std::string m_ptid;//私有库来源
	std::string m_refer; //私有来源时 引用静态库的id ，可为空
	int m_mode; //辅助信息;当地址为租车门店时,0代表取车门店,1代表换车门店
	int _dur;//暂时用来传递租车或还车所用时间
	const LYPlace * _rawPlace; //伪装的真实的点
	int _rec_priority; //yc
    std::string _utime;
public:
	int getRawType() const
	{
		if(_rawPlace) return _rawPlace->_t;
		else return _t;
	}
	bool setMode(int mode){
		m_mode = mode;
		return true;
	}
	int getMode(){
		return m_mode;
	}
	int dump(const std::string& uID, const std::string& wID) {
		MJ::PrintInfo::PrintDbg("[%s][%s]LYPlace::dump, %s(%s,%d)", uID.c_str(), wID.c_str(), _ID.c_str(), _name.c_str(),m_mode);
		return 0;
	}
};


class City: public LYPlace {
public:
	double _hot;
	std::string _country;
	//_uppers 记录包含当前城市的更大一级别的城市
	std::vector<std::string> _uppers;
};

class Station: public LYPlace {
};

class CarStore: public LYPlace {
};

class Hotel: public LYPlace {
public:
	virtual std::string getUniqId() const{
		//公转私的点名字有变化
		if(not m_refer.empty() and not m_ptid.empty()) return _ID+"_"+m_ptid;
		else return _ID;
	}
public:
	int _lvl;  // 星级
	std::string _tags;
// 由垂直接口来填
//	std::string _service;   //设施
	double _viewDis;  // 离city中心景点距离
	int dump(const std::string& uID, const std::string& wID) {
		MJ::PrintInfo::PrintDbg("[%s][%s]Hotel::dump, name:%s(%s), tags:%s", uID.c_str(), wID.c_str(), _ID.c_str(), _name.c_str(), _tags.c_str());
		return 0;
	}
};


// 可变place，包括 view restaurant shop
class VarPlace: public LYPlace {
public:
	VarPlace() {
		_intensity_list.resize(0);
		_rcmd_intensity = NULL;
		_hot_level = 1;
		_grade = 0;
		_level = 0;
		_ranking = 0;
		avgPrice = 0;
	}
	VarPlace(const VarPlace& vPlace):LYPlace(vPlace) {
		_hot_rank = vPlace._hot_rank;
		_tags = vPlace._tags;
		_time_rule = vPlace._time_rule;
		_price = vPlace._price;
		_hot_level = vPlace._hot_level;
		_grade = vPlace._grade;
		_level = vPlace._level;
		_ranking = vPlace._ranking;
		avgPrice = vPlace.avgPrice;
		// 强度
		_intensity_list.resize(0);
		_rcmd_intensity = NULL;
		for (int i = 0 ; i < vPlace._intensity_list.size(); i ++) {
			if (!vPlace._intensity_list[i]) {
				continue;
			}
			Intensity* intensity = new Intensity(*vPlace._intensity_list[i]);
			_intensity_list.push_back(intensity);
		}
		if (vPlace._rcmd_intensity) {
			_rcmd_intensity = new Intensity(*vPlace._rcmd_intensity);
		}
	}
	virtual ~VarPlace() {
		std::tr1::unordered_set<Intensity*> intensity_set(_intensity_list.begin(), _intensity_list.end());
		intensity_set.insert(_rcmd_intensity);
		for (std::tr1::unordered_set<Intensity*>::iterator it = intensity_set.begin(); it != intensity_set.end(); ++it) {
			if (*it) {
				delete *it;
			}
		}
		intensity_set.clear();
		_intensity_list.clear();
		_rcmd_intensity = NULL;
	}
public:
	double _hot_rank;
	// 强度
	std::vector<Intensity*> _intensity_list;
	Intensity* _rcmd_intensity;

	// 热度
	double _hot_level;
	std::string _tags;
	// 开关门规则
	std::string _time_rule;
	// 价格规则
	std::string _price;
	double _grade;
	int _level;
	int avgPrice;
	int _ranking;
public:
	int dump(const std::string& uID, const std::string& wID) {
		MJ::PrintInfo::PrintDbg("[%s][%s]View::dump, name:%s(%s), _hot_level:%.2f, dur:%d", uID.c_str(), wID.c_str(), _ID.c_str(), _name.c_str(), _hot_level, _rcmd_intensity->_dur);
		return 0;
	}
	virtual int GetHotLevel(int prefer = 0) const = 0;
	int SetHotLevel(int hotLevel) {
		_hot_level = hotLevel;
	}
};

class View: public VarPlace{
public:
	View() {
		_type = 0;
		_level = VIEW_LEVEL_NULL;
	}
	View(const View& view):VarPlace(view) {
		_type = view._type;
	}
	virtual std::string getUniqId() const{
		//公转私的点名字有变化
		if(not m_refer.empty() and not m_ptid.empty()) return _ID+"_"+m_ptid;
		else return _ID;
	}
public:
	int GetHotLevel(int viewPrefer = 0) const {
		return _hot_level;
	}
public:
	int _type;
};


class Shop: public VarPlace {
public:
	virtual std::string getUniqId() const{
		//公转私的点名字有变化
		if(not m_refer.empty() and not m_ptid.empty()) return _ID+"_"+m_ptid;
		else return _ID;
	}
public:
	int GetHotLevel(int shopPrefer = SHOP_INTENSITY_NULL) const {
		if (shopPrefer == SHOP_INTENSITY_SHOPAHOLIC) {
			return _hot_level + 10;
		}
		return _hot_level;
	}
};

class Restaurant: public VarPlace{
public:
	virtual std::string getUniqId() const{
		//公转私的点名字有变化
		if(not m_refer.empty() and not m_ptid.empty()) return _ID+"_"+m_ptid;
		else return _ID;
	}
	int GetHotLevel(int restPrefer = 0) const {
		return _hot_level;
	}
};

class Tour: public VarPlace {
public:
	int GetHotLevel(int prefer = 0) const{
		if( _hot_level ==0 ) return _hot_level;
		else return _hot_level;
	}
public:
	int m_preTime;
	int m_preBook;
	bool m_durEmpty;
	Json::Value m_srcTimes;				//时间相关原始信息
	std::tr1::unordered_set<std::string> m_refPoi;	//关联景点
	std::string m_pid;						//产品ID
	//集合和离开地点
	std::string m_srcJieSongPOI;
	std::tr1::unordered_set<std::string> m_gatherLocal;      //集合地点
	std::tr1::unordered_set<std::string> m_dismissLocal;        //离开地点
	int m_jiesongType;
	//接送poi
	std::string m_pid_3rd;					//第三方产品id
	Json::Value m_open;						//可用日期
	std::string m_sid;						//源id
};

class ViewTicket:public Tour {
public:
	ViewTicket():Tour() {}
	ViewTicket(const ViewTicket &p):Tour(p) {
		m_view = p.Getm_view();
	}
private:
	std::string m_view;
public:
	std::string Getm_view() const {
		return m_view;
	}
	int Setm_view(const std::string& id) {
		m_view = id;
		return 0;
	}
};

class TicketsFun{
public:
	std::string m_pid;		//门票产品ID
	std::string m_name;		//票务名称
	Json::Value m_info;		//票务信息
	std::string m_ccy;		//币种
	int m_id;				//门票ID
	std::string m_ticket_id; //综合公私票中,唯一的id
	int m_userNum;			//可用人数
	int book_pre;			//提前预定
	Json::Value m_times;	//场次
	Json::Value m_date_price; //价格(与日期相关)
	//公有库增加字段
	int m_ticketType;
	int m_max;
	int m_min;
	int m_agemax;
	int m_agemin;
	std::string m_ticket_3rd;
	std::string m_traveller_3rd;
	std::string m_sid;
};

// 每个block内 每个VarPlace对应一个PlaceInfo
// 两个PlaceInfo间的交通 用m_deptID->m_arvID来取
// arvID 与 deptID可变为hotelID
// 每个PlaceInfo对应多个openClose

class OpenClose {
public:
	OpenClose(time_t open = 0, time_t close = 0, time_t latestArv = 0, int meals = 0) {
		m_open = open;
		m_close = close;
		m_latestArv =latestArv;
		m_meals = meals;
	}
	OpenClose(const OpenClose& openClose) {
		m_open = openClose.m_open;
		m_close = openClose.m_close;
		m_latestArv = openClose.m_latestArv;
		m_meals = openClose.m_meals;
	}
	int Set(time_t open, time_t close, time_t latestArv, int meals = 0) {
		m_open = open;
		m_close = close;
		m_latestArv = latestArv;
		m_meals = meals;
	}
public:
	time_t m_open;
	time_t m_latestArv;
	time_t m_close;
	int m_meals;
};

class PlaceInfo {
public:
	PlaceInfo() {
		m_vPlace = NULL;
		m_dur = 0;
		m_type = 0;
		m_cost = 0.0;
	}
	virtual ~PlaceInfo() {
		for (int i = 0; i < m_openCloseList.size(); ++i) {
			if (m_openCloseList[i]) {
				delete m_openCloseList[i];
			}
		}
		m_openCloseList.clear();
		m_availOpenCloseList.clear();
	}
public:
	const VarPlace* m_vPlace;
	std::string m_arvID;
	std::string m_deptID;
	std::vector<const OpenClose*> m_openCloseList;
	std::vector<const OpenClose*> m_availOpenCloseList;
	int m_dur;	//停留时长
	double m_cost;
	int m_type;
	int m_timeZone;
public:
	int Dump() const {
		if (m_vPlace) {
			char buf[1024];
			int len = 0;
			int res = 0;
			if ( (res = snprintf(buf, sizeof(buf), "timeList: ")) != -1 ) {
				len += res;
			}
			for (int i = 0; i < m_openCloseList.size(); ++i) {
				if (m_openCloseList[i]) {
					std::string strOpen = MJ::MyTime::toString(m_openCloseList[i]->m_open, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "%s", strOpen.c_str())) != -1) {
						len += res;
					}
					std::string latestArvS = MJ::MyTime::toString(m_openCloseList[i]->m_latestArv, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "(%s)--->", latestArvS.c_str())) != -1) {
						len += res;
					}
					std::string strClose = MJ::MyTime::toString(m_openCloseList[i]->m_close, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "%s\t", strClose.c_str())) != -1) {
						len += res;
					}
				}
			}
			MJ::PrintInfo::PrintDbg("PlaceInfo::Dump, %s(%s), t: %d, timeInfo: %s", m_vPlace->_ID.c_str(), m_vPlace->_name.c_str(), m_type, buf);
		} else {
			MJ::PrintInfo::PrintErr("PlaceInfo::Dump, m_vPlace is null");
		}
		return 0;
	}
	int DumpAvail() const {
		if (m_vPlace) {
			char buf[1024];
			int len = 0;
			int res = 0;
			if ( (res = snprintf(buf, sizeof(buf), "timeList: ")) != -1 ) {
				len += res;
			}
			for (int i = 0; i < m_availOpenCloseList.size(); ++i) {
				if (m_availOpenCloseList[i]) {
					std::string strOpen = MJ::MyTime::toString(m_availOpenCloseList[i]->m_open, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "%s", strOpen.c_str())) != -1) {
						len += res;
					}
					std::string latestArvS = MJ::MyTime::toString(m_availOpenCloseList[i]->m_latestArv, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "(%s)--->", latestArvS.c_str())) != -1) {
						len += res;
					}
					std::string strClose = MJ::MyTime::toString(m_availOpenCloseList[i]->m_close, m_timeZone);
					if ( (res = snprintf(buf + len, sizeof(buf) - len, "%s\t", strClose.c_str())) != -1) {
						len += res;
					}
				}
			}
			MJ::PrintInfo::PrintDbg("PlaceInfo::DumpAvail, %s(%s), t: %d,timeInfo: %s", m_vPlace->_ID.c_str(), m_vPlace->_name.c_str(), m_type, buf);
		} else {
			MJ::PrintInfo::PrintErr("PlaceInfo::DumpAvail, m_vPlace is null");
		}
		return 0;
	}
};

class ShopInfo: public PlaceInfo {
};

class RestaurantInfo: public PlaceInfo {
};

class ViewInfo: public PlaceInfo {
};

class HotelInfo: public PlaceInfo {
public:
	HotelInfo() {
		_rnum = 1;
		_checkin = "";
		_checkout = "";
		_source = "";
		_type = "";
		_night = 0;
	}

	int Dump() const {
		if (m_vPlace) {
			MJ::PrintInfo::PrintDbg("HotelInfo::Dump, name:%s(%s), checkin:%s, checkout:%s, cost:%.2f, type:%s, room:%d", m_vPlace->_ID.c_str(), m_vPlace->_name.c_str(), _checkin.c_str(), _checkout.c_str(), m_cost, _type.c_str(), _rnum);
		} else {
			MJ::PrintInfo::PrintErr("HotelInfo::Dump, m_vPlace is null");
		}
		return 0;
	}
public:
	std::string _checkin;           //酒店入住时间
	std::string _checkout;          //酒店离开时间
	std::string _source;
	int _rnum;              //房间数
	std::string _type;
	std::string _service;
	double _grade;
	int _star;
	int _night;
};

class MoneyPrice{
	public:
		int m_curPrice;
		std::string m_curCcy;
		int m_oriPrice;
		std::string m_oriCcy;
		MoneyPrice() {
			m_curPrice = 0;
			m_curCcy = "";
			m_oriPrice = 0;
			m_oriCcy = "";

		}

};

class TrafficItem {
public:
	TrafficItem() {
		_mode = TRAF_MODE_WALKING;
		_time = 0;
		_dist = 0;
		m_realDist = 0;
		_mid = "";
		m_stat = TRAF_STAT_STATIC;
		m_order = -1;
		m_custom = 0;
		m_startP.clear();
		m_stopP.clear();
	}
	TrafficItem(const TrafficItem& trafItem) {
		Copy(&trafItem);
	}
	TrafficItem& operator= (const TrafficItem& trafItem) {
		if (this == &trafItem) {
			return *this;
		}
		Copy(&trafItem);
		return *this;
	}
	bool operator== (const TrafficItem trafItem) const {
		return (_mode == trafItem._mode && _time == trafItem._time && _dist == trafItem._dist && _mid == trafItem._mid);
	}
	int Copy(const TrafficItem* pTrafItem) {
		_mode = pTrafItem->_mode;
		_time = pTrafItem->_time;
		_dist = pTrafItem->_dist;
		m_realDist = pTrafItem->m_realDist;
		_mid = pTrafItem->_mid;
		m_uberPrice = pTrafItem->m_uberPrice;
		m_stat = pTrafItem->m_stat;
		m_order = pTrafItem->m_order;
		m_warnings = pTrafItem->m_warnings;
		m_transfers = pTrafItem->m_transfers;
		m_custom = pTrafItem->m_custom;
		m_startP.assign(pTrafItem->m_startP);
		m_stopP.assign(pTrafItem->m_stopP);
		return 0;
	}
	//用户定义的交通mode转内部定义的交通mode，前者比后者多1
	static int UserTrafMode2InnerDefine(int userMode) {
		if (userMode == TRAF_MODE_TAXI + 1) {
			return TRAF_MODE_TAXI;
		} else if (userMode == TRAF_MODE_WALKING + 1) {
			return TRAF_MODE_WALKING;
		} else if (userMode == TRAF_MODE_BUS + 1) {
			return TRAF_MODE_BUS;
		} else if (userMode == TRAF_MODE_UBER + 1) {
			return TRAF_MODE_UBER;
		} else if (userMode == TRAF_MODE_DRIVING + 1) {
			return TRAF_MODE_DRIVING;
		} else {
			return -1; //其他
		}
	}
	//内部定义的交通mode转用户定义的交通mode,前者比后者少1
	static int InnerTrafMode2UserDefine(int innerMode, bool isCharterCar = false, bool custom = false) {
		if (innerMode == TRAF_MODE_TAXI) {//非自定义时进行转换
			if (!custom) {
				if (!isCharterCar) {
					return TRAF_MODE_TAXI + 1;
				} else {
					return TRAF_MODE_DRIVING + 1;
				}
			} else {
				return TRAF_MODE_TAXI + 1;
			}
		} else if (innerMode == TRAF_MODE_WALKING) {
			return TRAF_MODE_WALKING + 1;
		} else if (innerMode == TRAF_MODE_BUS) {
			return TRAF_MODE_BUS + 1;
		} else if (innerMode == TRAF_MODE_UBER) {
			return TRAF_MODE_UBER + 1;
		} else if (innerMode == TRAF_MODE_DRIVING) {
			return TRAF_MODE_DRIVING + 1;
		} else {
			return 0;
		}
	}
	//用户定义的交通mode转描述字符串
	static std::string UserTrafMode2String(int userMode) {
		if (userMode == TRAF_MODE_TAXI + 1) {
			return "打车";
		} else if (userMode == TRAF_MODE_WALKING + 1) {
			return "步行";
		} else if (userMode == TRAF_MODE_BUS + 1) {
			return "公交";
		} else if (userMode == TRAF_MODE_UBER + 1) {
			return "UBer";
		} else if (userMode == TRAF_MODE_DRIVING + 1) {
			return "自驾";
		} else {
			return "其他";
		}
	}
public:
	bool m_custom;//自定义交通
	int _mode;  // 0驾车，1步行，2公共交通
	int _time;
	int _dist;
	int m_realDist;
	int m_stat;
	int m_order;
	std::string m_startP;
	std::string m_stopP;
	std::string _mid;
	Json::Value m_warnings;
	Json::Value m_transfers;
	MoneyPrice m_uberPrice;
	std::vector<const TrafficItem *> m_trafficItemList;
};

class TrafficDetail{
public:
	TrafficDetail() {
		m_stat = TRAF_STAT_STATIC;
		m_transit = -1;
		m_walkDist = -1;
		m_order = -1;
		m_trafficItem = NULL;
	}
	~TrafficDetail() {
		if (m_trafficItem) {
			delete m_trafficItem;
		}
	}
	int m_stat;
	int m_transit;
	int m_walkDist;
	int m_order;
	std::vector<std::pair<std::string, std::string> > m_srcList;//name ，url
	TrafficItem* m_trafficItem;
	Json::Value m_jDetailTraffic;
};

class PathScore{
public:
	PathScore(){
		_score = 0;
		_cost = 0;
		_t_time = 0;
	};
	~PathScore(){};
	int Clear() {
		_score = 0;;
		_cost = 0;
		_t_time = 0;
	}
public:
	int _score;		//最终的路径得分
	int _cost;		//总花费
	int _t_time;	//交通花费时间
};



class NextItem {
public:
	NextItem() {
		_cur_view = NULL;
		_next_view = NULL;
		_dist = 0;
		_score = 0;
	}
	NextItem(const LYPlace* cur_view, const LYPlace* next_view, int dist, double score) {
		_cur_view = cur_view;
		_next_view = next_view;
		_dist = dist;
		_score = score;
	}
public:
	const LYPlace* _cur_view;
	const LYPlace* _next_view;
	int _dist;
	double _score;
};

struct PlaceIDCmp {
	bool operator()(const LYPlace* pa, const LYPlace* pb) {
		return (pa->_ID < pb->_ID);
	}
};

class PathInfo {
public:
	int m_trafficTime;
	int m_matchRate;
	int m_dur;
	int m_totalTime;
	int m_trafficDist;
	int m_closeNum;
	int m_viewDur;
	int m_viewTotal;
	double m_cost;
	double m_trafficRate;
	int m_intensity;
	PathInfo() {
		m_totalTime = 0;
		m_trafficTime = 0;
		m_matchRate = 0;
		m_trafficDist = 0;
		m_cost  = 0;
		m_intensity = 0;
		m_trafficRate = 0;
	}
};


class ShowItem {
public:
    ShowItem() : m_place(NULL), m_score(0) {}
    ShowItem(const LYPlace* place) : m_place(place), m_score(0) {}
    ShowItem(const LYPlace* place, double s) : m_place(place), m_score(s) {}

    const LYPlace* GetPlace() const {
        return m_place;
    }

    double GetScore() const {
        return m_score;
    }

    int SetScore(double s) {
        if (s < 0) s = 0;
        m_score = s;
        return 0;
    }

private:
    const LYPlace* m_place;
    double m_score;
};

class DurS {
public:
	DurS(): m_min(0), m_zip(0), m_rcmd(0), m_extend(0), m_max(0) {}
	DurS(int minDur, int zipDur, int rcmdDur, int extendDur, int maxDur): m_min(minDur), m_zip(zipDur), m_rcmd(rcmdDur), m_extend(extendDur), m_max(maxDur) {}
	DurS(const DurS& durS) {
		Set(durS.m_min, durS.m_zip, durS.m_rcmd, durS.m_extend, durS.m_max);
	}
	int SetMax(int maxDur) {
		m_max = maxDur;
		return 0;
	}
	int Set(int minDur, int zipDur, int rcmdDur, int extendDur, int maxDur) {
		m_min = minDur;
		m_zip = zipDur;
		m_rcmd = rcmdDur;
		m_extend = extendDur;
		m_max = maxDur;
		return 0;
	}
	int Fix() {
		m_min = std::min(m_min, m_max);
		m_rcmd = std::max(m_min, m_rcmd);
		m_rcmd = std::min(m_rcmd, m_max);
		m_zip = std::max(m_min, m_zip);
		m_zip = std::min(m_zip, m_rcmd);
		m_extend = std::min(m_extend, m_max);
		m_extend = std::max(m_rcmd, m_extend);
		return 0;
	}
public:
	int m_min;
	int m_zip;
	int m_rcmd;
	int m_extend;
	int m_max;
};

struct varPlaceCmp {
	bool operator()(const LYPlace* A, const LYPlace* B) {
		const VarPlace* vPlaceA = dynamic_cast<const VarPlace*>(A);
		const VarPlace* vPlaceB = dynamic_cast<const VarPlace*>(B);
		if (!vPlaceA || !vPlaceB) return true;
		if (vPlaceA->_rec_priority == vPlaceB->_rec_priority) {
			if (vPlaceA->_hot_level > vPlaceB->_hot_level) {
				return true;
			} else if (vPlaceA->_hot_level < vPlaceB->_hot_level) {
				return false;
			} else {
				return (vPlaceA->_ID < vPlaceB->_ID);
			}
		} else if (vPlaceA->_rec_priority == 10) return true;
		else return false;
	}
};

class HInfo {
public:
	HInfo(const LYPlace* hotel = NULL, std::string checkIn = "", std::string checkOut = "", bool isCoreHotel = false) {
		m_hotel = hotel;
		m_checkIn = checkIn;
		m_checkOut = checkOut;
		m_name = "";
		m_lname = "";
		m_id = "";
		m_isCoreHotel = isCoreHotel;
		m_userCommand = false;
	}
public:
	const LYPlace* m_hotel;
	std::string m_checkIn;
	std::string m_checkOut;
	std::string m_name;
	std::string m_lname;
	std::string m_id;
	int m_dayStart;
	int m_dayEnd;
	bool m_isCoreHotel;
	bool m_userCommand;
};

struct hInfoCmp {
	bool operator()(const HInfo* A, const HInfo* B) {
		if (!A || !B) return true;
		return A->m_checkIn < B->m_checkIn;
	}
};

struct POIDetail {
	const LYPlace *m_place;
	int m_mode;
	std::string m_sDate;
	std::string m_eDate;
	const LYPlace *GetPlace() const {
		return m_place;
	}
};

//产品验证相关
struct ProductTicket {
	std::string ticketId;
	std::string productId;
	std::string date;
};

class Country {
public:
    Country() {
        _ID = "";
        _name = "";
    };

    Country(const Country& country) {
        _ID = country._ID;
        _name = country._name;
    }

    ~Country(){};

public:
    std::string _ID; //country id
    std::string _name; //country name
};

struct QueryParam {
	std::string token;
	std::string type;
	std::string ver;
	std::string lang;
	std::string qid;
	std::string uid;
	int dev;
	std::string wid;
	mutable std::string reqHead;//请求头
	std::string log;
	std::string ccy;//币种
	std::string ptid;//供应商id
	std::string csuid;
	std::string refer_id;
	std::string cur_id;
	int priDataThreadId;
	std::tr1::unordered_map<std::string,std::tr1::unordered_set<std::string>> available_sources;
	QueryParam():dev(0),priDataThreadId(0){}
	const std::string& ReqHead() const {
		if (!reqHead.empty()) return reqHead;
		char buff[5000] = {0};
		snprintf(buff, sizeof(buff),"token=%s&type=%s&dev=%d&ver=%s&lang=%s&qid=%s&uid=%s&ccy=%s&ptid=%s&query=",token.c_str(), type.c_str(), dev, ver.c_str(), lang.c_str(), qid.c_str(), uid.c_str(), ccy.c_str(), ptid.c_str());
		reqHead = std::string(buff);
		return reqHead;
	}
	int Log() {
		char buff[5000] = {0};
		snprintf(buff, sizeof(buff), "%s--%s", qid.c_str(), wid.c_str());
		log = std::string(buff);
		return 0;
	}

	int Log(std::string& cid) {
		char buff[5000] = {0};
		snprintf(buff, sizeof(buff), "%s--c%s", qid.c_str(), cid.c_str());
		log = std::string(buff);
		return 0;
	}
};

#endif		//__LY_DEFINE_H__
