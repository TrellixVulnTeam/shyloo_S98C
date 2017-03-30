#define ATTR_EXPORT
#include "AttrGetter.h"
#include "Attr.h"
#include "IDCCenter.h"

void getAttrProp(IObjectMgr* objectMgr);

bool AttrGetter::initialize(sl::api::IKernel * pKernel){
	_kernel = pKernel;
	FIND_MODULE(_objectMgr, ObjectMgr);

	getAttrProp(_objectMgr);
	return true;
}

bool AttrGetter::launched(sl::api::IKernel * pKernel){
	return true;
}

bool AttrGetter::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

const IProp* attr_def::exp = nullptr;
const IProp* attr_def::gate = nullptr;
const IProp* attr_def::id = nullptr;
const IProp* attr_def::status = nullptr;

void getAttrProp(IObjectMgr* objectMgr){
	attr_def::exp = objectMgr->getPropByName("exp");
	attr_def::gate = objectMgr->getPropByName("gate");
	attr_def::id = objectMgr->getPropByName("id");
	attr_def::status = objectMgr->getPropByName("status");
}
