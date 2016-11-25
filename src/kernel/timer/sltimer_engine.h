#ifndef SL_TIMER_ENGINE_H
#define SL_TIMER_ENGINE_H
#include "slitimer_engine.h"
#include "slsingleton.h"
#include "sltimer.h"
#include "slkr_timer.h"

namespace sl
{
namespace core
{
class TimerEngine: public ITimerEngine, public CSingleton<TimerEngine>
{
private:
	TimerEngine();
	~TimerEngine();

public:
	virtual bool ready();
	virtual bool initialize();
	virtual bool destory();

	virtual bool startTimer(api::ITimer* pTimer, int64 delay, int32 count, int64 interval);
	virtual bool killTimer(api::ITimer* pTimer);
	virtual bool pauseTimer(api::ITimer* pTimer);
	virtual bool resumeTimer(api::ITimer* pTimer);
	virtual int64 processing();

	timer::SLTimerHandler getTimerHander(api::ITimer* pTimer);

private:
	timer::ISLTimerMgr*		m_pTimerMgr;

};
}
}

#endif