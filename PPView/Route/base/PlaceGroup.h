#ifndef _PLACE_GROUP_H
#define _PLACE_GROUP_H

#include <iostream>
#include <vector>
#include <tr1/unordered_map>

#include "define.h"
#include "PathView.h"

// 当前点的group类型
const int GROUP_TYPE_NULL = 0x00000000;
const int GROUP_TYPE_LAST = 0x00000001;  // 与last成group
const int GROUP_TYPE_NEXT = 0x00000002;
const int GROUP_TYPE_ALL = GROUP_TYPE_LAST | GROUP_TYPE_NEXT;
const int GROUP_TYPE_IND = 0x00000004;  // independent 独立成组

// group变化状态
enum GROUP_STATUS_TYPE {
	GROUP_STATUS_DOWN,
	GROUP_STATUS_STAY,
	GROUP_STATUS_UP
};

class PlaceGroup {
public:
	PlaceGroup() {
		if (!_inited) {
			InitFreq();
		}
	}
public:
	int Push(const std::string& id_a, const std::string& id_b);
	bool IsGroup(const std::string& id_a, const std::string& id_b);
	bool HasGroup(const std::string& id);
	static bool IsFreq(const std::string& id_a, const std::string& id_b);
	static bool HasFreq(const std::string& id);
	int PenalizePath(const std::string& uid, const std::string& wid, PathView& path, bool dbg = false);
	int GetGroupStatus(const LYPlace* from_place, const LYPlace* to_place, const LYPlace* place, PathView* path_list, int path_num);
	static int InitFreq();
private:
	int GetItemGroupType(PathView& path, int index);
	bool IsBreakGroup(const LYPlace* from_place, const LYPlace* to_place, const LYPlace* place, PathView& path);
	static bool Load(const std::string& data_path, std::vector<std::string>& freq_group_file_list);

public:
	static std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> > _freq_map;
private:
	static std::tr1::unordered_set<std::string> _freq_group_set;
	static std::tr1::unordered_set<std::string> _freq_id_set;
	static bool _inited;
};

#endif
