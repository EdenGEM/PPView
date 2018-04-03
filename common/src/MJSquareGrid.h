#ifndef _MJSquareGrid_H_
#define _MJSquareGrid_H_

#include <set>
#include <tr1/unordered_set>
#include <tr1/unordered_map>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
using namespace std;
namespace MJ {
class MJSquareGrid {
public:
	//方便使用进一步封装,id与 extraLogo组合唯一
	bool addId2Grid(std::string id,std::string extraLogo,std::string mapInfo);
	bool delIdFromGrid(std::string id,std::string extraLogo);
    //给出中心点经纬度和距离,得到相应网格范围中的poi,dist单位是米,算球面距离过滤 
	bool getRangePlace(std::string mapInfo,int dist,std::set <std::pair<std::string,std::string> >& retData);

private:
	//基础能力提供,id唯一
	bool addId2Grid(std::string id,std::string mapInfo);
	bool delIdFromGrid(std::string id);
	bool getRangePlace(std::string mapInfo,int dist,std::tr1::unordered_set<std::string>& retData);
	//将经纬度换算成坐标
    static std::string LonLat2Coord(double lonDu, double latDu);

private:
//属性和内部实现函数
	static std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> >m_gridMap;//网格映射poi   Eden
	static std::tr1::unordered_map<std::string, std::string> m_Id2GridKeyMap;//id 映射对应的网格key    Eden
	static std::tr1::unordered_map<std::string, std::string> m_id2MapInfo;//id 映射对应的经纬度    Eden
	static std::tr1::unordered_map<int, int> m_gridBorder;//网格中坐标y对应的范围x   Eden
};
}
#endif
