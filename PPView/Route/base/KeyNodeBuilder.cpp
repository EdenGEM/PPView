#include <iostream>
#include "PathTraffic.h"
#include "KeyNodeBuilder.h"


bool KeynodeCmp(KeyNode* KeyNodeA, KeyNode* KeyNodeB) {
	if (KeyNodeA->_type == NODE_FUNC_KEY_ARRIVE) {
		return true;
	} else if (KeyNodeB->_type == NODE_FUNC_KEY_ARRIVE) {
		return false;
	} else if (KeyNodeA->_type == NODE_FUNC_KEY_DEPART) {
		return false;
	} else if (KeyNodeB->_type == NODE_FUNC_KEY_DEPART) {
		return true;
	} else {
		if (KeyNodeA->_close != KeyNodeB->_close) {
			return KeyNodeA->_close < KeyNodeB->_close;
		} else {
			return KeyNodeA->_open < KeyNodeB->_open;
		}
	}
}

// 确定与调整keynode
int KeyNodeBuilder::BuildKeyNode(Json::Value& req, BasePlan* basePlan) {
	int ret = BuildKeyNodeAll(req, basePlan);
	if (ret != 0) return 1;

	if (basePlan->GetKeyNum() < 2) {
		MJ::PrintInfo::PrintErr("[%s]BuildKeyNode::Keynode size is %d!!!", basePlan->m_qParam.log.c_str(), basePlan->GetKeyNum());
		basePlan->m_error.Set(54102, "keynode < 2");
		return 1;
	}

	return ret;
}

// 确定与调整keynode
int KeyNodeBuilder::BuildKeyNodeAll(Json::Value& req, BasePlan* basePlan) {
	MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::BuildKeyNodeAll...", basePlan->m_qParam.log.c_str());
	int ret = 0;

	// 1 加入keynode
	ret = AddKeyNode(req, basePlan);
	if (ret != 0) return 1;

	// 2 调整有交叉的keynode
	ret = MergeKeyNode(req, basePlan);
	if (ret != 0) {
		ret = MergeKeyNodeFaultToLerant(basePlan);
		if (ret) {
			basePlan->m_error.Set(54101,std::string("市内停留时间太短，车站/酒店 冲突"));
			return 1;
		}
	}

	ret = NotPlanBetweenSomeKeynodeByDate(req, basePlan);
	if (ret) {
		return 1;
	}

	//3h one day last Day limit
	if (basePlan->m_dayOneLast3hNotPlan) { //单城市前后两天 3小时 逻辑
		ret = ChangeContinueForDayOneLast3HNotPlan(req, basePlan);
		if (ret) {
			basePlan->m_error.Set(54101,std::string("市内停留时间太短，车站/酒店 冲突"));
			return 1;
		}
	}

	bool changeCotinue = false;
	for (int i = 0; i < basePlan->m_KeyNode.size(); i ++) {
		if (basePlan->m_faultTolerant && basePlan->m_KeyNode[i]->_type == NODE_FUNC_KEY_HOTEL_SLEEP && !changeCotinue) {
			changeCotinue = true;
			basePlan->m_KeyNode[i]->_continuous = KEY_NODE_CONTI_NO;
		}
		basePlan->m_KeyNode[i]->FixDurS();
		basePlan->GetKey(i)->Dump(true);
	}

	return 0;
}

