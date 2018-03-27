#include "LYConstData.h"
#include "RouteConfig.h"
#include "BasePlan.h"
#include "PlaceGroup.h"
	
std::tr1::unordered_map<std::string, std::tr1::unordered_set<std::string> > PlaceGroup::_freq_map;
std::tr1::unordered_set<std::string> PlaceGroup::_freq_group_set;
std::tr1::unordered_set<std::string> PlaceGroup::_freq_id_set;
bool PlaceGroup::_inited = false;

bool PlaceGroup::IsFreq(const std::string& id_a, const std::string& id_b) {
	std::string freq_key = id_a + "|" + id_b;
	if (_freq_group_set.find(freq_key) != _freq_group_set.end()) return true;
	return false;
}

int PlaceGroup::InitFreq() {
	if (_inited) return 0;
	bool ret = Load(RouteConfig::data_dir, RouteConfig::freq_group_file);
	if (ret) {
		_inited = true;
		return 0;
	}
	return 1;
}

// 加载freq_group
bool PlaceGroup::Load(const std::string& data_path, std::vector<std::string>& freq_group_file_list) {
	if (freq_group_file_list.size() <= 0) {
		MJ::PrintInfo::PrintErr("PlaceGroup::Load, file list size: 0");
		return false;
	}
	
	MJ::PrintInfo::PrintLog("PlaceGroup::Load, loading Freq Group...");
	for (int i = 0; i < freq_group_file_list.size(); ++i) {
		std::string file_name = data_path + "/" + freq_group_file_list[i];
		std::ifstream fin(file_name.c_str());
		if (!fin) {
			MJ::PrintInfo::PrintErr("PlaceGroup::Load, load file error: %s", file_name.c_str());
			return false;
		}
		std::string line;
		while (std::getline(fin, line)) {
			if (line.size() > 1 && line.substr(0, 1) == "#" ) continue;
			std::vector<std::string> tmp_list;
			ToolFunc::sepString(line, "\t", tmp_list);
			if (tmp_list.size() < 2) continue;

			for (int j = 0; j < tmp_list.size(); ++j) {
				std::string id_a = tmp_list[j];
				_freq_id_set.insert(id_a);
				for (int k = j + 1; k < tmp_list.size(); ++k) {
					std::string id_b = tmp_list[k];
					_freq_group_set.insert(id_a + "|" + id_b);
					_freq_group_set.insert(id_b + "|" + id_a);
				}
			}
		}
		fin.close();
	}
	MJ::PrintInfo::PrintLog("PlaceGroup::Load, load freq finish");
	return true;
}
