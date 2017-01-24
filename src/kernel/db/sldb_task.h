#ifndef __SL_CORE_DB_TASK_H__
#define __SL_CORE_DB_TASK_H__
#include "slthread.h"
#include "sldb.h"
#include "slikernel.h"
#include "slobjectpool.h"
namespace sl
{
namespace core
{
class DBTask : public sl::thread::ITPTask{
public:
	DBTask();
	DBTask(api::IDBTask* pTask);
	~DBTask();

	virtual bool SLAPI start(){ return true; }
	virtual bool SLAPI process();
	virtual bool SLAPI end(){ return true; }
	virtual thread::TPTaskState SLAPI presentMainThread();
	virtual void SLAPI release();

	static DBTask* newDBTask(api::IDBTask* pTask);
	
private:
	sl::db::ISLDBConnection*	m_dbConnection;
	sl::api::IDBTask*			m_dbTask;
};
CREATE_OBJECT_POOL(DBTask);

}
}
#endif