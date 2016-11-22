#include "slnet_engine.h"
namespace sl
{
SL_SINGLETON_INIT(network::NetEngine);
namespace network
{

bool NetEngine::initialize()
{
	m_pSLNetModule = SLNetGetModule();
	if(NULL == m_pSLNetModule)
		return false;
	return true;
}

bool NetEngine::addTcpServer(sl::api::ITcpServer* server, const char* ip, const short port, int sendSize, int recvSize)
{
	if(NULL == m_pSLNetModule)
		return false;

	m_pListener = m_pSLNetModule->CreateListener();
	if(NULL == m_pListener)
		return false;

	m_pListener->setBufferSize(recvSize, sendSize);
	m_pListener->setSessionFactory();
	if(!m_pListener->start(ip, port)){
		//SL_ASSERT(false);
		return false;
	}
	return true;
}
bool NetEngine::adddTcpClient(sl::api::ITcpSession* session, const char* ip, const short port, int sendSize, int recvSize)
{

}

}
}