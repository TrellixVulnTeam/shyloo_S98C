#ifndef SL_KERNEL_IKERNEL_H
#define SL_KERNEL_IKERNEL_H
#include "slmulti_sys.h"
#include "slthread.h"
#include "sldb.h"
#include "slargs.h"
#include <functional>
namespace sl
{
namespace api
{
class IKernel;
class IModule;
class IPipe{
public:
	virtual ~IPipe() {}
	virtual void send(const void* pContext, int dwLen) = 0;
	virtual void close() = 0;
	virtual const char* getRemoteIP() = 0;
	virtual void adjustSendBuffSize(const int32 size) = 0;
	virtual void adjustRecvBuffSize(const int32 size) = 0;
};

class ISocket{
public:
	virtual ~ISocket(){}
	IPipe* _pipe;
};

class ITcpSession: public ISocket{
public:
	virtual ~ITcpSession(){}
	virtual int32 onRecv(IKernel* pKernel, const char* pContext, int dwLen) = 0;
	virtual void onConnected(IKernel* pKernel) = 0;
	virtual void onDisconnect(IKernel* pKernel) = 0;

	inline void send(const void* pContext, const int32 size){
		if (_pipe)
			_pipe->send(pContext, size);
	}

	inline void adjustSendBuffSize(const int32 size){
		if (_pipe)
			_pipe->adjustSendBuffSize(size);
	}

	inline void adjustRecvBuffSize(const int32 size){
		if (_pipe)
			_pipe->adjustRecvBuffSize(size);
	}

	inline const char* getRemoteIP(){
		if (_pipe)
			return _pipe->getRemoteIP();
		return "";
	}

	inline void close(){
		if (_pipe)
			_pipe->close();
	}
};

class ITcpServer{
public:
	virtual ~ITcpServer(){}
	virtual ITcpSession* mallocTcpSession(IKernel* pKernel) = 0;
	virtual void setListenPort(uint16 port) = 0;
};

class IIPCServer{
public:
	virtual ~IIPCServer(){}
	virtual ITcpSession* mallocIPCSession(IKernel* pKernel) = 0;
};

class ITimer;
class ITimerBase{
public:
	virtual ~ITimerBase(){}
	virtual void setITimer(ITimer* pITimer) = 0;
};

class ITimer{
public:
	virtual ~ITimer(){}

	virtual void onInit(IKernel* pKernel, int64 timetick){}
	virtual void onStart(IKernel* pKernel, int64 timetick) = 0;
	virtual void onTime(IKernel* pKernel, int64 timetick) = 0;
	virtual void onTerminate(IKernel* pKernel, bool beForced, int64 timetick) = 0;
	virtual void onPause(IKernel* pKernel, int64 timetick) = 0;
	virtual void onResume(IKernel* pKernel, int64 timetick) = 0;

	virtual ITimerBase* getITimerBase() const { return _timerBase; }

	virtual void setITimerBase(ITimerBase* pITimerBase){
		_timerBase = pITimerBase;
	}

private:
	ITimerBase* _timerBase;
};

class IAsyncBase {
public:
	virtual ~IAsyncBase() {}
};

class IAsyncHandler{
public:
	IAsyncHandler() :_base(nullptr){}
	virtual ~IAsyncHandler() {}

	inline void setBase(IAsyncBase* base) { _base = base; }
	inline IAsyncBase* getBase() { return _base; }

	virtual bool onExecute(IKernel* pKernel) = 0;
	virtual bool onSuccess(IKernel* pKernel) = 0;
	virtual bool onFailed(IKernel* pKernel, bool nonviolent) = 0;
	virtual void onRelease(IKernel* pKernel) = 0;

private:
	IAsyncBase* _base;
};

class ILogger{
public:
	virtual ~ILogger() {}
	virtual void pushHeader(const char* header) = 0;
	virtual void setFileHandler(const char* file) = 0;
	virtual void setSyslogHandler(const char* file) = 0;
	virtual void setLogLevel(const int32 level) = 0;
	virtual void setLogPath(const char* path) = 0;

	virtual void traceLog(const char* format, ...) = 0; 
	virtual void errorLog(const char* format, ...) = 0;
	virtual void infoLog(const char* format, ...) = 0;
	virtual void debugLog(const char* format, ...) = 0;
	virtual void warningLog(const char* format, ...) = 0;
	virtual void fatalLog(const char* format, ...) = 0;
	
