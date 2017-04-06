#ifndef __SL_LIB_REDIS_MGR_H__
#define __SL_LIB_REDIS_MGR_H__
#include <string>
#include <vector>
#include "slpool.h"
#include "slredis_connection.h"

namespace sl{
namespace db{
class SLRedisMgr: public ISLRedisMgr{
public:
	ISLRedisConnection* SLAPI create(const char* ip, const int32 port, const int32 timeout = CONNECT_TIME_OUT);
	void SLAPI release(void) { DEL this; }
	void recover(ISLRedisConnection* conn);
	
private:
	sl::SLPool<SLRedisConnection> _redisConnPool;
};
}
}

#endif