// 添加keynode
int KeyNodeBuilder::AddKeyNode(Json::Value& req, BasePlan* basePlan) {
	for (int i = 0; i < basePlan->m_RouteBlockList.size(); ++i) {
		RouteBlock* route_block = basePlan->m_RouteBlockList[i];
		MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::AddKeyNode, m_RouteBlockList[%d]: %s->%s", basePlan->m_qParam.log.c_str(), i, MJ::MyTime::toString(route_block->_arrive_time, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(route_block->_depart_time, basePlan->m_TimeZone).c_str());

		KeyNode* segmentKey = NULL;
		KeyNode* arvKey = NULL;
		KeyNode* deptKey = NULL;
		KeyNode* leaveLuggageKey = NULL;
		KeyNode* reclaimLuggageKey = NULL;
		std::vector<KeyNode*> sleepKeyList;

		// 1 segment_place
		int arrive_continous = KEY_NODE_CONTI_NO;
		if (i > 0) {
			RouteBlock* last_route_block = basePlan->m_RouteBlockList[i - 1];
			int stayDur = route_block->_arrive_time - last_route_block->_depart_time;
			segmentKey = new KeyNode(LYConstData::m_segmentPlace, last_route_block->_depart_time, route_block->_arrive_time, MJ::MyTime::toString(route_block->_arrive_time, basePlan->m_TimeZone).substr(0, 8), stayDur, stayDur, stayDur, stayDur, stayDur, KEY_NODE_CONTI_OK, 0, NODE_FUNC_KEY_SEGMENT, ADJUST_NO, false, basePlan->m_TimeZone);
			arrive_continous = KEY_NODE_CONTI_OK;
			segmentKey->_notPlan =1;
		}

		// 2 arrive_place + depart_place
		time_t come_arrive_time = route_block->_arrive_time;//到达点的开始时间
		std::string comeArvDate = MJ::MyTime::toString(come_arrive_time, basePlan->m_TimeZone).substr(0, 8);//到达的日期
		if (route_block->_checkIn != "") comeArvDate = route_block->_checkIn;
		bool dayOne17Limit = route_block->_arrive_time >= MJ::MyTime::toTime(comeArvDate + "_17:00", basePlan->m_TimeZone) ? true : false;//17点限制
		std::cerr << "dayOneLimit " << dayOne17Limit << std::endl;
		time_t come_depart_time = route_block->_arrive_time + route_block->_arrive_dur;//到达点的结束时间
		arvKey = new KeyNode(route_block->_arrive_place, come_arrive_time, come_depart_time, comeArvDate, route_block->_arrive_dur, route_block->_arrive_dur, route_block->_arrive_dur, route_block->_arrive_dur, route_block->_arrive_dur, arrive_continous, 0, NODE_FUNC_KEY_ARRIVE, ADJUST_MIN, false, basePlan->m_TimeZone);

		time_t go_depart_time = route_block->_depart_time;//离开点的开始时间
		time_t go_arrive_time = go_depart_time - route_block->_depart_dur;//离开点的结束时间
		std::string goArvDate = MJ::MyTime::toString(go_arrive_time, basePlan->m_TimeZone).substr(0, 8);  // 离开Key的归属日期由上一睡觉Key一致 须修正
		if (route_block->_checkOut != "") goArvDate = route_block->_checkOut;
		deptKey = new KeyNode(route_block->_depart_place, go_arrive_time, go_depart_time, goArvDate, route_block->_depart_dur, route_block->_depart_dur, route_block->_depart_dur, route_block->_depart_dur, route_block->_depart_dur, KEY_NODE_CONTI_NO, 0, NODE_FUNC_KEY_DEPART, ADJUST_MIN, false, basePlan->m_TimeZone);
		//以上到达离开点冲突都需要后续修正

		bool setDay1Limit = false;//第一天的限制 开关~
		if (route_block->_need_hotel && !route_block->_hInfo_list.empty()) {
			// 3 行李点
			// 对于现在的行李点逻辑是有问题的,现在的逻辑是：
			// a.只要有酒店就先生成行李点 b.如果最后城市内是包车或租车,行李点就不放入
			//第一个hotel放行李，不可删除
			HInfo* hInfo = route_block->_hInfo_list[0];
			const LYPlace* hotel = hInfo->m_hotel;

			const TrafficItem* come2HotelTraf = NULL;
			come2HotelTraf = PathTraffic::GetTrafItem(basePlan, route_block->_arrive_place->_ID, hotel->_ID, arvKey->_trafDate);
			if (come2HotelTraf == NULL) return 1;
			time_t leave_luggage_start_time = come_depart_time + come2HotelTraf->_time;
			time_t leave_luggage_stop_time = leave_luggage_start_time + basePlan->m_LeaveLuggageTimeCost;
			//此处 交通信息未放入 后续冲突需要注意可修改~
			leaveLuggageKey = new KeyNode(hotel, leave_luggage_start_time, leave_luggage_stop_time, arvKey->_trafDate, basePlan->m_LeaveLuggageTimeCost, basePlan->m_LeaveLuggageTimeCost, basePlan->m_LeaveLuggageTimeCost, basePlan->m_LeaveLuggageTimeCost, basePlan->m_LeaveLuggageTimeCost, KEY_NODE_CONTI_OK, 0, NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE, ADJUST_NO, false, basePlan->m_TimeZone);

			//最后一个hotel取行李，不可删除
			hInfo = route_block->_hInfo_list[route_block->_hInfo_list.size() - 1];
			hotel = hInfo->m_hotel;

			const TrafficItem* hotel2GoTraf = NULL;
				hotel2GoTraf = PathTraffic::GetTrafItem(basePlan, hotel->_ID, route_block->_depart_place->_ID, deptKey->_trafDate);
			if (hotel2GoTraf == NULL) return 1;
			//go_arrive_time 待改
			time_t reclaim_luggage_stop_time = go_arrive_time - hotel2GoTraf->_time;
			time_t reclaim_luggage_start_time = reclaim_luggage_stop_time - basePlan->m_ReclaimLuggageTimeCost;
			//此处 交通信息未放入 后续冲突需要注意可修改~
			reclaimLuggageKey = new KeyNode(hotel, reclaim_luggage_start_time, reclaim_luggage_stop_time, deptKey->_trafDate, basePlan->m_ReclaimLuggageTimeCost, basePlan->m_ReclaimLuggageTimeCost, basePlan->m_ReclaimLuggageTimeCost, basePlan->m_ReclaimLuggageTimeCost, basePlan->m_ReclaimLuggageTimeCost, KEY_NODE_CONTI_NO, 0, NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE, ADJUST_NO, false, basePlan->m_TimeZone);

			//酒店休息点
			for (int i = 0; i < route_block->_hInfo_list.size(); i++) {
				hInfo = route_block->_hInfo_list[i];
				hotel = hInfo->m_hotel;

				int hotelDayStart = hInfo->m_dayStart;
				int hotelDayEnd = hInfo->m_dayEnd;
				if (hotelDayStart != 0) hotelDayStart++;
				for (int day_idx = hotelDayStart; day_idx <= hotelDayEnd; ++day_idx) {
					time_t day0_time = route_block->_day0_times[day_idx];
					std::string date = route_block->_dates[day_idx];
					time_t yesterday_reach_hotel_time = basePlan->GetDayRange(MJ::MyTime::datePlusOf(date,-1)).second; // 昨天到酒店休息时间 每个酒店都要考虑checkIn和checkOut时还需修正
					if (yesterday_reach_hotel_time < leave_luggage_stop_time) yesterday_reach_hotel_time = leave_luggage_stop_time;

					time_t today_left_hotel_time = basePlan->GetDayRange(date).first; // 今天离开酒店开始去玩
					//hy add: 离开酒店的时间不晚于取行李的开始
					if (today_left_hotel_time > reclaim_luggage_start_time) today_left_hotel_time = reclaim_luggage_start_time;

					int adjustable = ADJUST_MIN;
					//每次都是加前一晚的酒店  day_idx == 0 没有酒店
					if (day_idx == 0) {
						continue;
					} else if (day_idx == route_block->_day_limit - 1) {
						std::string dept_date = MJ::MyTime::toString(go_arrive_time, basePlan->m_TimeZone).substr(0, 8);
						if (yesterday_reach_hotel_time < today_left_hotel_time || (hInfo->m_userCommand && hInfo->m_checkOut != dept_date)) {
							int minDur = basePlan->m_MinSleepTimeCost;
							if (reclaim_luggage_start_time - yesterday_reach_hotel_time < minDur) {//看看 最小时间~
								minDur = std::max(0, static_cast<int>(reclaim_luggage_start_time - yesterday_reach_hotel_time));
							}
							int zipDur = std::max(minDur, static_cast<int>(today_left_hotel_time - std::max(day0_time, yesterday_reach_hotel_time)));
							int rcmdDur = std::max(zipDur, static_cast<int>(today_left_hotel_time - yesterday_reach_hotel_time));
							int extendDur = rcmdDur;
							int maxDur = extendDur;
							int keyContinue = KEY_NODE_CONTI_NO;//控制day1 17点后不再插入景点 直接酒店
							if (basePlan->m_useDay17Limit && dayOne17Limit && setDay1Limit == false) {
								keyContinue = KEY_NODE_CONTI_OK;//控制day1 17点后不再插入景点 直接酒店
							}
							setDay1Limit = true;
							sleepKeyList.push_back(new KeyNode(hotel, yesterday_reach_hotel_time, today_left_hotel_time, date, minDur, zipDur, rcmdDur, extendDur, maxDur, keyContinue, 0, NODE_FUNC_KEY_HOTEL_SLEEP, adjustable, false, basePlan->m_TimeZone));
						}
					} else {
						// 中间其它天睡一晚
						int minDur = basePlan->m_MinSleepTimeCost;
						int rcmdDur = today_left_hotel_time - yesterday_reach_hotel_time;
						int zipDur = today_left_hotel_time - std::max(day0_time, yesterday_reach_hotel_time);
						int extendDur = rcmdDur + 3600;
						int maxDur = extendDur;
						int keyContinue = KEY_NODE_CONTI_NO;//控制day1 17点后不再插入景点 直接酒店
						if (basePlan->m_useDay17Limit && dayOne17Limit && setDay1Limit == false) {
							keyContinue = KEY_NODE_CONTI_OK;//控制day1 17点后不再插入景点 直接酒店
						}
						setDay1Limit = true;
						sleepKeyList.push_back(new KeyNode(hotel, yesterday_reach_hotel_time, today_left_hotel_time, date, minDur, zipDur, rcmdDur, extendDur, maxDur, keyContinue, 0, NODE_FUNC_KEY_HOTEL_SLEEP, adjustable, false, basePlan->m_TimeZone));
					}
				}
			}
		}  // hotel

		FixTrafDate(basePlan, sleepKeyList, reclaimLuggageKey, arvKey, deptKey);
		if (segmentKey) {
			basePlan->m_KeyNode.push_back(segmentKey);
		}
		basePlan->m_KeyNode.push_back(arvKey);
		if (leaveLuggageKey) {
			if (!route_block->_delete_luggage_feasible and sleepKeyList.size()>0) {
				leaveLuggageKey->_notPlan = 1;
				basePlan->m_KeyNode.push_back(leaveLuggageKey);
			} else {
				delete leaveLuggageKey;
				leaveLuggageKey = NULL;
			}
		}
		basePlan->m_KeyNode.insert(basePlan->m_KeyNode.end(), sleepKeyList.begin(), sleepKeyList.end());
		if (reclaimLuggageKey) {
			if (!route_block->_delete_luggage_feasible and sleepKeyList.size()>0) {
				deptKey->_notPlan = 1;
				basePlan->m_KeyNode.push_back(reclaimLuggageKey);
			} else {
				delete reclaimLuggageKey;
				reclaimLuggageKey = NULL;
			}
		}

		if(not route_block->_delete_luggage_feasible and sleepKeyList.size()>0) deptKey->_continuous = KEY_NODE_CONTI_OK;
		basePlan->m_KeyNode.push_back(deptKey);
	}  // for route_block

	for (int i = 0; i < basePlan->m_KeyNode.size(); i ++) {
		basePlan->GetKey(i)->Dump(true);
	}

	return 0;
}

