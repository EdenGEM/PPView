#include <iostream>
#include "LYConstData.h"
#include "PathStat.h"
#include "PathCross.h"

// 1 剩余时长
// 每天： 预设可游玩时长 - 交通时长 - 除头尾点停留时长
//      预设可游玩时长：尾点到达(不晚于晚21点) - 头点离开(不早于早9点)
//      交通时长：头尾间用到所有交通
//      除头尾点停留时长
// 整体：各天累加
//
// 2 交通占比
// 游玩时间区间：尾点到达-头点离开
// 每天头尾点间交通时长之和 / 每天游玩时间区间之和
//
// 3 游玩时长占比
// 每天景点、购物、餐厅、附近就餐停留之和 / 每天游玩时间区间之和
//
// 4 游玩强度
// 所有景点、购物、餐厅、附近就餐停留之和 / 推荐时长之和

// 总距离
int PathStat::GetTotDist(Json::Value& jDayList) {
	int totDist = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			if (jView.isMember("traffic") && !jView["traffic"].isNull()) {
				totDist += jView["traffic"]["dist"].asInt();
			}
		}
	}
	return totDist;
}

// 关门点数
int PathStat::GetCloseNum(Json::Value& jDayList) {
	int totCloseCnt = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			int vType = jView["type"].asInt();
			int isOpen = jView["isOpen"].asInt();
			if (vType & LY_PLACE_TYPE_VAR_PLACE && isOpen == 0) {
				++totCloseCnt;
			}
		}
	}
	return totCloseCnt;
}

// 总价格
double PathStat::GetTotPrice(Json::Value& jDayList) {
	double totPrice = 0.0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			int vType = jView["type"].asInt();
			if (vType & LY_PLACE_TYPE_VAR_PLACE) {
				totPrice += jView["price"].asDouble();
			}
		}
	}
	return totPrice;
}

// 天数量
int PathStat::GetDayNum(Json::Value& jDayList) {
	return jDayList.size();
}

// 景点数量
int PathStat::GetViewNum(Json::Value& jDayList) {
	int viewCnt = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			int vType = jView["type"].asInt();
			if ((vType & LY_PLACE_TYPE_VIEW ) || (vType & LY_PLACE_TYPE_SHOP)) {
				++viewCnt;
			}
		}
	}
	return viewCnt;
}

// 餐厅数量
int PathStat::GetRestNum(Json::Value& jDayList) {
	int viewCnt = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			int vType = jView["type"].asInt();
			if (vType & LY_PLACE_TYPE_RESTAURANT) {
				++viewCnt;
			}
		}
	}
	return viewCnt;
}

// 活动数量
int PathStat::GetActNum(Json::Value& jDayList) {
	return 0;
	//todo :
	int viewCnt = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			int vType = jView["type"].asInt();
			if ((vType & LY_PLACE_TYPE_VIEW ) || (vType & LY_PLACE_TYPE_SHOP)) {
				++viewCnt;
			}
		}
	}
	return viewCnt;
}

// 剩余可用游玩时长
int PathStat::GetRest(BasePlan* basePlan, Json::Value& jDayList) {
	int totRest = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		totRest += jDay["rest"].asInt();
	}
	return totRest;
}

// 每天剩余游玩时长 = 结合偏好计算头尾 - 中间所耗时长
int PathStat::GetRestDay(BasePlan* basePlan, const std::string& date, Json::Value& jViewList) {
	if (jViewList.size() < 2) return 0;
	bool firstDay = true;
	bool lastDay = true;
	int hotelNum = 0;
	for (int i = 0; i < jViewList.size(); ++i) {
		Json::Value& jView = jViewList[i];
		int pType = jView["type"].asInt();
		if (i == 0 && pType & LY_PLACE_TYPE_HOTEL) {
			firstDay = false;
		}
		if (i + 1 >= jViewList.size() && pType & LY_PLACE_TYPE_HOTEL) {
			lastDay = false;
		}
		if (pType & LY_PLACE_TYPE_HOTEL) {
			++hotelNum;
		}
	}
	time_t preferStart = basePlan->GetDayRange(date).first;
	time_t preferStop = basePlan->GetDayRange(date).second;
	time_t viewStart = MJ::MyTime::toTime(jViewList[0u]["etime"].asString(), basePlan->m_TimeZone);
	time_t viewStop = MJ::MyTime::toTime(jViewList[jViewList.size() - 1]["stime"].asString(), basePlan->m_TimeZone);
	if (firstDay) {
		if (hotelNum > 1) {
			viewStop = std::min(viewStop, preferStop);
		}
	} else if (lastDay) {
		if (hotelNum > 1) {
			viewStart = std::max(viewStart, preferStart);
		}
	} else {
		viewStart = std::max(viewStart, preferStart);
		viewStop = std::min(viewStop, preferStop);
	}
	if (viewStop < viewStart) {
		viewStop = viewStart;
	}
	int totDur = viewStop - viewStart;
	if (totDur <= 0) return 0;
	int usedDur = GetTrafDay(jViewList) + GetMiddleDurDay(jViewList);
	int rest = totDur - usedDur;
	return rest;
}

