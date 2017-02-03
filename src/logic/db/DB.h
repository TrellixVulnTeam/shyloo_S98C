#ifndef __SL_CORE_DBMGR_H__
#define __SL_CORE_DBMGR_H__
#include "IDB.h"
#include <unordered_map>
#include "slobjectpool.h"
#include "slikernel.h"
#include <list>

#define INVAILD_CB_ID -1

class IHarbor;
class DB :public IDB
{
public:
	typedef std::unordered_map<int32, std::list<sl::api::ICacheDataResult::DataReadFuncType>> TASK_CALL_TYPE;

	virtual bool initialize(sl::api::IKernel * pKernel);
	virtual bool launched(sl::api::IKernel * pKernel);
	virtual bool destory(sl::api::IKernel * pKernel);

	virtual void execDBTask(sl::api::IDBTask* pTask, int32 cbID, const OArgs& args);
	virtual void rgsDBTaskCallBack(int32 messageId, sl::api::ICacheDataResult::DataReadFuncType handler);
	static void dealTaskCompleteCB(sl::api::IKernel * pKernel, int32 cbID, const sl::api::ICacheDataResult& result);

private:
	sl::api::IKernel*	m_kernel;
	IHarbor*			m_harbor;
	static TASK_CALL_TYPE m_dbTaskCompleteDB;
};



#endif