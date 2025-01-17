#include "CacheDB.h"
#include "slxml_reader.h"
#include "slargs.h"
#include "CacheDBStruct.h"
#include "IRedis.h"
#include "DataLand.h"
#include "SQLBase.h"

bool CacheDB::initialize(sl::api::IKernel * pKernel){
	_kernel = pKernel;

	/*char path[256] = { 0 };
	SafeSprintf(path, sizeof(path), "%s/cache_db.xml", pKernel->getEnvirPath());
	sl::XmlReader conf;
	if (!conf.loadXml(path)){
		SLASSERT(false, "can not load file %s", path);
		return false;
	}

	const sl::ISLXmlNode& tables = conf.root()["table"];
	for (int32 i = 0; i < tables.count(); i++){
		CacheTable tableDesc;
		tableDesc.name = tables[i].getAttributeString("name");
		tableDesc.del = tables[i].getAttributeBoolean("del");
		const sl::ISLXmlNode& columns = tables[i]["column"];
		for (int32 j = 0; j < columns.count(); j++){
			const char* name = columns[j].getAttributeString("name");
			const char* typeStr = columns[j].getAttributeString("type");

			if (!strcmp(typeStr, "int8")){
				tableDesc.columns[name] = CDB_TYPE_INT8;
			}
			else if (!strcmp(typeStr, "int16")){
				tableDesc.columns[name] = CDB_TYPE_INT16;
			}
			else if (!strcmp(typeStr, "int32")){
				tableDesc.columns[name] = CDB_TYPE_INT32;
			}
			else if (!strcmp(typeStr, "int64")){
				tableDesc.columns[name] = CDB_TYPE_INT64;
			}
			else if (!strcmp(typeStr, "float")){
				tableDesc.columns[name] = CDB_TYPE_FLOAT;
			}
			else if (!strcmp(typeStr, "string")){
				tableDesc.columns[name] = CDB_TYPE_STRING;
			}
			else if (!strcmp(typeStr, "blob")){
				tableDesc.columns[name] = CDB_TYPE_BLOB;
			}
			else{
				SLASSERT(false, "invalid type");
				return false;
			}

			tableDesc.columnsVec.push_back(name);

			if (columns[j].hasAttribute("key") && columns[j].getAttributeBoolean("key")){
				if (tableDesc.key == "")
					tableDesc.key = name;
				else{
					SLASSERT(false, "wtf");
					return false;
				}
			}
		}
		if (tableDesc.key == ""){
			SLASSERT(false, "etyf");
			return false;
		}
		if (tables[i].subNodeExist("index")){
			const char* index = tables[i]["index"][0].getAttributeString("name");
			auto itor = tableDesc.columns.find(index);
			if (itor != tableDesc.columns.end()){
				tableDesc.index = { itor->second, itor->first };
			}
			else{
				SLASSERT(false, "eyf");
				return false;
			}
		}
		_tables[tableDesc.name] = tableDesc;
	}
*/

	return true;
}
bool CacheDB::launched(sl::api::IKernel * pKernel){
	FIND_MODULE(_redis, Redis);
	//test();
	return true;
}
bool CacheDB::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

int8 CacheDB::getColumnType(const char* table, const char* column){
	auto tableItor = _tables.find(table);
	if (tableItor == _tables.end())
		return CDB_TYPE_NONE;
	
	CacheTable& desc = tableItor->second;
	auto descItor = desc.columns.find(column);
	if (descItor == desc.columns.end())
		return CDB_TYPE_NONE;

	return descItor->second;
}

int32 CacheDB::getColumnIdx(const char* table, const char* column){
	auto tableItor = _tables.find(table);
	if (tableItor == _tables.end())
		return -1;

	CacheTable& desc = tableItor->second;
	int32 size = (int32)desc.columnsVec.size();
	for (int32 i = 0; i < size; i++){
		if (desc.columnsVec[i] == column)
			return i;
	}
	return -1;
}

const char* CacheDB::getColumnByIdx(const char* table, const int32 idx){
	auto tableItor = _tables.find(table);
	if (tableItor == _tables.end())
		return nullptr;

	CacheTable& desc = tableItor->second;
	if (idx >= (int32)desc.columnsVec.size())
		return nullptr;

	return desc.columnsVec[idx].c_str();
}