// 强度 = sum(真实游玩时长) / sum(推荐游玩时长)
double PathStat::GetIntensity(BasePlan* basePlan, Json::Value& jDayList) {
	int totDur = 0;
	int totRcmdDur = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		totDur += GetPlaceDurDay(jViewList);
		totRcmdDur += GetRcmdDurDay(basePlan, jViewList);
	}
	if (totRcmdDur <= 0) return 1.0;
	return (totDur * 1.0 / totRcmdDur);
}

int PathStat::GetIntensityLabel(BasePlan* basePlan, Json::Value& jDayList) {
	double intensity = GetIntensity(basePlan, jDayList);
	if (intensity < 0.8) {
		return 3;
	} else if (intensity < 0.9) {
		return 2;
	} else {
		return 1;
	}
}

int PathStat::GetCrossCnt(BasePlan* basePlan, Json::Value& jDayList) {
	int totCnt = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		std::vector<const LYPlace*> placeList;
		Json::Value& jDay = jDayList[i];
		Json::Value& jViewList = jDay["view"];
		for (int j = 0; j < jViewList.size(); ++j) {
			Json::Value& jView = jViewList[j];
			std::string id = jView["id"].asString();
			if (!LYConstData::IsRealID(id)) continue;
			const LYPlace* place = basePlan->GetLYPlace(id);
			if (place) {
				placeList.push_back(place);
			}
		}
		totCnt += PathCross::GetCrossCnt(placeList);
	}
	return totCnt;
}


int PathStat::GetTrafDay(Json::Value& jViewList) {
	int totTrafTime = 0;
	for (int i = 0; i < jViewList.size(); ++i) {
		Json::Value& jView = jViewList[i];
		if (jView.isMember("traffic") && jView["traffic"].isMember("dur")) {
			int trafTime = jView["traffic"]["dur"].asInt();
			totTrafTime += trafTime;
		}
	}
	return totTrafTime;
}

// 一天中去头尾Place(包含餐厅)游玩时长之和
int PathStat::GetMiddleDurDay(Json::Value& jViewList) {
	int totDur = 0;
	for (int i = 0; i < jViewList.size(); ++i) {
		if (i == 0 || i + 1 >= jViewList.size()) continue;
		Json::Value& jView = jViewList[i];
		std::string id = jView["id"].asString();
		int dur = jView["dur"].asInt();
		int type = jView["type"].asInt();
		totDur += dur;
	}
	return totDur;
}

// 一天中VarPlace(包含餐厅)游玩时长之和
int PathStat::GetPlaceDurDay(Json::Value& jViewList) {
	int totDur = 0;
	for (int i = 0; i < jViewList.size(); ++i) {
		Json::Value& jView = jViewList[i];
		std::string id = jView["id"].asString();
		int dur = jView["dur"].asInt();
		int type = jView["type"].asInt();
		if ((type & LY_PLACE_TYPE_VAR_PLACE) || id == LYConstData::m_attachRest->_ID) {
			totDur += dur;
		}
	}
	return totDur;
}

int PathStat::GetRcmdDurDay(BasePlan* basePlan, Json::Value& jViewList) {
	int totRcmdDur = 0;
	for (int i = 0; i < jViewList.size(); ++i) {
		Json::Value& jView = jViewList[i];
		std::string id = jView["id"].asString();
		int extra = jView["extra"].asInt();
		if (id == LYConstData::m_attachRest->_ID) {
			if (extra == 2) {
				totRcmdDur += BasePlan::m_lunchTime._time_cost;
			} else {
				totRcmdDur += BasePlan::m_supperTime._time_cost;
			}
		} else {
			const LYPlace* place = basePlan->GetLYPlace(id);
			const VarPlace* vPlace = dynamic_cast<const VarPlace*>(place);
			if (vPlace) {
				totRcmdDur += basePlan->GetRcmdDur(vPlace);
			}
		}
	}
	return totRcmdDur;
}

double PathStat::GetTrafPer(Json::Value& jDayList) {
	int totRange = 0;
	int totTrafTime = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		std::string stimeS = jDay["stime"].asString();
		std::string etimeS = jDay["etime"].asString();
		totRange += MJ::MyTime::compareTimeStr(stimeS, etimeS);

		Json::Value& jViewList = jDay["view"];
		totTrafTime += GetTrafDay(jViewList);
	}
	return (totTrafTime * 1.0 / totRange);
}

double PathStat::GetPlayPer(Json::Value& jDayList) {
	int totRange = 0;
	int totDur = 0;
	for (int i = 0; i < jDayList.size(); ++i) {
		Json::Value& jDay = jDayList[i];
		std::string stimeS = jDay["stime"].asString();
		std::string etimeS = jDay["etime"].asString();
		totRange += MJ::MyTime::compareTimeStr(stimeS, etimeS);

		Json::Value& jViewList = jDay["view"];
		totDur += GetPlaceDurDay(jViewList);
	}
	return (totDur * 1.0 / totRange);
}
