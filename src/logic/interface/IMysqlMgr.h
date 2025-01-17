#ifndef __SL_INTERFACE_MYSQL_MGR_H__
#define __SL_INTERFACE_MYSQL_MGR_H__
#include "slimodule.h"
#include <unordered_map>
#include <vector>
#include "slikernel.h"
#include "SQLBase.h"

class ISQLBuilder{
public:
	virtual ~ISQLBuilder() {}
	virtual ISQLBuilder* table(const char* tableName) = 0;
	virtual ISQLBuilder* select(const char* field) = 0;
	virtual ISQLBuilder* where(const Expr& expr) = 0;
	virtual ISQLBuilder* orWhere(const Expr& expr) = 0;
	virtual ISQLBuilder* insert(const SetExpr* val) = 0;
	virtual ISQLBuilder* get(const int32 limit) = 0;
	virtual ISQLBuilder* update(const SetExpr* val) = 0;
	virtual ISQLBuilder* save(const SetExpr* val) = 0;
	virtual ISQLBuilder* del() = 0;

	virtual int32 optType() const = 0;
	virtual const char* toString() = 0;
	virtual bool checkVaild() = 0;
	virtual bool submit() = 0;
};

class SQLCommand{
public:
	SQLCommand(ISQLBuilder* sqlBuilder) :_sqlBuilder(sqlBuilder){}
	~SQLCommand(){
		if (_sqlBuilder)
			DEL _sqlBuilder;

		_sqlBuilder = nullptr; 
	}

	int32 optType() const { return _sqlBuilder->optType(); }
	const char* toString(){ return _sqlBuilder->toString(); }
	bool checkVaild(){ return _sqlBuilder->checkVaild(); }
	bool submit(){ return _sqlBuilder->submit(); }

	SQLCommand& table(const char* tableName){ _sqlBuilder->table(tableName); return *this;}
	SQLCommand& get(const int32 limit = 0){ _sqlBuilder->get(limit); return *this;}
	SQLCommand& del(){ _sqlBuilder->del(); return *this;}

	template<typename... Args>
	SQLCommand& select(const char* field, Args... args){
		return selectInner(field, args...);
	}
	template<typename... Args>
	SQLCommand& selectInner(const char* field, Args... args){
		_sqlBuilder->select(field);
		return selectInner(args...);
	}
	SQLCommand& selectInner(const char* field){
		_sqlBuilder->select(field);
		return *this;
	}

	template<typename... Args>
	SQLCommand& where(const Expr& expr, Args... args){
		return whereInner(expr, args...);
	}
	template<typename... Args>
	SQLCommand& whereInner(const Expr& expr, Args... args){
		_sqlBuilder->where(expr);
		return whereInner(args...);
	}
	SQLCommand& whereInner(const Expr& expr){
		_sqlBuilder->where(expr);
		return *this;
	}

	template<typename... Args>
	SQLCommand& orWhere(const Expr& expr, Args... args){
		return orWhereInner(expr, args...);
	}
	template<typename... Args>
	SQLCommand& orWhereInner(const Expr& expr, Args... args){
		_sqlBuilder->orWhere(expr);
		return orWhereInner(args...);
	}
	SQLCommand& orWhereInner(const Expr& expr){
		_sqlBuilder->orWhere(expr);
		return *this;
	}

	template<typename... Args>
	SQLCommand& insert(const SetExpr* val, Args... args){
		return insertInner(val, args...);
	}
	template<typename... Args>
	SQLCommand& insertInner(const SetExpr* val, Args... args){
		_sqlBuilder->insert(val);
		return insertInner(args...);
	}
	SQLCommand& insertInner(const SetExpr* val){
		_sqlBuilder->insert(val);
		return *this;
	}

	template<typename... Args>
	SQLCommand& update(const SetExpr* val, Args... args){
		return updateInner(val, args...);
	}
	template<typename... Args>
	SQLCommand& updateInner(const SetExpr* val, Args... args){
		_sqlBuilder->update(val);
		return updateInner(val, args...);
	}
	SQLCommand& updateInner(const SetExpr* val){
		_sqlBuilder->update(val);
		return *this;
	}

	template<typename... Args>
	SQLCommand& save(const SetExpr* val, Args... args){
		return saveInner(val, args...);
	}
	template<typename... Args>
	SQLCommand& saveInner(const SetExpr* val, Args... args){
		_sqlBuilder->save(val);
		return saveInner(val, args...);
	}
	SQLCommand& saveInner(const SetExpr* val){
		_sqlBuilder->save(val);
		return *this;
	}

private:
	ISQLBuilder* _sqlBuilder;
};

class IMysqlResult{
public:
	virtual ~IMysqlResult() {}
	virtual const char* errInfo() = 0;
	virtual int32 errCode()  = 0;
	virtual int32 rowCount() const = 0;
	virtual int32 columnCount() const = 0;
	virtual bool columnExist(const char* column) const = 0;
	virtual uint32 columnLength(const char* column) const = 0;
	virtual uint32 columnMaxLength(const char* column) const = 0;
	virtual uint32 columnFlags(const char* column) const = 0;
	virtual int32 columnType(const char* column) const = 0;
	
	virtual int8 getDataInt8(const int32 row, const char* column) const = 0;
	virtual int16 getDataInt16(const int32 row, const char* column) const = 0;
	virtual int32 getDataInt32(const int32 row, const char* column) const = 0;
	virtual int64 getDataInt64(const int32 row, const char* column) const = 0;
	virtual float getDataFloat(const int32 row, const char* column) const = 0;
	virtual const char* getDataString(const int32 row, const char* column) const = 0;
	virtual const void* getDataBlob(const int32 row, const char* column, int32& dataSize) const = 0;
	virtual const std::vector<std::string>& columns() const = 0;
	virtual const uint64 affectedRows() const = 0;
	virtual const uint64 insertId() const = 0;
	virtual const int32 optType() const = 0;
	virtual void release() = 0;
};

typedef std::function<bool(sl::api::IKernel* pKernel, SQLCommand& sqlCommnand)> SQLCommnandFunc;
typedef std::function<bool(sl::api::IKernel* pKernel, IMysqlResult* result)> SQLExecCallback;
class IDBInterface{
public:
	virtual ~IDBInterface() {}
	//异步执行sql语句，子线程
	virtual void execSql(const int64 id, const SQLCommnandFunc& command, const SQLExecCallback& cb) = 0;
	virtual void execSql(const int64 id, const int32 optType, const char* sql, const SQLExecCallback& cb) = 0;
	//同步执行sql语句
	virtual IMysqlResult* execSqlSync(const SQLCommnandFunc& f) = 0;
	virtual IMysqlResult* execSqlSync(const int32 optType, const char* sql) = 0;
	virtual IMysqlResult* getTableFields(const char* tableName) = 0;
	virtual SQLCommand& createSqlCommand() = 0;

	virtual const char* host() = 0;
	virtual const int32 port() = 0;
	virtual const char* user() = 0;
	virtual const char* passwd() = 0;
	virtual const char* dbName() = 0;
	virtual const char* charset() = 0;
	virtual const int32 threadCount() = 0;
};

class IMysqlMgr : public sl::api::IModule{
public:
	virtual  ~IMysqlMgr(){}
	virtual IDBInterface* createDBInterface(const char* host, const int32 port, const char* user, const char* pwd, const char* dbName, const char* charset, int32 threadNum = 1) = 0;
	virtual void closeDBInterface(const char* host, const int32 port, const char* dbName) = 0;
};
#endif