bool CacheDB::read(const char* table, const CacheDBColumnFuncType& cf, const CacheDBReadFuncType& f, int32 count, ...){
	if (count > 0 && _tables.find(table) != _tables.end()){
		CacheTable& desc = _tables[table];
		IArgs<MAX_KEYS, MAX_ARGS> args;

		CacheDBReader reader(desc, args);
		cf(_kernel, &reader);

		char keyName[128] = { 0 };
		SafeSprintf(keyName, 128, "%s|", table);
		args << keyName << count;

		va_list ap;
		va_start(ap, count);
		int8 type = desc.columns[desc.key];
		for (int32 i = 0; i < count; ++i){
			switch (type){
			case CDB_TYPE_INT8: args << (int8)va_arg(ap, int8); break;
			case CDB_TYPE_INT16: args << (int16)va_arg(ap, int16); break;
			case CDB_TYPE_INT32: args << (int32)va_arg(ap, int32); break;
			case CDB_TYPE_INT64: args << (int64)va_arg(ap, int64); break;
			case CDB_TYPE_STRING: args << (const char*)va_arg(ap, const char*); break;
			default: SLASSERT(false, "wtf");
			}
		}
		va_end(ap);

		SLASSERT(reader.count() > 0, "wtf");
		args.fix();
		return _redis->call(0, "db_get", reader.count(), args.out(), [this, &f](const sl::api::IKernel* pKernel, const sl::db::ISLRedisResult* rst){
			CacheDBReadResult result(rst);
			f(_kernel, &result);
			return true;
		});
	}
	else{
		SLASSERT(false, "wtf");
	}
	return false;
}

bool CacheDB::readByIndex(const char* table, const CacheDBColumnFuncType& cf, const CacheDBReadFuncType& f, const int64 index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "wtf");
		return false;
	}

	CacheTable& desc = _tables[table];
	if (desc.index.type < CDB_TYPE_CANT_BE_KEY && desc.index.type != CDB_TYPE_STRING && desc.index.type != CDB_TYPE_NONE){
		IArgs<MAX_KEYS, MAX_ARGS> args;
		CacheDBReader reader(desc, args);
		cf(_kernel, &reader);
		SLASSERT(reader.count() > 0, "wtf");

		char tmp[128];
		SafeSprintf(tmp, sizeof(tmp), "%s|i+%lld", table, index);
		args << tmp;
		args.fix();

		return _redis->call(0, "db_get_index", reader.count(), args.out(), [this, &f](const sl::api::IKernel* pKernel, const sl::db::ISLRedisResult* rst){
			CacheDBReadResult result(rst);
			f(_kernel, &result);
			return true;
		});
	}
	else{
		SLASSERT(false, "wrtf");
	}
	return false;
}

bool CacheDB::readByIndex(const char* table, const CacheDBColumnFuncType& cf, const CacheDBReadFuncType& f, const char* index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "wtf");
		return false;
	}

	CacheTable& desc = _tables[table];
	if (desc.index.type != CDB_TYPE_STRING){
		SLASSERT(false, "wtf");
		return false;
	}

	IArgs<MAX_KEYS, MAX_ARGS> args;
	CacheDBReader reader(desc, args);
	cf(_kernel, &reader);
	SLASSERT(reader.count() > 0, "wtf");

	char tmp[128];
	SafeSprintf(tmp, sizeof(tmp), "%s|i+%s", table, index);
	args << tmp;
	args.fix();

	return _redis->call(0, "db_get_index", reader.count(), args.out(), [this, &f](const sl::api::IKernel* pKernel, const sl::db::ISLRedisResult* rst){
		CacheDBReadResult result(rst);
		f(_kernel, &result);
		return true;
	});
}