// 调整keynode
int KeyNodeBuilder::MergeKeyNode(Json::Value& req, BasePlan* basePlan) {
	// 1 keynode按离开时间排序
	if (basePlan->m_KeyNode.size() < 2) {
		MJ::PrintInfo::PrintErr("[%s]KeyNodeBuilder::MergeKeyNode, Err size:%d", basePlan->m_qParam.log.c_str(), basePlan->GetKeyNum());
		return 1;
	}
	for (int i = 0,start=0; i <= basePlan->m_KeyNode.size(); i ++) {
		if (i==basePlan->m_KeyNode.size() || basePlan->GetKey(i)->_place == LYConstData::m_segmentPlace) {
			MJ::PrintInfo::PrintLog("start %d, end %d", start , i);
			std::stable_sort(basePlan->m_KeyNode.begin() + start, basePlan->m_KeyNode.begin() + i, KeynodeCmp);//保证前面keynode不能乱动 否则这里会乱~
			start = i + 1;
		}
	}
	for (int i = 0; i < basePlan->m_KeyNode.size(); i ++) {
		basePlan->GetKey(i)->Dump(true);
	}

	MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, Merging keynode before, size:%d", basePlan->m_qParam.log.c_str(), basePlan->GetKeyNum());
	int keyNodeError = 0;

	// 2 开始合并
	for (std::vector<KeyNode*>::iterator it = basePlan->m_KeyNode.begin(); it + 1 != basePlan->m_KeyNode.end();) {
		KeyNode* cur_keynode = *it;
		KeyNode* next_keynode = *(it + 1);
//		cur_keynode->Dump(true);
//		next_keynode->Dump(true);
		// Step 1 计算交叉时间
		int traffic_time = basePlan->m_ATRTimeCost;
		const TrafficItem* traffic_item = NULL;
		if (!cur_keynode || !next_keynode) {
			MJ::PrintInfo::PrintDbg("%d", (it - basePlan->m_KeyNode.begin()));
			return 1;

		}
		traffic_item = PathTraffic::GetTrafItem(basePlan, cur_keynode->_place->_ID, next_keynode->_place->_ID, cur_keynode->_trafDate);
		if (traffic_item) {
			traffic_time = traffic_item->_time;
		}

		int time_cover = 0;
		time_cover = cur_keynode->_close + traffic_time - next_keynode->_open;
		// 无需调整
		if (time_cover <= 0) {
			++it;
			continue;
		}

		MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, %s-->%s, time_cover: %d, close: %s, open: %s, traffic:%d", basePlan->m_qParam.log.c_str(), cur_keynode->_place->_ID.c_str(), next_keynode->_place->_ID.c_str(), time_cover, MJ::MyTime::toString(cur_keynode->_close, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(next_keynode->_open, basePlan->m_TimeZone).c_str(), traffic_time);
		MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, Time conflict, need merge!", basePlan->m_qParam.log.c_str());
		int cur_time_left = 0;
		if (cur_keynode->_adjustable == ADJUST_MIN) {
			cur_time_left = cur_keynode->_close - cur_keynode->_open - cur_keynode->GetMinDur();
		} else if (cur_keynode->_adjustable == ADJUST_FIX) {
			cur_time_left = cur_keynode->_close - cur_keynode->_open - cur_keynode->GetRcmdDur() * basePlan->m_KeyNodeFixRatio;
		}//当前点的可用时间

		MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, CurNode Adj:%d, timeDur:%d, time_left:%d", basePlan->m_qParam.log.c_str(), cur_keynode->_adjustable, cur_keynode->GetRcmdDur(), cur_time_left);
		int next_time_left = 0;
		if (next_keynode->_adjustable == ADJUST_MIN) {
			next_time_left = next_keynode->_close - next_keynode->_open - next_keynode->GetMinDur();
		} else if (next_keynode->_adjustable == ADJUST_FIX) {
			next_time_left = next_keynode->_close - next_keynode->_open - next_keynode->GetRcmdDur() * basePlan->m_KeyNodeFixRatio;
		}//下一个点的可用时间

		MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, NxtNode Adj:%d, timeDur:%d, time_left:%d", basePlan->m_qParam.log.c_str(), next_keynode->_adjustable, next_keynode->GetRcmdDur(), next_time_left);
		if (cur_time_left + next_time_left - time_cover > ERR_THRES) {
			// 能调整
			if (cur_time_left > 0) {
				if (cur_time_left - time_cover > ERR_THRES) {
					cur_keynode->SetClose(cur_keynode->_close - time_cover);
					time_cover = 0;
				} else {
					cur_keynode->SetClose(cur_keynode->_close - cur_time_left);
					time_cover -= cur_time_left;
				}
			}
			next_keynode->SetOpen(next_keynode->_open + time_cover);
			++it;
			MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, Keynodes mergable, CurNode close:%s, NxtNode open:%s", basePlan->m_qParam.log.c_str(), MJ::MyTime::toString(cur_keynode->_close, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(next_keynode->_open, basePlan->m_TimeZone).c_str());
			next_keynode->Dump(true);
		} else {// heavy conflict!
			if (cur_keynode->deletable()) {
				MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, Curnode Deletable:", basePlan->m_qParam.log.c_str());
				MergeKeyNode(cur_keynode, next_keynode, false);
				it = basePlan->m_KeyNode.erase(it);
				if (it - 1 != basePlan->m_KeyNode.begin()) {
					--it;
				}
			} else if (next_keynode->deletable()) {
				MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNode, Nxtnode Deletable:", basePlan->m_qParam.log.c_str());
				MergeKeyNode(cur_keynode, next_keynode, true);
				basePlan->m_KeyNode.erase(it + 1); //指向it的指针仍然有效
			} else {
				MJ::PrintInfo::PrintErr("[%s]KeyNodeBuilder::MergeKeyNode, Keynode Heavily Conflict! Need FaultToLerant!", basePlan->m_qParam.log.c_str());
				keyNodeError ++;//有错但是依然要继续
				it ++;
			}
		}  // if-else
	}  // for

	if (basePlan->m_KeyNode.size() <= 0) {//什么时候出现的情况？待验证~
		basePlan->m_error.Set(55104,  std::string("策略问题，keynode大小为0"));
		return 1;//用备用逻辑
	}
	if (keyNodeError > 0) {
		return 1;//出现过keyNode merge失败时 用备用逻辑
	}

	return 0;
}

