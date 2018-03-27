#include"Eden.h"
#include<string>
#include<time.h>
#include<iostream>
#include<algorithm>

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
	mj=MJSquareGrid();
	int x=20,y=30;
	int Dist=1000;
	std::string lonLat=std::to_string(x)+","+std::to_string(y);
	mj.GridMap(lonLat,LonLat);
	std::tr1::unordered_map<std::string,std::string> idLonLatMapi;
	std::tr1::unordered_set<std::string> sphereList;
	std::string key,map_info;
	double idx,idy;
	srand((unsigned)time(NULL));
	for (int i=0;i<10000;i++)
	{
		idx=(x-5)+(double)(rand())%11;
		idy=(y-5)+(double)(rand())%11;
		std::cout<<"idx: "<<idx<<" idy: "<<idy<<endl;
//		key=std::to_string(idx)+"_"+std::to_string(idy);
		map_info=std::to_string(idx)+","+std::to_string(idy);
		std::cout<<"map_info: "<<map_info<<std::endl;
		idLonLatMap[key]=map_info;
		mj.GridMap(map_info,map_info);
	}
	std::cout<<"idLonLat size: "<<idLonLatMap.size()<<std::endl;
	std::tr1::unordered_set<std::string> idList;
	mj.GetRangePlaceId(lonLat,Dist,std::tr1::unordered_set<std::string> &idList);
	std::cout<<"GetRange id size: "<<idList.size()<<std::endl;
	for (auto it=idList.begin();it !=idList.end();it++)
	{
		double dist=CaluateSphereDist(lonLat,*it);
		if (dist>Dist)
		{
			idList.earse(*it);
		}
	}
	std::cout<<"after filter idList size: "<<idList.size()<<std::endl;
	for (auto it = idLonLatMap.begin();it != idLonLatMap.end();it++)
	{
		double dist=CaluateSphereDist(lonLat,idLonLatMap[*it]);
		if (dist<=Dist)
		{
			sphereList.insert(*it);
		}
	}
	std::cout<<"sphereList size: "<<sphereList.size()<<std::endl;
	return 0;
}
