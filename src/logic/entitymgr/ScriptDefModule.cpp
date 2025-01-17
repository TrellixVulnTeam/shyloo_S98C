#include "ScriptDefModule.h"
#include "slxml_reader.h"
#include "slfile_utils.h"
#include "IDCCenter.h"
#include "DataTypeMgr.h"
#include "NodeDefine.h"
#include "slstring_utils.h"
#include "EntityDef.h"
#include "NodeProtocol.h"
//#include "IMmoServer.h"
#include "GameDefine.h"
#include "ScriptObject.h"
#include "IHarbor.h"
#include "Attr.h"

ScriptDefModule::ENTITY_FLAGS_MAP	ScriptDefModule::s_entityFlagMapping;
ScriptDefModule::ScriptDefModule(const char* moduleName, ScriptDefModule* parentModule)
	:_moduleName(moduleName),
	_propDict(PyDict_New()),
    _cellPropDict(PyDict_New()),
    _cellMethodDict(PyDict_New()),
    _baseMethodDict(PyDict_New()),
    _clientMethodDict(PyDict_New()),
    _remoteEntityMethod(nullptr),
    _hasCell(false),
    _hasBase(false),
    _hasClient(false)
{
	loadParentModule(parentModule);
}

ScriptDefModule::~ScriptDefModule(){
	Py_DECREF(_propDict);
    Py_DECREF(_cellMethodDict);
    Py_DECREF(_baseMethodDict);
    Py_DECREF(_clientMethodDict);
}

bool ScriptDefModule::initialize(){
	s_entityFlagMapping["CELL_PUBLIC"] = prop_def::EntityDataFlag::CELL_PUBLIC;
	s_entityFlagMapping["CELL_PRIVATE"] = prop_def::EntityDataFlag::CELL_PRIVATE;
	s_entityFlagMapping["ALL_CLIENTS"] = prop_def::EntityDataFlag::ALL_CLIENTS;
	s_entityFlagMapping["CELL_PUBLIC_AND_OWN"] = prop_def::EntityDataFlag::CELL_PUBLIC_AND_OWN;
	s_entityFlagMapping["BASE_AND_CLIENT"] = prop_def::EntityDataFlag::BASE_AND_CLIENT;
	s_entityFlagMapping["BASE"] = prop_def::EntityDataFlag::BASE;
	s_entityFlagMapping["OTHER_CLIENTS"] = prop_def::EntityDataFlag::OTHER_CLIENTS;
	s_entityFlagMapping["OWN_CLIENT"] = prop_def::EntityDataFlag::OWN_CLIENT;
	return true;
}

bool ScriptDefModule::loadParentModule(ScriptDefModule* parentModule){
	if (parentModule){
		const std::vector<PropDefInfo*>& parentPropsDefInfo = parentModule->propsDefInfo();
		
		for (auto defInfo : parentPropsDefInfo){
			_propsDefInfo.push_back(defInfo);
			appendObjectProp(defInfo);
		}

        const std::vector<PropDefInfo*>& parentMethodsDefInfo = parentModule->methodsDefInfo();
        for (auto defInfo : parentMethodsDefInfo){
            _methodsDefInfo.push_back(defInfo);
            appendObjectProp(defInfo, true);
        }

		if (parentModule->hasBase())
			_hasBase = true;

		if (parentModule->hasCell())
			_hasCell = true;

		if (parentModule->hasClient())
			_hasClient = true;
	}

	return true;
}

bool ScriptDefModule::loadFrom(const sl::ISLXmlNode& root){
	if (!loadAllDefDescriptions(_moduleName.c_str(), root)){
		SLASSERT(false, "load def description error");
		return false;
	}
	return true;
}