int KeyNodeBuilder::CheckNotConfictPerCity(BasePlan* basePlan) {
	int cityArvIdx = -1;
	int cityDeptIdx = -1;
	for(int i = 0 ; i + 1 < basePlan->GetKeyNum(); i ++) {
		const KeyNode* curKeynode = basePlan->GetKey(i);
		const KeyNode* nextKeynode =basePlan->GetKey(i + 1);
		if (!curKeynode || !nextKeynode) {
			continue;
		}
		if (curKeynode->_place == LYConstData::m_segmentPlace || nextKeynode->_place == LYConstData::m_segmentPlace) {
			if (curKeynode->_close > nextKeynode->_open) {
				return 1;
			}
		}
	}
	return 0;
}

int KeyNodeBuilder::MergeKeyNodeFaultToLerant(BasePlan* basePlan) {
	//后续会加前置检验
	MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, Merging keynode before, size:%d", basePlan->m_qParam.log.c_str(), basePlan->GetKeyNum());

	// 1 默认有序
	for (int i = 0; i < basePlan->m_KeyNode.size(); i ++) {
		basePlan->GetKey(i)->Dump(true);
	}

	int ret = CheckNotConfictPerCity(basePlan);
	if (ret) {
		return 1;
	}

	// 2 开始合并
	for (std::vector<KeyNode*>::iterator it = basePlan->m_KeyNode.begin(); it + 1 != basePlan->m_KeyNode.end();) {
		KeyNode* curKeynode = *it;
		KeyNode* nextKeynode = *(it + 1);
//		cur_keynode->Dump(true);
//		next_keynode->Dump(true);
		// Step 1 计算交叉时间
		int trafTime = basePlan->m_ATRTimeCost;
		const TrafficItem* trafItem = NULL;
		trafItem = PathTraffic::GetTrafItem(basePlan, curKeynode->_place->_ID, nextKeynode->_place->_ID, curKeynode->_trafDate);
		if (trafItem) {
			trafTime = trafItem->_time;
		}
		//空交通会返回0时间
		bool zipArv = false;
		bool zipDept = false;
		bool zipTraffic = false;
		bool zeroArvDept = false;
		bool success = false;//控制下方调整结束
		if (basePlan->m_qParam.type == "s202"
			||	basePlan->m_qParam.type == "s125") {
			zipArv = true;
			zipDept = true;
			zeroArvDept =true;
		}
		while (!success) {
			int timeCover = 0;
			timeCover = curKeynode->_close + trafTime - nextKeynode->_open;
			if (timeCover > 0) { //时间不够的话 开始层层压缩时长
				if (!zipArv) {//先压缩到达时间
					if (curKeynode->_type & NODE_FUNC_KEY_ARRIVE) {//当前是到达点时 压缩时长
						int dur = basePlan->GetEntryZipTime(curKeynode->_place);
						curKeynode->SetDur(dur, dur, dur, dur, dur);
						curKeynode->SetClose(curKeynode->_open + dur);
						MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, cur %s(%s) do zip Arv %d ", basePlan->m_qParam.log.c_str(), curKeynode->_place->_ID.c_str(), curKeynode->_place->_name.c_str(), dur);
					}
					if (!basePlan->m_planAfterZip) {//压缩后 不规划
						nextKeynode->_continuous = KEY_NODE_CONTI_OK;//所有点不可达
					}
					zipArv = true;
				} else if (!zipDept) {
					if (nextKeynode->_type & NODE_FUNC_KEY_DEPART) {//只有下一个是离开时 压缩时长
						int dur = basePlan->GetExitZipTime(curKeynode->_place);;
						nextKeynode->SetDur(dur, dur, dur, dur, dur);
						nextKeynode->SetOpen(nextKeynode->_close - dur);
						MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, next %s(%s) do zip Dept %d ", basePlan->m_qParam.log.c_str(), nextKeynode->_place->_ID.c_str(), nextKeynode->_place->_name.c_str(), dur);
					}
					if (!basePlan->m_planAfterZip) {//压缩后 不规划
						nextKeynode->_continuous = KEY_NODE_CONTI_OK;//所有点不可达
					}
					zipDept = true;
				} else if (!zipTraffic) {//交通置零
					trafTime  = 0;
					MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, cur %s(%s) -> next %s(%s) do zero traffic %d ",  basePlan->m_qParam.log.c_str(), curKeynode->_place->_ID.c_str(), curKeynode->_place->_name.c_str(), nextKeynode->_place->_ID.c_str(), nextKeynode->_place->_name.c_str(), trafTime);
					zipTraffic = true;
					if (!basePlan->m_planAfterZip) {//压缩后 不规划
						nextKeynode->_continuous = KEY_NODE_CONTI_OK;//所有点不可达
					}
				} else if (!zeroArvDept) {//到达离开时间为用户设置时间(考虑置为zip仍放不下的情况)
					if (curKeynode->_type & NODE_FUNC_KEY_ARRIVE) {
						int dur = 0;
						curKeynode->SetDur(dur, dur, dur, dur, dur);
						curKeynode->SetClose(curKeynode->_open + dur);
						MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, cur %s(%s) do zero Arv %d ", basePlan->m_qParam.log.c_str(), curKeynode->_place->_ID.c_str(), curKeynode->_place->_name.c_str(), dur);
					}
					if (nextKeynode->_type & NODE_FUNC_KEY_DEPART) {
						int dur = 0;
						nextKeynode->SetDur(dur, dur, dur, dur, dur);
						nextKeynode->SetOpen(nextKeynode->_close - dur);
						MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, next %s(%s) do zero Dept %d ", basePlan->m_qParam.log.c_str(), nextKeynode->_place->_ID.c_str(), nextKeynode->_place->_name.c_str(), dur);
					}
					if (!basePlan->m_planAfterZip) {//压缩后 不规划
						nextKeynode->_continuous = KEY_NODE_CONTI_OK;//所有点不可达
					}
					zeroArvDept = true;
				} else {
					//行程冲突了~ 外层已加报错
					MJ::PrintInfo::PrintErr("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, adjust failed.", basePlan->m_qParam.log.c_str());
//					plan->m_error.Set(55104, "keyNode 调整异常", "当日安排过于紧张，不宜安排任何游玩。");
					return 1;//调整异常 结束
				}
			}
			timeCover = curKeynode->_close + trafTime - nextKeynode->_open;

			// 无需调整
			if (timeCover <= 0) {
				++it;
				success = true;
				continue;
			}

			MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, %s-->%s, time_cover: %d, close: %s, open: %s, traffic:%d", basePlan->m_qParam.log.c_str(), curKeynode->_place->_ID.c_str(), nextKeynode->_place->_ID.c_str(), timeCover, MJ::MyTime::toString(curKeynode->_close, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(nextKeynode->_open, basePlan->m_TimeZone).c_str(), trafTime);
			MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, Time conflict, need merge!", basePlan->m_qParam.log.c_str());
			int curTimeLeft = 0;
			if (curKeynode->_adjustable == ADJUST_MIN) {
				curTimeLeft = curKeynode->_close - curKeynode->_open - curKeynode->GetMinDur();
			} else if (curKeynode->_adjustable == ADJUST_FIX) {
				curTimeLeft = curKeynode->_close - curKeynode->_open - curKeynode->GetRcmdDur() * basePlan->m_KeyNodeFixRatio;
			}//当前点的可用时间

			MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, CurNode Adj:%d, timeDur:%d, time_left:%d", basePlan->m_qParam.log.c_str(), curKeynode->_adjustable, curKeynode->GetRcmdDur(), curTimeLeft);
			int nextTimeLeft = 0;
			if (nextKeynode->_adjustable == ADJUST_MIN) {
				nextTimeLeft = nextKeynode->_close - nextKeynode->_open - nextKeynode->GetMinDur();
			} else if (nextKeynode->_adjustable == ADJUST_FIX) {
				nextTimeLeft = nextKeynode->_close - nextKeynode->_open - nextKeynode->GetRcmdDur() * basePlan->m_KeyNodeFixRatio;
			}//下一个点的可用时间

			MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, NextNode Adj:%d, timeDur:%d, time_left:%d", basePlan->m_qParam.log.c_str(), nextKeynode->_adjustable, nextKeynode->GetRcmdDur(), nextTimeLeft);

			if (curTimeLeft + nextTimeLeft - timeCover > ERR_THRES) {
				// 能调整
				if (curTimeLeft > 0) {
					if (curTimeLeft - timeCover > ERR_THRES) {
						curKeynode->SetClose(curKeynode->_close - timeCover);
						timeCover = 0;
					} else {
						curKeynode->SetClose(curKeynode->_close - curTimeLeft);
						timeCover -= curTimeLeft;
					}
				}
				nextKeynode->SetOpen(nextKeynode->_open + timeCover);
				success = true;
				++it;
				MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, Keynodes mergable, CurNode close:%s, NextNode open:%s", basePlan->m_qParam.log.c_str(), MJ::MyTime::toString(curKeynode->_close, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(nextKeynode->_open, basePlan->m_TimeZone).c_str());
				nextKeynode->Dump(true);
			} else {// heavy conflict!
				MJ::PrintInfo::PrintDbg("[%s]KeyNodeBuilder::MergeKeyNodeFaultToLerant, Keynodes merge failed, CurNode close:%s, NextNode open:%s", basePlan->m_qParam.log.c_str(), MJ::MyTime::toString(curKeynode->_close, basePlan->m_TimeZone).c_str(), MJ::MyTime::toString(nextKeynode->_open, basePlan->m_TimeZone).c_str());
				continue;
			}
		}  // while !success
	}//for

	return 0;
}

