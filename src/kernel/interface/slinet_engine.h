#ifndef KERNEL_INET_ENGINE_H
#define KERNEL_INET_ENGINE_H
#include "slicore.h"
#include "slikernel.h"
namespace sl
{
namespace network
{
class INetEngine: public core::ICore
{
public:
	virtual bool addTcpServer(sl::api::ITcpServer* server, const char* ip, const short port, int sendSize, int recvSize) = 0;
	virtual bool addTcpClient(sl::api::ITcpSession* session, const char* ip, const short port, int sendSize, int recvSize) = 0;
};

}
}

#endif