bool ScriptDefModule::loadAllDefDescriptions(const char* moduleName, const sl::ISLXmlNode& root){
	//������������
	if (!root.subNodeExist("Properties") || !loadDefPropertys(moduleName, root["Properties"][0]))
		return false;

	//����cell��������
	if (root.subNodeExist("CellMethods")){
		if (!loadDefCellMethods(moduleName, root["CellMethods"][0]))
			return false;
	}

	//����base��������
	if (root.subNodeExist("BaseMethods")){
		if (!loadDefBaseMethods(moduleName, root["BaseMethods"][0]))
			return false;
	}

	//����client��������
	if (root.subNodeExist("ClientMethods")){
		if (!loadDefClientMethods(moduleName, root["ClientMethods"][0]))
			return false;
	}

	return true;
}

bool ScriptDefModule::loadDefPropertys(const char* moduleName, const sl::ISLXmlNode& root){
	std::vector<sl::ISLXmlNode*> allPropsConf = root.getAllChilds();
	for (auto propConf : allPropsConf){
		const char* propName = propConf->value();
		if (!propConf->subNodeExist("Flags")){
			ECHO_ERROR(false, "prop %s has not Flags attribute", propName);
			return false;
		}
		std::string strFlags = (*propConf)["Flags"][0].text();
        sl::CStringUtils::MakeUpper(strFlags);
        auto flagItor = s_entityFlagMapping.find(strFlags);
		if (flagItor == s_entityFlagMapping.end()){
			SLASSERT(false, "load def property: not find flags %s of prop:%s on module:%s", strFlags.c_str(), propName, moduleName);
			return false;
		}

		uint32 flags = flagItor->second;
		uint32 hasBaseFlags = flags & ENTITY_BASE_DATA_FLAGS;
		if (hasBaseFlags > 0)
			_hasBase = true;

		uint32 hasCellFlags = flags & ENTITY_CELL_DATA_FLAGS;
		if (hasCellFlags > 0)
			_hasCell = true;

		uint32 hasClientFlags = flags & ENTITY_CLIENT_DATA_FLAGS;
		if (hasClientFlags > 0)
			_hasClient = true;

		if (hasBaseFlags <= 0 && hasCellFlags <= 0){
			ECHO_ERROR(false, "load Def property error: flags[%s] of prop[%s] on module[%s] not both on cell and base", strFlags.c_str(), propName, moduleName);
			return false;
		}

		if (propConf->subNodeExist("Persistent")){
			std::string strPersistent = (*propConf)["Persistent"][0].text();
            sl::CStringUtils::MakeLower(strPersistent);
            if (strPersistent == "true")
				flags |= prop_def::persistent;
		}

		if (!propConf->subNodeExist("Type")){
			ECHO_ERROR(false, "loadDefProperty error: not found Type of prop[%s] on module[%s]", propName, moduleName);
			return false;
		}
		const char* strType = (*propConf)["Type"][0].text();
		DATATYPE_UID dateType = DataTypeMgr::getDataTypeUid(strType);
		
        if (propConf->subNodeExist("Index")){
			std::string strIndex = (*propConf)["Index"][0].text();
            sl::CStringUtils::MakeLower(strIndex);
            if (strIndex == "true")
				flags |= prop_def::index;
		}

		if (propConf->subNodeExist("Identifier")){
			std::string strIdentifier = (*propConf)["Identifier"][0].text();
            sl::CStringUtils::MakeLower(strIdentifier);
            if (strIdentifier == "true")
				flags |= prop_def::identifier;
		}

	    int32 nodeType = SLMODULE(Harbor)->getNodeType();
        if(nodeType == NodeType::SCENE && !hasCellFlags)
            continue;

		PropDefInfo* info = NEW PropDefInfo();
		info->_name = propName;
		info->_size = sizeof(uint64);
		info->_index = 0;
		info->_flags = flags;
		info->_type = dateType;
        info->_extra = 0;
		_propsDefInfo.push_back(info);
		appendObjectProp(info);
	}

	return true;
}

bool ScriptDefModule::loadDefCellMethods(const char* moduleName, const sl::ISLXmlNode& root){
	return loadDefMethods(moduleName, RemoteMethodType::RMT_CELL, root);
}