// 合并keynode时重写keynode
int KeyNodeBuilder::MergeKeyNode(KeyNode* cur_node, KeyNode* next_node, bool keep_cur) {
	// 不是相同place，删了就删了
	if (cur_node->_place->_ID != next_node->_place->_ID) return 0;
	KeyNode* save_node = NULL;
	if (keep_cur) {
		cur_node->SetClose(std::max(cur_node->_close, next_node->_close));
		cur_node->SetTrafDate(next_node->_trafDate);
		save_node = cur_node;
	} else {
		next_node->SetOpen(cur_node->_open);
		save_node = next_node;
	}
	save_node->SetDur(
			cur_node->GetMinDur() + next_node->GetMinDur(),
			cur_node->GetZipDur() + next_node->GetZipDur(),
			cur_node->GetRcmdDur() + next_node->GetRcmdDur(),
			cur_node->GetExtendDur() + next_node->GetExtendDur(),
			cur_node->GetMaxDur() + next_node->GetMaxDur());

	if(keep_cur) delete next_node;
	else delete cur_node;

	return 0;
}

// 确保最后的dept和reclaimLuggage与最后一天Sleep的trafDate相同,以让这几个点展示在同一个view内
int KeyNodeBuilder::FixTrafDate(BasePlan* basePlan, std::vector<KeyNode*>& sleepKeyList, KeyNode* reclaimLuggageKey, KeyNode* arvKey, KeyNode* deptKey) {
	if (sleepKeyList.empty() || !reclaimLuggageKey) {
		// arv与dept算一天
		deptKey->_trafDate = arvKey->_trafDate;
	} else {
		KeyNode* lastSleepKey = sleepKeyList.back();
		if (reclaimLuggageKey->_trafDate != lastSleepKey->_trafDate) {
			reclaimLuggageKey->_trafDate = lastSleepKey->_trafDate;
			const TrafficItem* hotel2GoTraf = PathTraffic::GetTrafItem(basePlan, reclaimLuggageKey->_place->_ID, deptKey->_place->_ID, reclaimLuggageKey->_trafDate);
			if (hotel2GoTraf == NULL) return 1;
			time_t reclaimLuggageStop = deptKey->_open - hotel2GoTraf->_time;
			time_t reclaimLuggageStart = reclaimLuggageStop - reclaimLuggageKey->GetRcmdDur();
			reclaimLuggageKey->SetOpen(reclaimLuggageStart);
			reclaimLuggageKey->SetClose(reclaimLuggageStop);
		}
		if (deptKey->_trafDate != reclaimLuggageKey->_trafDate) {
			deptKey->_trafDate = reclaimLuggageKey->_trafDate;
		}
	}
	return 0;
}

