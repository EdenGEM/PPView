#include "MYHotUpdate.h"
#include "Route/base/LYConstData.h"
#include "Route/base/PrivateConstData.h"

void* PPViewUpdate::reloadData(const std::vector<const MJ::MJHotUpdateItem*>& items, const void* curPtr) {
	int ret = 0;
	PrivateConstData* ppviewUpdate_new = new PrivateConstData();
	ret = ppviewUpdate_new->Copy((PrivateConstData*)(curPtr));//进行拷贝数据
    if (ret != 0) {
	    MJ::PrintInfo::PrintLog("PPViewUpdate::reloadData, LYConstData::PrivateDataCopy fail!");
		deleteSharedData(ppviewUpdate_new);
		return NULL;
	}
	for (int i = 0 ; i < items.size(); i ++) {
		const MJ::MJHotUpdateItem* mjHotUpItem = items[i];
		if (mjHotUpItem->needUpdate == 0) {
			continue;
		}
		MJ::PrintInfo::PrintLog("sleep time %ds", mjHotUpItem->defer);
		sleep(mjHotUpItem->defer);
		MJ::PrintInfo::PrintLog("lastTS %s, newTS %s, database %s, table %s", mjHotUpItem->lastTS.c_str(), mjHotUpItem->newTS.c_str(), mjHotUpItem->database.c_str(), mjHotUpItem->table.c_str());
		int tret = ppviewUpdate_new->Update(mjHotUpItem->lastTS, mjHotUpItem->newTS, mjHotUpItem->database, mjHotUpItem->table, mjHotUpItem->dur, mjHotUpItem->rowCnt);
		if (tret) {
			ret = tret;
		}
	}
	return ppviewUpdate_new;
}


int PPViewUpdate::deleteSharedData(void* ptr) {
	PrivateConstData* ppviewUpdate = (PrivateConstData*)ptr;
	delete ppviewUpdate;
	return 0;
}
