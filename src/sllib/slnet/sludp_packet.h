#ifndef _SL_SOCKETUDPPACKET_H_
#define _SL_SOCKETUDPPACKET_H_
#include "slpacket.h"
namespace sl
{
namespace network
{
class EndPoint;
class Address;
class UDPPacket: public Packet
{
public:
	typedef SLShared_ptr<SmartPoolObject<UDPPacket>> SmartPoolObjectPtr;
	static SmartPoolObjectPtr createSmartPoolObj();
	static CObjectPool<UDPPacket>& ObjPool();
	static UDPPacket* createPoolObject();
	static void reclaimPoolObject(UDPPacket* obj);
	static void destroyObjPool();
	static size_t maxBufferSize();

	UDPPacket(size_t res = 0);
	virtual ~UDPPacket(void);

	int recvFromEndPoint(EndPoint& ep, Address* pAddr = NULL);

	virtual void onReclaimObject();
};

typedef SmartPoolObject<UDPPacket> UDPPacketPtr;
}
}
#endif