int KeyNodeBuilder::NotPlanBetweenSomeKeynodeByDate(Json::Value& req, BasePlan* basePlan) {
	int len = basePlan->GetKeyNum();
	for (int i = 1; i < len; i ++) {
		KeyNode* curKey = basePlan->GetKey(i);
		KeyNode* lastKey = basePlan->GetKey(i - 1);
		for(auto it = basePlan->m_notPlanDateSet.begin(); it != basePlan->m_notPlanDateSet.end(); it ++) {
			std::string date = (*it);
			if (lastKey->_trafDate != date) {//参考部分 trafDate 的内容
				continue;
			}
			time_t day_00_00 = MJ::MyTime::toTime(date + "_00:00", basePlan->m_TimeZone);
			time_t second_day_play_start = basePlan->GetDayRange(MJ::MyTime::datePlusOf(date,1)).first;
			if (lastKey->_close >= day_00_00 && curKey->_open <= second_day_play_start) {//满足时间直接相连
				curKey->_continuous = KEY_NODE_CONTI_OK;
				curKey->_notPlan = 1;
			}
		}
	}

	for (int i = 0; i < basePlan->m_KeyNode.size(); i ++) {
		basePlan->GetKey(i)->Dump(true);
	}

	return 0;
}

int KeyNodeBuilder::ChangeContinueForDayOneLast3HNotPlan(Json::Value& req, BasePlan* basePlan) {
	int len = basePlan->GetKeyNum();
	int start = 0, end = 0;
	for (int i = 0 ; i < len ; i ++) {
		KeyNode* curKeynode = basePlan->GetKey(i);
		end = i;
		if ( (curKeynode->_type & NODE_FUNC_KEY_SEGMENT) || i + 1 >= len) {
			KeyNode* arvKey = NULL;
			KeyNode* leaveLuggageKey = NULL;
			KeyNode* firstSleepHotel = NULL;
			KeyNode* lastSleepHotel = NULL;
			KeyNode* reclaimLuggageKey = NULL;
			KeyNode* deptKey = NULL;
			std::cerr << "start " << start << " end " << end << std::endl;
			for (int j = start ; j <= end; j ++) {
				curKeynode = basePlan->GetKey(j);
				if (curKeynode->_type & NODE_FUNC_KEY_ARRIVE) {
					arvKey = curKeynode;
				} else if (curKeynode->_type & NODE_FUNC_KEY_DEPART) {
					deptKey = curKeynode;
				} else if (curKeynode->_type & NODE_FUNC_KEY_HOTEL_LEFT_LUGGAGE) {
					leaveLuggageKey = curKeynode;
				} else if (curKeynode->_type & NODE_FUNC_KEY_HOTEL_RECLAIM_LUGGAGE) {
					reclaimLuggageKey = curKeynode;
				} else if (curKeynode->_type & NODE_FUNC_KEY_HOTEL_SLEEP) {
					if (firstSleepHotel == NULL) {
						firstSleepHotel = curKeynode;
						lastSleepHotel = curKeynode;
					} else {
						lastSleepHotel = curKeynode;
					}
				}
			}
			if (!arvKey || !deptKey) {
				MJ::PrintInfo::PrintLog("[%s]ChangeContinueForDayOneLast3HNotPlan lost Keynode", basePlan->m_qParam.log.c_str());
				return 1;
			}
			//第一天 最早开始时间
			int timeHourLimit = 3 * 3600;

			time_t dayOneStartTime;
			dayOneStartTime = arvKey->_close;
			if (leaveLuggageKey) {
				dayOneStartTime = leaveLuggageKey->_close;
			}
			if (firstSleepHotel) {
				if (dayOneStartTime + timeHourLimit > firstSleepHotel->_open) {
					firstSleepHotel->_continuous = KEY_NODE_CONTI_OK;
				}
			}
			else if (reclaimLuggageKey) {//有行李的话
				if (dayOneStartTime + timeHourLimit > reclaimLuggageKey->_open) {
					reclaimLuggageKey->_continuous = KEY_NODE_CONTI_OK;
				}
			}
			else {
				if (dayOneStartTime + timeHourLimit > deptKey->_open) {
					deptKey->_continuous = KEY_NODE_CONTI_OK;
				}
			}

			if (lastSleepHotel) {
				if (reclaimLuggageKey) {
					if (lastSleepHotel->_close + timeHourLimit > reclaimLuggageKey->_open) {//时间不足
						reclaimLuggageKey->_continuous = KEY_NODE_CONTI_OK;
					}
				}
				else {
					if (lastSleepHotel->_close + timeHourLimit > deptKey->_open) {
						deptKey->_continuous = KEY_NODE_CONTI_OK;
					}
				}
			}

			start = end + 1;
		}
	}

	return 0;
}