bool ScriptDefModule::loadDefBaseMethods(const char* moduleName, const sl::ISLXmlNode& root){
	return loadDefMethods(moduleName, RemoteMethodType::RMT_BASE, root);
}

bool ScriptDefModule::loadDefClientMethods(const char* moduleName, const sl::ISLXmlNode& root){
	return loadDefMethods(moduleName, RemoteMethodType::RMT_CLIENT, root);
}

bool ScriptDefModule::loadDefMethods(const char* moduleName, const int8 type, const sl::ISLXmlNode& root){
	std::vector<sl::ISLXmlNode*> allMethodsConf = root.getAllChilds();
	for (auto methodConf : allMethodsConf){
		const char* methodName = methodConf->value();
		bool isExposed = false;
		if (methodConf->subNodeExist("Exposed"))
			isExposed = true;

		std::vector<uint8> dataTypes;
		if (methodConf->subNodeExist("Arg")){
			const sl::ISLXmlNode& args = (*methodConf)["Arg"];
			for (int32 i = 0; i < args.count(); i++){
				const char* strType = args[i].text();
				DATATYPE_UID dateType = DataTypeMgr::getDataTypeUid(strType);
				dataTypes.push_back(dateType);
            }
        }            
	
        PropDefInfo* info = NEW PropDefInfo();
        info->_name = methodName;

        //��������������(uint64)
        info->_size = 0;
        info->_index = 0;
        info->_flags = 0;
        info->_type = type;
        info->_extra = 0;
        _methodsDefInfo.push_back(info);

        appendObjectProp(info, true, true);    
	}

	return true;
}

bool ScriptDefModule::appendObjectProp(PropDefInfo* defInfo, bool isMethod, bool isTemp){
	const IProp* prop = isTemp ? SLMODULE(ObjectMgr)->appendObjectTempProp(_moduleName.c_str(), defInfo->_name.c_str(), defInfo->_type, defInfo->_size, defInfo->_flags, defInfo->_index, defInfo->_extra) : 
        SLMODULE(ObjectMgr)->appendObjectProp(_moduleName.c_str(), defInfo->_name.c_str(), defInfo->_type, defInfo->_size, defInfo->_flags, defInfo->_index, defInfo->_extra);
	if (!prop){
		SLASSERT(false, "wtf");
		return false;
	}

	PyObject* pykey = PyUnicode_FromString(defInfo->_name.c_str());
	PyObject* pyPropPtr = PyLong_FromVoidPtr(const_cast<IProp*>(prop));
	if (PyErr_Occurred()){
		PyErr_Clear();
		PyErr_Format(PyExc_TypeError, "createObject failed");
		PyErr_PrintEx(0);

		S_RELEASE(pyPropPtr);
		S_RELEASE(pykey);
		return false;
	}

    PyObject* dict = _propDict;
    if(isMethod){
        switch(defInfo->_type){
        case RemoteMethodType::RMT_CLIENT: dict = _clientMethodDict; break;
        case RemoteMethodType::RMT_CELL: dict = _cellMethodDict; break;
        case RemoteMethodType::RMT_BASE: dict = _baseMethodDict; break;
        default: break;
        }
    }
	PyDict_SetItem(dict, pykey, pyPropPtr);
	Py_DECREF(pykey);
	Py_DECREF(pyPropPtr);

    //��base�celldata
	uint32 hasCellFlags = defInfo->_flags & ENTITY_CELL_DATA_FLAGS;
    if(SLMODULE(Harbor)->getNodeType() == NodeType::LOGIC && hasCellFlags && !isMethod){
        PyDict_SetItem(_cellPropDict, pykey, pyPropPtr);
        Py_DECREF(pykey);
        Py_DECREF(pyPropPtr);
    }
	
    return true;
}