	virtual const char* headers() const  = 0;
	virtual const char* fileHandler() const  = 0;
	virtual const char* sysLogHandler() const = 0;
	virtual const int32 logLevel() const = 0;
	virtual const char* logPath() const = 0;

};

class IKernel{
public:
	virtual ~IKernel() {}
	virtual bool ready() = 0;
	virtual bool initialize(int32 argc, char ** argv) = 0;
	virtual bool destory() = 0;

	//net interface
	virtual bool startTcpServer(api::ITcpServer * server, const char* ip, const int32 port, int32 sendSize = 0, int32 recvSize = 0) = 0;
	virtual bool startTelnetServer(sl::api::ITcpServer* server, const char* ip, const int32 port) = 0;
	virtual bool startTcpClient(api::ITcpSession * client, const char* ip, const int32 port, int32 sendSize = 0, int32 recvSize = 0) = 0;
	virtual const char* getInternetIp() = 0;
	virtual const char* getLocalIp() = 0;
	virtual uint64 getSpareTime() = 0;
	virtual void clearSpareTime() = 0;

	//ipc interface
	virtual bool addIPCServer(sl::api::ITcpServer* server, const int64 serverId) = 0;
	virtual bool addIPCClient(sl::api::ITcpSession* session, const int64 clientId, const int64 serverId, const int32 sendSize = 0, const int32 recvSize = 0) = 0;

	//timer interface
	virtual bool startTimer(api::ITimer* timer, int64 delay, int32 count, int64 interval, const char* file, const int32 line) = 0;
	virtual bool killTimer(api::ITimer* timer) = 0;
	virtual void pauseTimer(api::ITimer* timer) = 0;
	virtual void resumeTimer(api::ITimer* timer) = 0;

	// logic interface
	virtual IModule * findModule(const char * name) = 0;
	virtual const char* getCmdArg(const char* name) = 0;
	
	//async interface
	virtual void startAsync(const int64 threadId, IAsyncHandler* handler, const char* debug) = 0;
	virtual void stopAsync(IAsyncHandler* handler) = 0;

	//log interface
	virtual void syncLog(int32 filter, const char* log, const char* file, const int32 line) = 0;
//`	virtual void asyncLog(int32 filter, const char* log, const char* file, const int32 line) = 0;
	virtual ILogger* createLogger() = 0;
	
	virtual bool reloadCoreConfig(const char* coreFile) = 0;

	virtual void shutdown() = 0;
	virtual bool isShutdown() = 0;
};

extern "C" KERNEL_API sl::api::IKernel* getCore();

template <typename T>
struct SLModule{
	inline static T * instance(const char* module){
		static T* g_instance = nullptr;
		if (!g_instance){
			g_instance = (T*)getCore()->findModule(module);
			SLASSERT(g_instance, "where is module %s", module);
		}
		return g_instance;
	}
};
}
}

#define START_TIMER(timer, delay, count, interval){		\
	pKernel->startTimer(timer, delay, count, interval, __FILE__, __LINE__);	\
}

#define FIND_MODULE(m, name) {\
	m = (I##name *)pKernel->findModule(#name); \
	SLASSERT(m, "where is %s", #name); \
}

#define TIMER_BEAT_FOREVER	-1

#define SLMODULE(name) (sl::api::SLModule<I##name>::instance(#name))
/*
#define INFO_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
	getLogger()->asyncLog(0x8, log, __FILE__, __LINE__); \
}

#define DEBUG_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
	getLogger()->asyncLog(0x10, log, __FILE__, __LINE__); \
}

#define WARNING_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
	getLogger()->asyncLog(0x4, log, __FILE__, __LINE__); \
}

#define ERROR_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
	getLogger()->asyncLog(0x2, log, __FILE__, __LINE__); \
}

#define TRACE_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
	getLogger()->asyncLog(0x20, log, __FILE__, __LINE__); \
}

#define IMPORTANT_LOG(format, ...) { \
	char log[8192]; \
	SafeSprintf(log, 8192, format, ##__VA_ARGS__); \
}
	//syncLog(0x10, log, __FILE__, __LINE__); \
}
*/
#endif