int BlockBuilder::BuildBlock(Json::Value& req, BasePlan* basePlan) {
	char buff[100];
	MJ::PrintInfo::PrintDbg("[%s]BlockBuilder::BuildBlock, key size:%d", basePlan->m_qParam.log.c_str(), basePlan->GetKeyNum());
	for (int i = 0; i < basePlan->GetKeyNum(); ++i) {
		basePlan->GetKey(i)->Dump();
	}

	basePlan->m_availDur = 0;
	bool needHotel = false;//hy 若不需要酒店则酒店的开关门不能影响每个block的起止时间
	if (!basePlan->m_RouteBlockList.empty()) needHotel = basePlan->m_RouteBlockList[0]->_need_hotel;

	for (int i = 1; i < basePlan->GetKeyNum(); ++i) {
		const KeyNode* lastKey = basePlan->GetKey(i-1);
		const KeyNode* keynode = basePlan->GetKey(i);
		time_t start = lastKey->_close;
		time_t stop = keynode->_open;

		if (keynode->_continuous == KEY_NODE_CONTI_OK) {
			int availDur = stop - start;
			availDur = std::max(0, availDur);
			if(keynode->_notPlan) availDur = -1;
			TimeBlock * timeBlock = new TimeBlock(start, stop, lastKey->_trafDate, availDur, NODE_FUNC_NULL, basePlan->m_TimeZone);
			timeBlock->SetNotPlay(true);
			basePlan->m_availDur += availDur;
			basePlan->m_BlockList.push_back(timeBlock);
		} else {
			std::string date = MJ::MyTime::toString(start, basePlan->m_TimeZone, "%Y%m%d");
			snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
			int minStart = basePlan->GetDayRange(date).first;
			if (start < minStart && minStart < stop && needHotel) {
				start = minStart;
			}
			date = MJ::MyTime::toString(stop, basePlan->m_TimeZone, "%Y%m%d");
			snprintf(buff, sizeof(buff), "%s_00:00", date.c_str());
			int maxStop = basePlan->GetDayRange(date).second;
			if (stop > maxStop && maxStop > start && needHotel) {
				stop = maxStop;
			}

			int availDur = stop - start;
			availDur = std::max(0, availDur);
			//block的availDur 最大8 有问题 ???
			availDur = std::min(3600*8,availDur);

			basePlan->m_availDur += availDur;
			basePlan->PushBlock(start, stop, lastKey->_trafDate, availDur, NODE_FUNC_NULL, basePlan->m_TimeZone);
		}
	}

	MJ::PrintInfo::PrintDbg("[%s]BlockBuilder::BuildBlock, block size:%d", basePlan->m_qParam.log.c_str(), basePlan->GetBlockNum());
	for (int i = 0; i < basePlan->GetBlockNum(); ++i) {
		basePlan->GetBlock(i)->Dump(true);
	}

	return 0;
}