PyObject* ScriptDefModule::scriptGetObjectAttribute(PyObject* object, PyObject* attr){
	ScriptObject* scriptObject = static_cast<ScriptObject*>(object);
    const IProp* prop = getProp(attr);
	if (!prop){
		return nullptr;
	}

	return DataTypeMgr::getPyAttrValue(scriptObject->getInnerObject(), prop);
}

int32 ScriptDefModule::scriptSetObjectAttribute(PyObject* object, PyObject* attr, PyObject* value){
	ScriptObject* scriptObject = static_cast<ScriptObject*>(object);
    const IProp* prop = getProp(attr);
	if (!prop){
		return -1;
	}
	return DataTypeMgr::setPyAttrValue(scriptObject->getInnerObject(), prop, value);
}

const IProp* ScriptDefModule::getProp(PyObject* attr){
	PyObject* pyVal = PyDict_GetItem(_propDict, attr);
	if (!pyVal){
		return nullptr;
	}
	return (const IProp*)PyLong_AsVoidPtr(pyVal);
}

const IProp* ScriptDefModule::getMethodProp(const int8 type, PyObject* attr){
    PyObject* methodDict = nullptr;
    switch(type){
    case EntityMailBoxType::MAILBOX_TYPE_BASE: methodDict = _baseMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_CELL: methodDict = _cellMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_CLIENT: methodDict = _clientMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_CELL_VIA_BASE: methodDict = _cellMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_BASE_VIA_CELL: methodDict = _baseMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_CLIENT_VIA_BASE: methodDict = _clientMethodDict; break;
    case EntityMailBoxType::MAILBOX_TYPE_CLIENT_VIA_CELL: methodDict = _clientMethodDict; break;
    default: break;
    } 
    
    PyObject* pyVal = PyDict_GetItem(methodDict, attr);
    if(!pyVal)
        return nullptr;
    return (const IProp*)PyLong_AsVoidPtr(pyVal);
}

bool ScriptDefModule::checkMethodArgs(IObject* object, const IProp* methodProp, PyObject* args){
    if(!args || !PyTuple_Check(args)){
        PyErr_Format(PyExc_AssertionError, "Method::checkArgs: method[%d] args is not a tuple.\n", methodProp->getName());
        PyErr_PrintEx(0);
        return false;
    }

    int64 argsType = methodProp->getExtra(object);
    int8* argType = (int8*)argsType;

    int32 pyArgsSize = PyTuple_Size(args);
    int32 i = 0;
    for(;i < pyArgsSize; ++i){
        PyObject* pyArg = PyTuple_GetItem(args, i);
        if(!DataTypeMgr::isDataType(pyArg, argType[i]))
            return false;
    }

    if(i >= (int32)(sizeof(int64)/sizeof(int8)) || argType[i] != 0xFF)
        return false;

    return true;
}

PyObject* ScriptDefModule::createPyObject(const uint64 entityId){
    PyObject* obj = PyType_GenericAlloc(getScriptType(), 0);
	if (obj == NULL){
        PyErr_Format(PyExc_AssertionError, "ScriptDefModule::createPyObject: GenericAlloc is failed.\n");
		PyErr_PrintEx(0);
        return NULL;
	}

    if(entityId == 0){
		PyErr_Format(PyExc_AssertionError,"ScriptDefModule::createPyObject: entityId is 0.\n");
        PyErr_PrintEx(0);
        return NULL;
    }
    
    
    SCRIPT_ERROR_CHECK();

    return obj;
}

void ScriptDefModule::initializeEntity(PyObject* object, PyObject* dictData){
    createNameSpace(object, dictData);
    initializeScript(object);
}

