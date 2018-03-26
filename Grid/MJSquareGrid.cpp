#include "MJSquareGrid.h"
#include <algorithm>
#include <cmath>
#include <string.h>
#include <numeric>
#include <limits>
#include <iostream>
#include <time.h>
using namespace std;
using namespace MJ;

static double kPI = atan(1) * 4;
static int earthRadius = 6378137;//地球赤道半径(m)
static int EquatorRadius = 6378.137;//地球赤道半径(km)
static int PloarRadius=6356.9088;//地球极半径(km)

std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> >  MJSquareGrid::m_gridMap;//网格映射poi   Eden
std::tr1::unordered_map<std::string, std::string> MJSquareGrid::m_Id2GridKeyMap;//id 映射对应的网格key    Eden
std::tr1::unordered_map<int, int> MJSquareGrid::m_gridBorder;//网格中坐标y对应的范围x   Eden
std::tr1::unordered_map<std::string, std::string> MJSquareGrid::m_id2MapInfo;//id 映射对应的经纬度    Eden

double CaluateSphereDist(std::string place_a,  std::string place_b) {
    std::string::size_type pos = place_a.find(",");
    if (pos == std::string::npos) return -1;
    double lon_a = atof(place_a.substr(0, pos).c_str()) * M_PI / 180;
    double lat_a = atof(place_a.substr(pos + 1).c_str()) * M_PI / 180;
    pos = place_b.find(",");
    if (pos == std::string::npos) return -1;
    double lon_b = atof(place_b.substr(0, pos).c_str()) * M_PI / 180;
    double lat_b = atof(place_b.substr(pos + 1).c_str()) * M_PI / 180;
    return std::max(0.0, earthRadius * acos(cos(lat_a) * cos(lat_b) * cos(lon_a - lon_b) + sin(lat_a) * sin(lat_b)));
}
//删点
bool MJSquareGrid::delIdFromGrid(std::string id,std::string extraLogo){
	id=id+"#"+extraLogo;
	delIdFromGrid(id);
	return true;
}
bool MJSquareGrid::delIdFromGrid(std::string id) { //删除GridMap中Id的映射 Eden
	if(m_Id2GridKeyMap.count(id)) {
		std::string key = m_Id2GridKeyMap[id];
		m_gridMap[key].erase(id);
		m_Id2GridKeyMap.erase(id);
	}
    return true;
}
//加点 将poi存放到对应的单元格中 Eden
bool MJSquareGrid::addId2Grid(std::string id,std::string extraLogo,std::string mapInfo) {
	id=id+"#"+extraLogo;
	bool ret=addId2Grid(id,mapInfo);
	if(ret)
		return true;
	else
		return false;
}
bool MJSquareGrid::addId2Grid(std::string id,std::string mapInfo){
    std::string::size_type pos = mapInfo.find(",");
    if (pos == std::string::npos) return false;
    double lonDu=atof(mapInfo.substr(0, pos).c_str());
    double latDu=atof(mapInfo.substr(pos + 1).c_str());
    std::string key=MJSquareGrid::LonLat2Coord(lonDu,latDu);
    m_gridMap[key].insert(id);
	m_id2MapInfo[id]=mapInfo;
	m_Id2GridKeyMap[id]=key;
    return true;
}
//将经纬度换算成坐标 Eden
std::string MJSquareGrid::LonLat2Coord(double lonDu, double latDu){
    double lat = abs(latDu * M_PI / 180);
    double EveryLatLength=2*M_PI*PloarRadius/360;//每一纬度长度（定值）
    double EveryLonLength=2*M_PI*EquatorRadius/360*cos(lat); //每一经度长度（当地纬度）
    int halfCircle=floor(EveryLonLength*180); //定纬度下的纬度圈周长/2(即坐标x范围)

    long long x=floor(EveryLonLength*lonDu);
    long long y=floor(EveryLatLength*latDu);
    std::string key=std::to_string(x)+"_"+std::to_string(y);
    if (m_gridBorder.find(abs(y))==m_gridBorder.end()){
        m_gridBorder[abs(y)]=halfCircle;
    }
    return key;
}
//给出中心点经纬度和距离,得到相应网格范围中的poi  Eden
bool MJSquareGrid::getRangePlace(std::string mapInfo,int Dist,std::set<std::pair<std::string, std::string> > &retData){
	std::tr1::unordered_set<std::string> idList;
	getRangePlace(mapInfo,Dist,idList);
	for(auto it = idList.begin(); it != idList.end(); it++) {
		std::string::size_type pos = (*it).find("#");
		if (pos == std::string::npos)
			continue;
		std::string id=(*it).substr(0,pos);
		std::string extraLogo = (*it).substr(pos+1);
		retData.insert(std::make_pair(id,extraLogo));
	}
	return true;
}
bool MJSquareGrid::getRangePlace(std::string mapInfo,int Dist,std::tr1::unordered_set<std::string> &idList){
    std::string::size_type pos = mapInfo.find(",");
    if (pos == std::string::npos) {
        std::cerr<<"mapInfo error"<<std::endl;
        return false;
    }
    double lonDu=atof(mapInfo.substr(0, pos).c_str());
    double latDu=atof(mapInfo.substr(pos + 1).c_str());
	int maxDist;
	int expand=1;
	if(abs(latDu)>=0 && abs(latDu)<=30) expand=2;
	else if(abs(latDu)>30 && abs(latDu)<=60) expand=3;
	else if(abs(latDu)>60 && abs(latDu)<=90) expand=5;
	maxDist=Dist/1000*expand;
    std::string key= LonLat2Coord(lonDu,latDu);
    pos = key.find("_");
    if(pos == std::string::npos) {
        return false;
    }
    int x=atof(key.substr(0, pos).c_str());
    int y=atof(key.substr(pos+1).c_str());
    int leftX=x-maxDist;
    int rightX=x+maxDist;
    int upY=y+maxDist;
    int downY=y-maxDist;
    int rightXPlus = std::numeric_limits<int>::max();//跨越公里网边界后的网格右边界
    int leftXPlus = std::numeric_limits<int>::min();//跨越公里网边界后的网格左边界
    int minXRange = std::numeric_limits<int>::max();
    int maxXRange = std::numeric_limits<int>::min();
    for(int i=downY;i<=upY;i++) {
        if(m_gridBorder.find(abs(i))!=m_gridBorder.end()) {
            minXRange=min(m_gridBorder[abs(i)],minXRange);
            maxXRange=max(m_gridBorder[abs(i)],maxXRange);
        }
    }
    if(x<0) {  //处理公里网左边界
        if(-minXRange>leftX) {
            leftX=-maxXRange;
            if(-minXRange<x) {
                leftXPlus=minXRange-(maxDist-(x-(-minXRange)));
            }else {
                leftXPlus=minXRange-maxDist;
            }
			rightXPlus=maxXRange;
        }
    }else if(x>=0) {  //处理公里网右边界
        if(minXRange<rightX) {
            rightX=maxXRange;
            if(minXRange>x) {
                rightXPlus=-(minXRange-(maxDist-(minXRange-x)));
            }else{
                rightXPlus=-(minXRange-maxDist);
            }
			leftXPlus=-maxXRange;
        }
    }
    std::string newKey;
    for(long long i=leftX;i<rightX;i++){
        for(long long j=downY;j<upY;j++) {
            newKey=to_string(i)+"_"+to_string(j);
            if(m_gridMap.find(newKey)== m_gridMap.end())continue;
			auto gridPList=m_gridMap[newKey];
            for(auto it = gridPList.begin(); it != gridPList.end(); ++it ){
                idList.insert(*it);
            }
        }
    }
    if(leftXPlus!=std::numeric_limits<int>::min()){
        for(long long i=leftXPlus;i<rightXPlus;i++){
            for(long long j=downY;j<upY;j++) {
                newKey=to_string(i)+"_"+to_string(j);
                if(m_gridMap.find(newKey)== m_gridMap.end())continue;
                auto gridPList=m_gridMap[newKey];
                for(auto it = gridPList.begin(); it != gridPList.end(); ++it ){
                    idList.insert(*it);
                }
            }
        }
    }
	std::tr1::unordered_set<std::string> filterIdList;
	for(auto it = idList.begin(); it != idList.end(); it++){
		double dist=CaluateSphereDist(mapInfo,m_id2MapInfo[*it]);
		if(dist <= Dist){
			filterIdList.insert(*it);
		}
	}
	swap(filterIdList,idList);
	//std::cerr<<"getRangePlace: after filter idList size: "<<idList.size()<<std::endl;
	//for(auto it = idList.begin();it != idList.end();it++){
	//	std::cerr<<*it<<std::endl;
	//}
    return true;
}
int main()
{
	for(int i=0;i<100;i++){
		MJSquareGrid mj=MJSquareGrid();
		srand((unsigned)time(NULL));
		long long lon=rand()%180;
		long long lat=rand()%90;
		int Dist=30000+rand()%20001;
		std::cout<<"Dist: "<<Dist<<std::endl;
		std::string lonLat=std::to_string(lon)+","+std::to_string(lat);
		std::string extraLogo="";
		mj.addId2Grid(lonLat,extraLogo,lonLat);
		std::map<std::string,std::string> idLonLatMap;
		idLonLatMap[lonLat]=lonLat;
		std::tr1::unordered_set<std::string> sphereList;
		std::string key,map_info;
		long double lon_id,lat_id;
		for (long long i=0;i<10000;i++)
		{
			double f=(double)(rand()%100001)/100001;
			double h=(double)(rand()%100001)/100001;
//			idx=(double)(x)+(double)(rand()%11);
//			idy=(double)(y)+(double)(rand()%11);
			lon_id=lon+f;
			lat_id=lat+h;
//			gcvt(idy,9,str2);
//			gcvt(idx,9,str1);
			map_info=std::to_string(lon_id)+","+std::to_string(lat_id);
			std::string id=map_info;
//			std::cout<<"map_info: "<<map_info<<std::endl;
			idLonLatMap[id]=map_info;
			mj.addId2Grid(id,extraLogo,map_info);
		}
		std::cout<<"idLonLat size: "<<idLonLatMap.size()<<std::endl;
		std::set<std::pair<std::string,std::string> >idList;
		mj.getRangePlace(lonLat,Dist,idList);
		std::tr1::unordered_set<std::string> tmpIdList;
		std::cout<<"after filter"<<std::endl;
		for (auto it = idLonLatMap.begin();it != idLonLatMap.end();it++)
		{
			double dist=CaluateSphereDist(lonLat,it->second);
			std::cout<<"id: "<<it->second<<" dist: "<<dist<<std::endl;
			if (dist<=Dist)
			{
				sphereList.insert(it->second);
				std::cout<<it->second<<std::endl;
			}
		}
		std::cout<<"lon: "<<lon<<" lat: "<<lat<<std::endl;
		std::cout<<"Dist: "<<Dist<<std::endl;
		std::cout<<"getRangePlace id size: "<<idList.size()<<std::endl;
		for(auto it = idList.begin();it != idList.end();it++){
			tmpIdList.insert(it->first);
		}
		std::cout<<"sphereList size: "<<sphereList.size()<<std::endl;
		if(idList.size() == sphereList.size()){
			std::cout<<"result: Same"<<std::endl;
		}
		else{
			std::cout<<"result: Diff"<<std::endl;
			for(auto it = sphereList.begin();it != sphereList.end();it++){
				if(tmpIdList.find(*it) == tmpIdList.end()){
					std::cout<<*it<<std::endl;
				}
			}
		}
		std::cout<<endl;
	}
	return 0;
}