bool CacheDB::write(const char* table, bool sync, const CacheDBWriteFuncType& f, int32 count, ...){
	if (count <= 0 || _tables.find(table) == _tables.end()){
		SLASSERT(false, "not find table %s", table);
		return false;
	}

	CacheTable& desc = _tables[table];
	IArgs<MAX_KEYS, MAX_ARGS> args;
	IArgs<MAX_KEYS, MAX_ARGS> landData;

	CacheDBContext context(desc, args);
	f(_kernel, &context);

	va_list ap;
	va_start(ap, count);
	int8 type = desc.columns[desc.key];
	char temp[128];
	SafeSprintf(temp, 128, "%s|", table);
	args << temp << desc.key.c_str() << count;
	for (int32 i = 0; i < count; i++){
		switch (type){
		case CDB_TYPE_INT8: { int8 val = (int8)va_arg(ap, int8); args << val; landData << val; break; }
		case CDB_TYPE_INT16: { int16 val = (int16)va_arg(ap, int16); args << val; landData << val; break; }
		case CDB_TYPE_INT32: { int32 val = (int32)va_arg(ap, int32); args << val; landData << val; break; }
		case CDB_TYPE_INT64: { int64 val = (int64)va_arg(ap, int64); args << val; landData << val; break; }
		case CDB_TYPE_STRING: { const char* val = (const char*)va_arg(ap, const char*); args << val; landData << val; break; }
		default: SLASSERT(false, "wtf");
		}
	}
	va_end(ap);

	if (context.isChangedIndex()){
		char tmp[128];
		SafeSprintf(tmp, sizeof(tmp), "%s|i+", table);
		args << tmp << desc.index.name.c_str();
	}

	args.fix();
	const OArgs& out = args.out();
	bool ret = _redis->call(0, "db_set", context.count() * 2, out);

	if (ret){
		for (int32 i = 0; i < context.count() * 2; i += 2){
			landData << out.getString(i);
		}
		landData.fix();
		DataLand::getInstance()->askLand(table, count, landData.out(), desc.key.c_str(), DB_OPT::DB_OPT_SAVE, sync);
	}
	return ret;
}

bool CacheDB::writeByIndex(const char* table, bool sync, const CacheDBWriteFuncType& f, const int64 index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "not find table %s", table);
		return false;
	}
	
	CacheTable& desc = _tables[table];
	if (desc.index.type >= CDB_TYPE_CANT_BE_KEY || desc.index.type == CDB_TYPE_STRING || desc.index.type == CDB_TYPE_NONE){
		SLASSERT(false, "invaild Key type");
		return false;
	}

	IArgs<MAX_KEYS, MAX_ARGS> args;
	CacheDBContext context(desc, args);
	f(_kernel, &context);

	char temp[128];
	SafeSprintf(temp, 128, "%s|i+%lld", table, index);
	args << temp;
	args.fix();
	const OArgs& out = args.out();
	bool ret = _redis->call(0, "db_set_index", context.count() * 2, out);

	if (ret){
		IArgs<MAX_KEYS, MAX_ARGS> landData;
		landData << index;
		for (int32 i = 0; i < context.count() * 2; i += 2){
			landData << out.getString(i);
		}
		landData.fix();
		DataLand::getInstance()->askLand(table, 1, landData.out(), desc.index.name.c_str(), DB_OPT::DB_OPT_UPDATE, sync);
	}
	return ret;
}

bool CacheDB::writeByIndex(const char* table, bool sync, const CacheDBWriteFuncType& f, const char* index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "wtf");
		return false;
	}

	CacheTable& desc = _tables[table];
	if (desc.index.type != CDB_TYPE_STRING){
		SLASSERT(false, "wtf");
		return false;
	}

	IArgs<MAX_KEYS, MAX_ARGS> args;

	CacheDBContext context(desc, args);
	f(_kernel, &context);

	char tmp[128];
	SafeSprintf(tmp, sizeof(tmp), "%s|i+%s", table, index);
	args << tmp;
	args.fix();
	const OArgs& out = args.out();
	bool ret = _redis->call(0, "db_set_index", context.count() * 2, out);
	
	if (ret){
		IArgs<MAX_KEYS, MAX_ARGS> landData;
		landData << index;
		for (int32 i = 0; i < context.count() * 2; i += 2){
			landData << out.getString(i);
		}
		landData.fix();
		DataLand::getInstance()->askLand(table, 1, landData.out(), desc.index.name.c_str(), DB_OPT::DB_OPT_UPDATE, sync);
	}

	return ret;
}