void ScriptDefModule::createNameSpace(PyObject* object, PyObject* dictData){
    if(dictData == NULL)
        return;

    if(!PyDict_Check(dictData)){
        PyErr_Format(PyExc_AssertionError, "createNameSpace: create failed, args is not dict\n");
        PyErr_PrintEx(0);
        return;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    PyObject * cellDataDict = PyObject_GetAttrString(object, "cellData");
    if(cellDataDict == NULL)
        PyErr_Clear();

    while(PyDict_Next(dictData, &pos, &key, &value)){
        PyObject* val = scriptGetObjectAttribute(object, key);
        if(val != nullptr){
            scriptSetObjectAttribute(object, key, value);
            continue;
        }

        if(cellDataDict != NULL && PyDict_Contains(cellDataDict, key) > 0)
            PyDict_SetItem(cellDataDict, key, value);
        else
            PyObject_SetAttr(object, key, value);
    }

    SCRIPT_ERROR_CHECK();
    Py_XDECREF(cellDataDict);
}

void ScriptDefModule::initializeScript(PyObject* object){
	if (PyObject_HasAttrString(object, "__init__")){																									
		PyObject* pyResult = PyObject_CallMethod(object, const_cast<char*>("__init__"), 
		const_cast<char*>(""));											
		if (pyResult != NULL)																			
			Py_DECREF(pyResult);																		
		else																							
			SCRIPT_ERROR_CHECK();																		
	}																									
}

void ScriptDefModule::setDefaultCellData(PyObject* dataDict){
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while(PyDict_Next(_cellPropDict, &pos, &key, &value)){
	    if (!value){
            PyErr_Format(PyExc_AssertionError, "SetDefaultCellData:getCellPropData Prop is null!\n");
            PyErr_PrintEx(0);
		    return;
	    }
	    const IProp* cellProp = (const IProp*)PyLong_AsVoidPtr(value);
        PyObject* pyValue = PyLong_FromLong(0);
        PyDict_SetItem(dataDict, key, pyValue);
    }

    SCRIPT_ERROR_CHECK();
}

void ScriptDefModule::addCellDataToStream(PyObject* object, PyObject* cellDataDict, sl::IBMap& dataStream){
	ScriptObject* scriptObject = static_cast<ScriptObject*>(object);
    IObject* innerObject = scriptObject->getInnerObject();

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while(PyDict_Next(_cellPropDict, &pos, &key, &value)){
	    if (!value){
            PyErr_Format(PyExc_AssertionError, "SetDefaultCellData:getCellPropData Prop is null!\n");
            PyErr_PrintEx(0);
		    return;
	    }
		
        wchar_t*wideCharString = PyUnicode_AsWideCharString(key, NULL);
		char* valStr = sl::CStringUtils::wchar2char(wideCharString);
		PyMem_Free(wideCharString);
        printf("current type name:%s\n", valStr);
	    
        const IProp* cellProp = (const IProp*)PyLong_AsVoidPtr(value);
        
        PyObject* pyValue = NULL;
        if(PyDict_Contains(cellDataDict, key) > 0)
            pyValue = PyDict_GetItem(cellDataDict, key);

        pyValue = PyLong_FromLong(13);

        DataTypeMgr::addDataToStream(innerObject, cellProp, dataStream, pyValue);
    }
}

bool ScriptDefModule::createCellDataFromStream(PyObject* object, const void* cellData, const int32 cellDataSize){
	ScriptObject* scriptObject = static_cast<ScriptObject*>(object);
    IObject* entity = scriptObject->getInnerObject();
    
    sl::OBMap cellDataStream(cellData, cellDataSize);
    printf("cell entity %s has attr %d, datasize:%d\n", entity->getObjTypeString(), entity->getObjProps().size(), cellDataSize);
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while(PyDict_Next(_propDict, &pos, &key, &value)){
	    if (!value){
            PyErr_Format(PyExc_AssertionError, "createCellDataFromStream:getPropData Prop is null!\n");
            PyErr_PrintEx(0);
		    return false;
	    }
	    const IProp* prop = (const IProp*)PyLong_AsVoidPtr(value);
        DataTypeMgr::readDataFromStream(entity, prop, cellDataStream);
    }
    return true;
}
