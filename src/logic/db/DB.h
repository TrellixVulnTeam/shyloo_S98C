#ifndef __SL_CORE_DBMGR_H__
#define __SL_CORE_DBMGR_H__
#include "IDB.h"
#include <unordered_map>
#include "slikernel.h"
#include <list>
#include "slsingleton.h"

class IHarbor;
class IMysqlMgr;
class DataBase;
class DB : public IDB, public sl::api::ITimer, public sl::SLHolder<DB> {
public:
	virtual bool initialize(sl::api::IKernel * pKernel);
	virtual bool launched(sl::api::IKernel * pKernel);
	virtual bool destory(sl::api::IKernel * pKernel);
	
	IMysqlMgr* getMysqlMgr(){ return _mysql; }
	virtual void onStart(sl::api::IKernel* pKernel, int64 timetick){}
	virtual void onTime(sl::api::IKernel* pKernel, int64 timetick);
	virtual void onTerminate(sl::api::IKernel* pKernel, bool beForced, int64 timetick){}
	virtual void onPause(sl::api::IKernel* pKernel, int64 timetick){}
	virtual void onResume(sl::api::IKernel* pKernel, int64 timetick){}

	void test();

private:
	sl::api::IKernel*	_kernel;
	DB*					_self;
	IDBInterface*		_dbInterface;
	IHarbor*			_harbor;
	IMysqlMgr*			_mysql;
	DataBase*			_database;
};



#endif
