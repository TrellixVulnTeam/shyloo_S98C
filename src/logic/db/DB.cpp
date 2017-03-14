#include "DB.h"
#include "IHarbor.h"
#include "NodeDefine.h"
#include "NodeProtocol.h"
#include "DBTaskCall.h"

bool DB::initialize(sl::api::IKernel * pKernel){
	_kernel = pKernel;
	return true;
}

bool DB::launched(sl::api::IKernel * pKernel){
	FIND_MODULE(_harbor, Harbor);

	return true;
}

bool DB::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

IDBCall* DB::create(int64 threadId, const int64 id, const char* file, const int32 line, const void* context, const int32 size = 0){

}


