#ifndef SL_SLTIMER_H
#define SL_SLTIMER_H
#include "sltype.h"
namespace sl
{
namespace timer
{

class ISLTimer
{
public:
	virtual void SLAPI onInit(int64 timetick) = 0;
	virtual void SLAPI onStart(int64 timetick) = 0;
	virtual void SLAPI onTime(int64 timetick) = 0;
	virtual void SLAPI onTerminate(int64 timetick) = 0;
	virtual void SLAPI onPause(int64 timetick) = 0;
	virtual void SLAPI onResume(int64 timetick) = 0;
};

typedef void* SLTimerHandle;

class ISLTimerMgr
{
public:
	virtual SLTimerHandle SLAPI startTimer(ISLTimer* pTimer, int64 delay, int32 count, int64 interval) = 0;
	virtual bool SLAPI killTimer(SLTimerHandle pTimer) = 0;
	virtual void SLAPI pauseTimer(SLTimerHandle pTimer) = 0;
	virtual void SLAPI resumeTimer(SLTimerHandle pTimer) = 0;
	virtual int SLAPI process(uint64 now) = 0;
};

ISLTimerMgr* SLAPI getSLTimerModule(void);
}
}
#endif
