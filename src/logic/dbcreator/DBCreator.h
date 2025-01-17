#ifndef __SL_CORE_DBCREATOR_H__
#define __SL_CORE_DBCREATOR_H__
#include "slimodule.h"
#include "DBType.h"
#include <unordered_map>
#include <map>
#include <set>
#include "slxml_reader.h"
#include "IMysqlMgr.h"
class IHarbor;
class OArgs;
class IEventEngine;
class DBCreator :public sl::api::IModule{
public:
	struct FieldInfo{
		string _fieldName;
		DBType _fieldType;
		string _null;
		string _defaultValue;
		string _extra;
	};

	struct IndexInfo{
		string _indexType;
		string _fields;
		string _func;
	};

	struct PartitionInfo{
		string _field;
		string _type;
		int32 _limit;
		bool _partByDay;
		map<string, string> _defaultValue;
	};

	typedef std::unordered_map<string, FieldInfo> FieldsMap;
	typedef std::vector<FieldInfo> FieldsVec;
	typedef std::unordered_map<string, IndexInfo> IndexsMap;
	struct TableConfig{
		string _tableName;
		FieldsMap _fields;
		FieldsVec _fieldsVec;
		IndexsMap _indexs;
		PartitionInfo _partition;
	};

	typedef std::unordered_map<string, TableConfig> TablesMap;
	struct DataBaseConfig{
		string _dbName;
		TablesMap _tables;
	};

public:
	virtual bool initialize(sl::api::IKernel * pKernel);
	virtual bool launched(sl::api::IKernel * pKernel);
	virtual bool destory(sl::api::IKernel * pKernel);

	bool loadDataBaseConfig(sl::api::IKernel* pKernel);
	bool loadDBFieldsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& table, set<string>& autoIncrementFields);
	bool loadDBIndexsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& index, set<string>& autoIncrementFields);
	bool loadDBPartitionsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& partition);

	void updateDBTables();

	static void tablesShowCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableDropCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableDescCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableCreateCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableShowIndexCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableShowPartitionCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);
	static void tableAddPartitionCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result);

	static IMysqlMgr* getMysqlMgr() { return s_mysqlMgr; }

	void onDatabaseUpdateFinished(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const OArgs& args);

private:
	bool descDBTable(const char* tableName);
	bool showIndexDBTable(const char* tableName);
	bool showPartitionDBTable(const char* tableName);

	bool createDBTable(const char* tableName);
	bool updateDBTable(const char* tableName, IMysqlResult* result);
	bool dropDBTable(const char* tableName);

	bool dropDBTableField(const char* tableName, const char* field);
	bool addDBTableField(const char* tableName, const char* field, FieldInfo& confInfo);
	bool updateDBTableField(const char* tableName, const char* field, FieldInfo& confInfo, const int32 tableIdx, IMysqlResult* result);
	bool checkFieldChanged(FieldInfo& confInfo, const int32 tableIdx, IMysqlResult* result);
	void appendFieldProp(ostringstream& os, FieldInfo& confInfo, bool sep = true);

	bool updateDBTableIndexs(const char* tableName, IMysqlResult* result);
	bool dropDBTableIndex(const char* tableName, const char* indexName);
	bool addDBTableIndex(const char* tableName, const char* indexName, const IndexInfo& indexInfo);
	bool updateDBTablePrimaryKey(const char* tableName, IMysqlResult* result);
	bool dropDBTablePrimaryKey(const char* tableName, vector<string>& autoIncdbPriKeys);
	void appendIndexProp(ostringstream& os, const char* indexName, const IndexInfo& indexInfo, bool sep = true);

	bool updateDBTablePartition(const char* tableName, IMysqlResult* result);
	bool addDBTablePartition(const char* tableName, FieldInfo& fieldInfo, PartitionInfo& partInfo, bool isAdd);
	bool dropDBTablePartition(const char* tableName, const string& partIdx);
	void appendDBTablePartition(const char* tableName, ostringstream& os, FieldInfo& fieldInfo, PartitionInfo& partInfo, bool isAdd);
	void setDayPartitionValue(PartitionInfo& partInfo, int64 tick);

	void setLastUpdateTable(IMysqlResult* result);
	void broadcastDBUpdateFinished();

private:
	static sl::api::IKernel*	s_kernel;
	static IHarbor*			s_harbor;
	static IMysqlMgr*   s_mysqlMgr;
	static DBCreator*	s_self;
	static IEventEngine* s_eventEngine;

	static DataBaseConfig	s_dbConfig;
	static string           s_lastUpdateTable;
	static bool	s_updateFinished;
};



#endif