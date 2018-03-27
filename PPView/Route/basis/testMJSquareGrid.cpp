#include "MJSquareGrid.h"
#include <string>
#include <time.h>
#include <iostream>
#include <cmath>
#include <algorithm>

static int earthRadius = 6378137;//地球赤道半径(m)

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
int main()
{
	MJSquareGrid mj=MJSquareGrid();
	long long x=20,y=30;
	int Dist=30000;
	std::string lonLat=std::to_string(x)+","+std::to_string(y);
	std::string extraLogo="";
	mj.addId2Grid(lonLat,extraLogo,lonLat);
	std::tr1::unordered_map<std::string,std::string> idLonLatMap;
	idLonLatMap[lonLat]=lonLat;
	std::tr1::unordered_set<std::string> sphereList;
	std::string key,map_info;
	double idx,idy;
	srand((unsigned)time(NULL));
	int u=0;
	for (long long i=0;i<10000;i++)
	{
		char str1[6],str2[6];
		double f=(double)(rand()%100001)/100001;
		double h=(double)(rand()%100001)/100001;
		idx=(double)(x-5)+(double)(rand()%11);
		idy=(double)(y-5)+(double)(rand()%11);
		idx=idx+f;
		idy=idy+h;
		gcvt(idy,7,str2);
		gcvt(idx,7,str1);
		map_info=std::string(str1)+","+std::string(str2);
		std::string id=map_info;
		std::cout<<"map_info: "<<map_info<<std::endl;
		idLonLatMap[id]=map_info;
		mj.addId2Grid(id,extraLogo,map_info);
	}
	std::cout<<"idLonLat size: "<<idLonLatMap.size()<<std::endl;
	std::set<std::pair<std::string,std::string> >idList;
//	std::tr1::unordered_set<std::string >idList;
	mj.getRangePlace(lonLat,Dist/1000+1,idList);
	/*
	std::tr1::unordered_set<std::string> tmpIdList;
	for (auto it=idList.begin();it !=idList.end();it++)
	{
		double dist=CaluateSphereDist(lonLat,*it);
		if (dist<=Dist)
		{
			std::cout<<"in filter key: "<<*it<<std::endl;
			tmpIdList.insert(*it);
		}
	}
	std::cout<<"after filter idList size: "<<tmpIdList.size()<<std::endl;
	*/
	for (auto it = idLonLatMap.begin();it != idLonLatMap.end();it++)
	{
		double dist=CaluateSphereDist(lonLat,it->second);
		if (dist<=Dist)
		{
			std::cout<<" do filter key: "<<it->second<<std::endl;
			sphereList.insert(it->second);
		}
	}
	std::cout<<"sphereList size: "<<sphereList.size()<<std::endl;
	return 0;
}
