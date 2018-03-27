#ifndef _PATH_UTIL_H_
#define _PATH_UTIL_H_

#include <iostream>
#include "BasePlan.h"
#include "Utils.h"
class PlanItem;

class PathUtil {
public:
	static int MergeLuggAndSleep(BasePlan* basePlan);
	static int BackHotelTimeChange(PathView & planList);
	static int DoZipDur(BasePlan* basePlan, PathView& path);//压缩行程时长为zipDur
	//smz
	static int Norm15Min(BasePlan* basePlan);//15分钟凑整逻辑
	static int TimeEnrich(BasePlan* basePlan, PathView& path);//拉伸逻辑
	static int CalIdel(BasePlan* basePlan, PathView& path);//计算空白时长
	static int CalTraf(BasePlan* basePlan, PathView& path);//计算交通
	static int SetDur2PlanStats(BasePlan* basePlan, PathView& path);//进行poi dur的收集
	static int GetScale(BasePlan* basePlan, std::tr1::unordered_map<std::string, int>& allocDurMap);
	static int PerfectDelSet(BasePlan* basePlan, PathView& path);//用于贪心等各类点统计删点 貌似现在已弃用
	static int CutPathBySegment(BasePlan* basePlan, PathView& path, std::vector<PathView*>& pathList);//多城市 按城市中的段切分

private:
	//smz
	static int TimeStretch(BasePlan* basePlan, int start, int end,  PathView& path);//对指定开头结尾的 功能进行拉伸
	static time_t GetLatestArv(BasePlan* basePlan, int end, int index, PathView& path);//寻找下一个点最迟的到达时间
	static time_t GetEarliestDept(BasePlan* basePlan, int start, int index, PathView& path);//寻找前一个点最早的离开时间
};

class StretchItem {//拉伸用的类 参与排序的有分数 item的下标 时长
public:
	StretchItem(int& index, PlanItem*& pItem, int rcmdDur) {
		m_index = index;
		m_pItem = pItem;
		m_rcmdDur = rcmdDur;
	}

	StretchItem(StretchItem& sItem) {
		m_index = sItem.m_index;
		m_pItem = sItem.m_pItem;
		m_rcmdDur = sItem.m_rcmdDur;
	}
	StretchItem() {
		m_index = 0;
		m_pItem = NULL;
		m_rcmdDur = 0;
	}

	StretchItem(StretchItem* sItem) {
		m_index = sItem->m_index;
		m_pItem = sItem->m_pItem;
		m_rcmdDur = sItem->m_rcmdDur;
	}
public:
	static double GetStretchRate(const StretchItem* sItem);
	static int GetHotLevel(const StretchItem* sItem);
public:
	int m_index;
	PlanItem* m_pItem;
	int m_rcmdDur;
};

struct StretchItemCmp {
	bool operator() (const StretchItem* sItemA, const StretchItem* sItemB) {
		return StretchItem::GetHotLevel(sItemA) > StretchItem::GetHotLevel(sItemB);
	}
};

struct StretchItemRateCmp {
	bool operator() (const StretchItem* sItemA, const StretchItem* sItemB) {
		return StretchItem::GetStretchRate(sItemA) < StretchItem::GetStretchRate(sItemB);
	}
};
#endif

