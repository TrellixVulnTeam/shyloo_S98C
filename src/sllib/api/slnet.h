#ifndef SL_SLNET_H
#define SL_SLNET_H
#include "sltype.h"
namespace sl
{
namespace network
{
class ISLSession;
class ISLSessionFactory;

class ISLChannel
{
public:
	virtual bool SLAPI isConnected(void) = 0;
	virtual void SLAPI send(const char* pBuf, unsigned int dwLen) = 0;
	virtual void SLAPI disconnect(void) = 0;
	
	virtual const unsigned int SLAPI getRemoteIP(void) = 0;
	virtual const char* SLAPI getRemoteIPStr(void) = 0;
	virtual const unsigned short SLAPI getRemotePort(void) = 0;

	virtual const unsigned int SLAPI getLocalIP(void) = 0;
	virtual const char* SLAPI getLocalIPStr(void) = 0;
	virtual const unsigned short SLAPI getLocalPort(void) = 0;
};

class ISLListener
{
public:
	virtual void SLAPI setSessionFactory(ISLSessionFactory* poSessionFactory) = 0;

	virtual void SLAPI setBufferSize(unsigned int dwRecvBufSize, unsigned int dwSendBufSize) = 0;

	virtual bool SLAPI start(const char* pszIP, unsigned short wPort, bool bReUseAddr = true) = 0;

	virtual bool SLAPI stop(void) = 0;
	
	virtual void SLAPI release(void) = 0;

};

class ISLSession
{
public:
	virtual void SLAPI setChannel(ISLChannel* poChannel) = 0;
	virtual void SLAPI release(void) = 0;
	virtual void SLAPI onRecv(const char* pBuf, unsigned int dwLen) = 0;
};

class ISLSessionFactory
{
public:
	virtual ISLSession* SLAPI createSession(ISLChannel* poChannel) = 0;
};

class ISLNet
{
public:

	virtual ISLListener* SLAPI createListener() = 0;
};

ISLNet* __cdecl getSLNetModule(void);
}
}


#endif