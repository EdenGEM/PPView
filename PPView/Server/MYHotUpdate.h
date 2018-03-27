#ifndef __MY_HOT_UPDATE_H__
#define __MY_HOT_UPDATE_H__
#include "http/MJHotUpdate.h"

class PPViewUpdate : public MJ::MJHotUpdate {
	public:
		//curPtr表示当前内存中的共享数据指针 增量更新需要用到该数据 
		virtual void* reloadData(const std::vector<const MJ::MJHotUpdateItem*>& items, const void* curPtr); 
		//删除旧数据(销毁失败会引起内存泄露) 
		virtual int deleteSharedData(void* ptr);
};

#endif
