#include "slnet_session.h"
#include "slkernel.h"
namespace sl{
namespace core{
#define SINGLE_RECV_SIZE 32768
int32 NetPacketParser::parsePacket(const char* pDataBuf, int32 len){
	if (!pDataBuf)
		return -1;
	
	if (len < 2 * sizeof(int32))
		return 0;

	int32 dwLen = *(int32*)(pDataBuf + sizeof(int32));
	if (dwLen > SINGLE_RECV_SIZE)
		return -1;

	if (len >= dwLen)
		return dwLen;
	else 
		return 0;
}

NetSession::NetSession(ITcpSession* pTcpSession)
	:_tcpSession(pTcpSession),
	 _channel(NULL)
{
	_tcpSession->_pipe = this;
}

NetSession::~NetSession(){
	//m_pTcpSession->close();
}

void NetSession::setChannel(ISLChannel* pChannel){
	_channel = pChannel;
}

void NetSession::release(){
	RELEASE_POOL_OBJECT(NetSession, this);
}

void NetSession::onRecv(const char* pBuf, uint32 dwLen){
	_tcpSession->onRecv(core::Kernel::getInstance(), pBuf, dwLen);
}

void NetSession::onEstablish(){
	_tcpSession->onConnected(core::Kernel::getInstance());
}

void NetSession::onTerminate(){
	_tcpSession->onDisconnect(core::Kernel::getInstance());
}

void NetSession::send(const void* pContext, int dwLen){
	_channel->send((const char*)pContext, dwLen);
}

void NetSession::close(){
	return _channel->disconnect();
}

const char* NetSession::getRemoteIP(){
	return _channel->getRemoteIPStr();
}

void NetSession::adjustSendBuffSize(const int32 size){
	_channel->adjustSendBuffSize(size);
}
void NetSession::adjustRecvBuffSize(const int32 size){
	_channel->adjustRecvBuffSize(size);
}

ISLSession* ServerSessionFactory::createSession(ISLChannel* poChannel){
	if(NULL == _server)
		return NULL;

	ITcpSession* pTcpSession = _server->mallocTcpSession(core::Kernel::getInstance());
	if(NULL == pTcpSession)
	{
		SLASSERT(false, "wtf");
		return NULL;
	}
	NetSession* pNetSession = CREATE_POOL_OBJECT(NetSession, pTcpSession);
	if(NULL == pNetSession)
	{
		SLASSERT(false, "wtf");
		return NULL;
	}

	pNetSession->setChannel(poChannel);
	pNetSession->setTcpSession(pTcpSession);
	return pNetSession;
}

}
}