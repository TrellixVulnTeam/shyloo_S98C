#ifndef __SL_CORE_MYSQL_BASE_H__
#define __SL_CORE_MYSQL_BASE_H__
#include "IMysqlMgr.h"
#include "sldb.h"
using namespace sl::db;
class MysqlResult : public IMysqlResult{
public:
	struct TABLE_COLUMN{
		int32  index;
		uint32 length;
		uint32 maxLength;
		uint32 flags;
		int32  type;
	};

	MysqlResult()
		:_errCode(0),
		_affectedRows(0),
		_insertId(0),
		_optType(DB_OPT_NONE)
	{}
	virtual ~MysqlResult() {}

	virtual int32 rowCount() const { return (int32)_result.size(); }
	virtual int32 columnCount() const { return (int32)_columns.size(); }
	virtual bool columnExist(const char* column) const { return _columns.find(column) != _columns.end(); }
	virtual const std::vector<std::string>& columns() const {return _columnsVec;}

	virtual int8 getDataInt8(const int32 i, const char* key) const { return sl::CStringUtils::StringAsInt8(getColData(i, key).c_str()); }
	virtual int16 getDataInt16(const int32 i, const char* key) const { return sl::CStringUtils::StringAsInt16(getColData(i, key).c_str()); }
	virtual int32 getDataInt32(const int32 i, const char* key) const { return sl::CStringUtils::StringAsInt32(getColData(i, key).c_str()); }
	virtual int64 getDataInt64(const int32 i, const char* key) const { return sl::CStringUtils::StringAsInt64(getColData(i, key).c_str()); }
	virtual float getDataFloat(const int32 i, const char* key) const { return sl::CStringUtils::StringAsFloat(getColData(i, key).c_str()); }
	virtual double getDataDouble(const int32 i, const char* key) const { return sl::CStringUtils::StringAsDouble(getColData(i, key).c_str()); }
	virtual const char* getDataString(const int32 i, const char* key) const { return getColData(i, key).c_str(); }
	virtual const void* getDataBlob(const int32 i, const char* key, int32& dataSize) const { const std::string& data = getColData(i, key); dataSize = data.size(); return data.c_str();}
	virtual int32 errCode() {return _errCode;}
	virtual const char* errInfo() {return _errInfo.c_str();}
	virtual const uint64 affectedRows() const {return _affectedRows;}
	virtual const uint64 insertId() const {return _insertId;}
	virtual const int32 optType() const {return _optType;}
	virtual void release() {DEL this;}

	void setErrCode(int32 code) {_errCode = code;}
	void setErrInfo(const char* info) {_errInfo = info;}
	void setAffectedRows(const uint64 rows) {_affectedRows = rows;}
	void setInsertId(const uint64 insertId) {_insertId = insertId;}
	void setOptType(const int32 opt) {_optType = opt;}
	
	virtual uint32 columnLength(const char* column) const {
		auto itor = _columns.find(column);
		if(itor != _columns.end())
			return itor->second.length;
		return 0;
	}

	virtual uint32 columnMaxLength(const char* column) const {
		auto itor = _columns.find(column);
		if(itor != _columns.end())
			return itor->second.maxLength;
		return 0;
	}

	virtual uint32 columnFlags(const char* column) const {
		auto itor = _columns.find(column);
		if(itor != _columns.end())
			return itor->second.flags;
		return 0;
	}

	virtual int32 columnType(const char* column) const {
		auto itor = _columns.find(column);
		if(itor != _columns.end())
			return itor->second.type;
		return 0;
	}

	void setColData(ISLDBResult* dbResult){
		int32 fieldNum = (int32)dbResult->fieldNum();
		std::vector<string> rowResult;
		for (int32 i = 0; i < fieldNum; i++){
			rowResult.push_back(string(dbResult->fieldValue(i), dbResult->fieldLength(i) + 1));
		}
		_result.push_back(std::move(rowResult));
	}

	void setColumns(ISLDBResult* dbResult){
		if (!_columns.empty())
			return;

		int32 fieldNum = (int32)dbResult->fieldNum();
		for (int32 i = 0; i < fieldNum; i++){
			const char* fieldName = dbResult->fieldName(i);
			TABLE_COLUMN& info = _columns[fieldName];
			info.index = i;
			info.length = dbResult->fieldDBLength(i);
			info.maxLength = dbResult->fieldDBMaxLength(i);
			info.flags = dbResult->fieldFlags(i);
			info.type = dbResult->fieldType(i);
			_columnsVec.push_back(fieldName);
		}
	}

private:
	inline int32 columnIdx(const char* column) const{
		auto itor = _columns.find(column);
		SLASSERT(itor != _columns.end(), "cannot find column %s", column);
		return itor->second.index;
	}

	const std::string& getColData(int32 row, const char* column) const{
		SLASSERT(row >= 0 && row < (int32)_result.size(), "out of range");
		const int32 idx = columnIdx(column);
		return _result[row][idx];
	}

private:
	std::unordered_map<string, TABLE_COLUMN>	_columns;
	std::vector<std::string>					_columnsVec;
	std::vector<std::vector<string>>			_result;
	int32										_errCode;
	std::string									_errInfo;
	uint64										_affectedRows;
	uint64										_insertId;
	int32										_optType;
};

class MysqlBase : public sl::api::IAsyncHandler{
public:
	MysqlBase(ISLDBConnection* dbConn, SQLCommand* sqlCommand, const SQLExecCallback& cb);
	~MysqlBase();

	virtual bool onExecute(sl::api::IKernel* pKernel);
	virtual bool onSuccess(sl::api::IKernel* pKernel);
	virtual bool onFailed(sl::api::IKernel* pKernel, bool nonviolent);
	virtual void onRelease(sl::api::IKernel* pKernel);

	static int32 realExecSql(SQLCommand* sqlCommand, ISLDBConnection* dbConnection, MysqlResult* mysqlResult);
	static int32 getTableFields(ISLDBConnection* dbConnection, const char* tableName, MysqlResult* mysqlResult);
private:
	ISLDBConnection*	_dbConnection;
	SQLExecCallback		_callback;
	SQLCommand*			_sqlCommand;
	MysqlResult			_result;
};
#endif