bool CacheDB::del(const char* table, bool sync, int32 count, ...){
	if (count <= 0 || _tables.find(table) == _tables.end()){
		SLASSERT(false, "not find table %s", table);
		return false;
	}

	CacheTable& desc = _tables[table];
	IArgs<MAX_KEYS, MAX_ARGS> args;
	args << (desc.del ? 1 : 0);

	char keyName[128];
	SafeSprintf(keyName, 128, "%s|", table);
	args << keyName << count;

	IArgs<MAX_KEYS, MAX_ARGS> landData;

	va_list ap;
	va_start(ap, count);
	int8 type = desc.columns[desc.key];
	for (int32 i = 0; i < count; i++){
		switch (type){
		case CDB_TYPE_INT8: { int8 val = (int8)va_arg(ap, int8); args << val; landData << val; break; }
		case CDB_TYPE_INT16: { int16 val = (int16)va_arg(ap, int16); args << val; landData << val; break; }
		case CDB_TYPE_INT32: { int32 val = (int32)va_arg(ap, int32); args << val; landData << val; break; }
		case CDB_TYPE_INT64: { int64 val = (int64)va_arg(ap, int64); args << val; landData << val; break; }
		case CDB_TYPE_STRING: { const char* val = (const char*)va_arg(ap, const char*); args << val; landData << val; break; }
		default: SLASSERT(false, "wtf");
		}
	}
	va_end(ap);

	args.fix();
	bool ret = _redis->call(0, "db_del", 1, args.out());

	if (ret){
		landData.fix();
		DataLand::getInstance()->askLand(table, count, landData.out(), desc.key.c_str(), DB_OPT::DB_OPT_DELETE, sync);
	}
	return ret;
}

bool CacheDB::delByIndex(const char* table, bool sync, const int64 index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "not find table %s", table);
		return false;
	}

	CacheTable& desc = _tables[table];
	if (desc.index.type >= CDB_TYPE_CANT_BE_KEY || desc.index.type == CDB_TYPE_STRING || desc.index.type == CDB_TYPE_NONE){
		SLASSERT(false, "invaild key type");
		return false;
	}

	char indesStr[128];
	SafeSprintf(indesStr, sizeof(indesStr), "%s|i+%lld", table, index);

	IArgs<MAX_KEYS, MAX_ARGS> args;
	args << indesStr << (desc.del ? 1 : 0);
	args.fix();

	bool ret = _redis->call(0, "db_del_index", 0, args.out());
	if (ret){
		IArgs<1, 64> landData;
		landData << index;
		landData.fix();
		DataLand::getInstance()->askLand(table, 1, landData.out(), desc.index.name.c_str(), DB_OPT::DB_OPT_DELETE, sync);
	}
	return ret;
}

bool CacheDB::delByIndex(const char* table, bool sync, const char* index){
	if (_tables.find(table) == _tables.end()){
		SLASSERT(false, "not find table %s", table);
		return false;
	}

	CacheTable& desc = _tables[table];
	if (desc.index.type != CDB_TYPE_STRING){
		SLASSERT(false, "wtf");
		return false;
	}

	char indesStr[128];
	SafeSprintf(indesStr, sizeof(indesStr), "%s|i+%s", table, index);

	IArgs<MAX_KEYS, MAX_ARGS> args;
	args << indesStr << (desc.del ? 1 : 0);
	args.fix();

	bool ret = _redis->call(0, "db_del_index", 0, args.out());
	if (ret){
		IArgs<1, 128> landData;
		landData << index;
		landData.fix();
		DataLand::getInstance()->askLand(table, 1, landData.out(), desc.index.name.c_str(), DB_OPT::DB_OPT_DELETE, sync);
	}
	return ret;
}

void CacheDB::test(){
	/*write("actor", [&](sl::api::IKernel* pKernel, ICacheDBContext* context){
		context->writeInt64("account", 1454565);
		context->writeString("name", "fyyyy");
		}, 1, (int64)2345);*/

	/*writeByIndex("actor", [&](sl::api::IKernel* pKernel, ICacheDBContext* context){
		context->writeString("name", "ddc");
		}, (int64)1454565);*/

	/*auto& cf = [&](sl::api::IKernel* pKernel, ICacheDBReader* reader){
		reader->readColumn("name");
		reader->readColumn("account");
		};

		auto& rf = [&](sl::api::IKernel* pKernel, ICacheDBReadResult* result){
		int32 rowCount = result->count();
		for (int32 i = 0; i < rowCount; i++){
		const char* name = result->getString(i, 0);
		int32 account = result->getInt64(i, 1);
		int32 s = 0;
		}
		};*/
	
	//readByIndex("actor", cf, rf, 1454565);
	//delByIndex("actor", 1454565);
}
