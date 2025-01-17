#include "DBCreator.h"
#include "DBCall.h"
#include <sstream>
#include "sltime.h"
#include "NodeProtocol.h"
#include "IHarbor.h"
#include "EventID.h"
#include "IEventEngine.h"
#include "NodeDefine.h"
#include <algorithm>

sl::api::IKernel* DBCreator::s_kernel = nullptr;
IHarbor* DBCreator::s_harbor = nullptr;
IMysqlMgr* DBCreator::s_mysqlMgr = nullptr;
DBCreator* DBCreator::s_self = nullptr;
IEventEngine* DBCreator::s_eventEngine = nullptr;
DBCreator::DataBaseConfig	DBCreator::s_dbConfig;
string DBCreator::s_lastUpdateTable = "";
bool DBCreator::s_updateFinished = false;
bool DBCreator::initialize(sl::api::IKernel * pKernel){
	s_self = this;
	s_kernel = pKernel;

	if (!loadDataBaseConfig(pKernel)){
		SLASSERT(false, "wtf");
		return false;
	}

	return true;
}

bool DBCreator::launched(sl::api::IKernel * pKernel){
	FIND_MODULE(s_harbor, Harbor);
	FIND_MODULE(s_eventEngine, EventEngine);
	if (s_harbor->getNodeType() == NodeType::DATABASE){
		FIND_MODULE(s_mysqlMgr, MysqlMgr);

		updateDBTables();
	}
	else{
		RGS_NODE_ARGS_HANDLER(s_harbor, NodeProtocol::DB_MSG_UPDATE_DATABASE_FINISHED, DBCreator::onDatabaseUpdateFinished);
	}
	
	return true;
}

bool DBCreator::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

bool DBCreator::loadDataBaseConfig(sl::api::IKernel* pKernel){
	string dbConfigDir = string(pKernel->getEnvirPath()) + "database.xml";
	sl::XmlReader conf;
	if (!conf.loadXml(dbConfigDir.c_str())){
		SLASSERT(false, "load config file %s failed", dbConfigDir.c_str());
		return false;
	}

	s_dbConfig._dbName = conf.root().getAttributeString("name");

	const sl::ISLXmlNode& tables = conf.root()["table"];
	for (int32 i = 0; i < tables.count(); i++){
		TableConfig tableConf;
		tableConf._tableName = tables[i].getAttributeString("name");

		set<string> autoIncrementFields;
		loadDBFieldsConfig(tableConf, tables[i], autoIncrementFields);
		loadDBIndexsConfig(tableConf, tables[i], autoIncrementFields);
		loadDBPartitionsConfig(tableConf, tables[i]);

		SLASSERT(s_dbConfig._tables.find(tableConf._tableName) == s_dbConfig._tables.end(), "wtf");
		s_dbConfig._tables[tableConf._tableName] = tableConf;
	}

	return true;
}

bool DBCreator::loadDBFieldsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& table, set<string>& autoIncrementFields){
	const sl::ISLXmlNode& fields = table["field"];
	for (int32 col = 0; col < fields.count(); ++col){
		FieldInfo fieldInfo;
		fieldInfo._fieldName = fields[col].getAttributeString("name");
		const char* fieldType = fields[col].getAttributeString("type");
		const char* typeSize = nullptr;
		if (fields[col].hasAttribute("size")){
			typeSize = fields[col].getAttributeString("size");
		}

		fieldInfo._fieldType = DBType().convert(fieldType, typeSize);
		fieldInfo._null = "YES";
		if (fields[col].hasAttribute("notNull") && fields[col].getAttributeBoolean("notNull")){
			fieldInfo._null = "NO";
		}

		if (fields[col].hasAttribute("extra")){
			fieldInfo._extra = fields[col].getAttributeString("extra");
			transform(fieldInfo._extra.begin(), fieldInfo._extra.end(), fieldInfo._extra.begin(), ::toupper);
			if (fieldInfo._extra == "AUTO_INCREMENT")
				autoIncrementFields.insert(fieldInfo._fieldName);
		}

		if (fields[col].hasAttribute("default")){
			fieldInfo._defaultValue = fields[col].getAttributeString("default");
		}
		SLASSERT(tableConfig._fields.find(fieldInfo._fieldName) == tableConfig._fields.end(), "wtf");
		tableConfig._fields[fieldInfo._fieldName] = fieldInfo;
		tableConfig._fieldsVec.push_back(fieldInfo);
	}
	return true;
}

bool DBCreator::loadDBIndexsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& index, set<string>& autoIncrementFields){
	if (!index.subNodeExist("index"))
		return true;

	const sl::ISLXmlNode& indexs = index["index"];
	for (int32 idx = 0; idx < indexs.count(); idx++){
		const char* indexType = indexs[idx].getAttributeString("type");
		const char* fields = indexs[idx].getAttributeString("fields");

		IndexInfo idxInfo{ indexType, fields, "" };
		if (indexs[idx].hasAttribute("func")){
			idxInfo._func = indexs[idx].getAttributeString("func");
		}

		transform(idxInfo._indexType.begin(), idxInfo._indexType.end(), idxInfo._indexType.begin(), ::toupper);
		transform(idxInfo._func.begin(), idxInfo._func.end(), idxInfo._func.begin(), ::toupper);
		SLASSERT(idxInfo._func == "" || idxInfo._func == "BTREE" || idxInfo._func == "HASH", "invaild idx type");

		string indexName = "";
		if (idxInfo._indexType == "PRIMARY"){
			indexName = idxInfo._indexType;
			SLASSERT(tableConfig._indexs.find(indexName) == tableConfig._indexs.end(), "duplicate primary key");
			
			vector<string> indexVec;
			sl::CStringUtils::Split(fields, ",", indexVec);
			for (auto vec : indexVec){
				SLASSERT(tableConfig._fields[vec]._null == "NO", "primary ket must be not null");
				tableConfig._fields[vec]._null = "NO";
				auto findItor = autoIncrementFields.find(vec);
				if (findItor != autoIncrementFields.end())
					autoIncrementFields.erase(findItor);
			}
		}
		else{
			indexName = fields;
			auto strPos = indexName.find(",");
			while (strPos != string::npos){
				indexName.replace(strPos, 1, "_");
				strPos = indexName.find(",");
			}
		}

		SLASSERT(tableConfig._indexs.find(indexName) == tableConfig._indexs.end(), "wtf");
		tableConfig._indexs[indexName] = idxInfo;
	}

	if (!autoIncrementFields.empty()){
		auto cItor = autoIncrementFields.begin();
		for (; cItor != autoIncrementFields.end(); ++cItor){
			SLASSERT(false, "%s must be primary key for autoIncrement", *cItor);
			tableConfig._fields[*cItor]._extra = "";
		}
	}

	return true;
}

bool DBCreator::loadDBPartitionsConfig(TableConfig& tableConfig, const sl::ISLXmlNode& partition){
	tableConfig._partition._limit = 0;
	if (!partition.subNodeExist("partition"))
		return true;

	const sl::ISLXmlNode& partitions = partition["partition"];
	SLASSERT(partitions.count() == 1, "wtf");
	tableConfig._partition._field = partitions[0].getAttributeString("field");
	tableConfig._partition._type = partitions[0].getAttributeString("type");
	tableConfig._partition._partByDay = false;

	const char* defaultValue = partitions[0].getAttributeString("default");
	if (strcmp(defaultValue, "CURRENT_TIMESTAMP") == 0){
		SLASSERT(tableConfig._fields[tableConfig._partition._field]._fieldType._dbType == DBType::DBTYPE_TIME, "wtf");
		tableConfig._partition._partByDay = true;
		if (partitions[0].hasAttribute("limit")){
			tableConfig._partition._limit = partitions[0].getAttributeInt32("limit");
		}
	}
	else{
		vector<string> valVec;
		sl::CStringUtils::Split(partitions[0].getAttributeString("default"), ",", valVec);
		for (auto& val : valVec){
			string valIdx = tableConfig._partition._field + val;
			auto strPos = valIdx.find("-");
			while (strPos != string::npos){
				valIdx.replace(strPos, 1, "_");
				strPos = valIdx.find("-");
			}
			tableConfig._partition._defaultValue[valIdx] = val;
		}
	}
	return true;
}

void DBCreator::setLastUpdateTable(IMysqlResult* result){
	string dbTableName("Tables_in_");
	dbTableName += s_dbConfig._dbName;

	string lastDBTable = "";
	TablesMap tablesInfo = s_dbConfig._tables;
	const int32 rowCount = result->rowCount();
	for (int32 i = 0; i < rowCount; i++){
		const char* tableName = result->getDataString(i, dbTableName.c_str());
		auto confItor = tablesInfo.find(tableName);
		if (confItor != tablesInfo.end()){
			tablesInfo.erase(confItor);
		}
		lastDBTable = tableName;
	}

	string lastCreateTable = "";
	auto addItor = tablesInfo.begin();
	for (; addItor != tablesInfo.end(); ++addItor){
		lastCreateTable = addItor->first;
	}

	s_lastUpdateTable = lastCreateTable == "" ? lastDBTable : lastCreateTable;
}

void DBCreator::tablesShowCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	s_self->setLastUpdateTable(result);

	std::string dbTablesName("Tables_in_");
	dbTablesName += s_dbConfig._dbName;

	TablesMap tablesInfo = s_dbConfig._tables;
	const int32 rowCount = result->rowCount();
	for (int32 i = 0; i < rowCount; i++){
		const char* tableName = result->getDataString(i, dbTablesName.c_str());
		auto confItor = tablesInfo.find(tableName);
		if (confItor == tablesInfo.end())
			s_self->dropDBTable(tableName);
		else{
			s_self->descDBTable(confItor->first.c_str());
			s_self->showIndexDBTable(confItor->first.c_str());
			s_self->showPartitionDBTable(confItor->first.c_str());
			tablesInfo.erase(confItor);
		}
	}

	if (!tablesInfo.empty()){
		auto addItor = tablesInfo.begin();
		for (; addItor != tablesInfo.end(); ++addItor){
			s_self->createDBTable(addItor->first.c_str());
		}
	}

	if (s_lastUpdateTable == ""){
		s_self->broadcastDBUpdateFinished();
	}

}

void DBCreator::tableDropCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (strcmp(s_lastUpdateTable.c_str(), tableName) == 0)
		s_self->broadcastDBUpdateFinished();
}

void DBCreator::tableDescCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (success)
		s_self->updateDBTable(tableName, result);
}

void DBCreator::tableCreateCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (strcmp(s_lastUpdateTable.c_str(), tableName) == 0)
		s_self->broadcastDBUpdateFinished();
}

void DBCreator::tableShowIndexCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (success){
		s_self->updateDBTableIndexs(tableName, result);
		s_self->updateDBTablePrimaryKey(tableName, result);
	}
}

void DBCreator::tableShowPartitionCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (success){
		s_self->updateDBTablePartition(tableName, result);
	}
}

void DBCreator::tableAddPartitionCB(sl::api::IKernel* pKernel, const char* tableName, const bool success, IMysqlResult* result){
	if (strcmp(s_lastUpdateTable.c_str(), tableName) == 0)
		s_self->broadcastDBUpdateFinished();
}

void DBCreator::updateDBTables(){
	DBCall* callor = NEW DBCall("", 0);
	callor->exec(DB_OPT_QUERY, "SHOW TABLES", DBCreator::tablesShowCB);
}


bool DBCreator::dropDBTable(const char* tableName){
	ostringstream os;
	os << "DROP	TABLE IF EXISTS `" << tableName << "`";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_DELETE, os.str().c_str(), tableDropCB);
	return true;
}

bool DBCreator::descDBTable(const char* tableName){
	ostringstream os;
	os << "DESC	`" << tableName << "`";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_QUERY, os.str().c_str(), DBCreator::tableDescCB);
	return true;
}

bool DBCreator::showIndexDBTable(const char* tableName){
	ostringstream os;
	os << "SHOW INDEX FROM `" << tableName << "`";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_QUERY, os.str().c_str(), DBCreator::tableShowIndexCB);
	return true;
}

bool DBCreator::showPartitionDBTable(const char* tableName){
	ostringstream os;
	os << "SELECT partition_name, partition_expression FROM information_schema.partitions where TABLE_SCHEMA = schema() and table_name='" << tableName << "'";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_QUERY, os.str().c_str(), DBCreator::tableShowPartitionCB);
	return true;
}

bool DBCreator::createDBTable(const char* tableName){
	auto itor = s_dbConfig._tables.find(tableName);
	if (itor == s_dbConfig._tables.end()){
		SLASSERT(false, "config not contain table %s", tableName);
		return false;
	}

	ostringstream os; 
	os << "CREATE TABLE `" << tableName << "` (";
	FieldsVec& fields = itor->second._fieldsVec;
	for (int32 idx = 0; idx < (int32)fields.size(); idx++){
		bool sep = idx != 0 ? true : false;
		appendFieldProp(os, fields[idx], sep);
	}

	auto indexItor = itor->second._indexs.begin();
	for (; indexItor != itor->second._indexs.end(); ++indexItor){
		appendIndexProp(os, indexItor->first.c_str(), indexItor->second);
	}

	os << ") ENGINE = InnoDB DEFAULT CHARSET = utf8";

	if (itor->second._partition._field != ""){
		auto partColItor = itor->second._fields.find(itor->second._partition._field);
		if (partColItor != itor->second._fields.end()){
			if (itor->second._partition._partByDay){
				SLASSERT(itor->second._partition._defaultValue.empty(), "wtf");
				setDayPartitionValue(itor->second._partition, sl::getTimeMilliSecond() + 10 * MINUTE);
				setDayPartitionValue(itor->second._partition, sl::getTimeMilliSecond() + 10 * MINUTE + 1 * DAY);
				setDayPartitionValue(itor->second._partition, sl::getTimeMilliSecond() + 10 * MINUTE + 2 * DAY);
			}

			appendDBTablePartition(tableName, os, partColItor->second, itor->second._partition, false);
		}
	}

	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_INSERT, os.str().c_str(), DBCreator::tableCreateCB);
	return true;
}

bool DBCreator::updateDBTable(const char* tableName, IMysqlResult* result){
	if (s_dbConfig._tables.find(tableName) == s_dbConfig._tables.end())
		return false;

	FieldsMap fieldsInfo = s_dbConfig._tables[tableName]._fields;
	const int32 rowCount = result->rowCount();
	for (int32 i = 0; i < rowCount; i++){
		const char* fieldName = result->getDataString(i, "Field");
		auto confItor = fieldsInfo.find(fieldName);
		if (confItor == fieldsInfo.end()){
			s_self->dropDBTableField(tableName, fieldName);
		}
		else{
			updateDBTableField(tableName, confItor->first.c_str(), confItor->second, i, result);
			fieldsInfo.erase(confItor);
		}
	}

	if (!fieldsInfo.empty()){
		auto addItor = fieldsInfo.begin();
		for (; addItor != fieldsInfo.end(); ++ addItor){
			addDBTableField(tableName, addItor->first.c_str(), addItor->second);
		}
	}

	return true;
}

bool DBCreator::dropDBTableField(const char* tableName, const char* field){
	ostringstream os;
	os << "ALTER TABLE `" << tableName << "` DROP `" << field << "`";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_DELETE, os.str().c_str(), nullptr);
	return true;
}

bool DBCreator::addDBTableField(const char* tableName, const char* field, FieldInfo& confInfo){
	ostringstream addStr;
	addStr << "ALTER TABLE `" << tableName << "` ADD ";
	appendFieldProp(addStr, confInfo, false);

	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_INSERT, addStr.str().c_str(), nullptr);
	return true;
}

bool DBCreator::updateDBTableField(const char* tableName, const char* field, FieldInfo& confInfo, const int32 tableIdx, IMysqlResult* result){
	if (!checkFieldChanged(confInfo, tableIdx, result))
		return true;

	ostringstream updateStr;
	updateStr << "ALTER TABLE `" << tableName << "` CHANGE `" << field << "` ";
	appendFieldProp(updateStr, confInfo, false);

	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_UPDATE, updateStr.str().c_str(), nullptr);
	return true;
}

bool DBCreator::checkFieldChanged(FieldInfo& confInfo, const int32 tableIdx, IMysqlResult* result){
	if (!result)
		return true;

	const char* typeInfo = result->getDataString(tableIdx, "Type");
	const char* nullInfo = result->getDataString(tableIdx, "Null");
	const char* defaultInfo = result->getDataString(tableIdx, "Default");
	const char* extraInfo = result->getDataString(tableIdx, "Extra");

	bool needUpdate = confInfo._fieldType != DBType().convert(typeInfo) ? true : false;
	if (strcmp(confInfo._null.c_str(), nullInfo) != 0){
		needUpdate = true;
	}

	if (_stricmp(confInfo._extra.c_str(), extraInfo) != 0){
		needUpdate = true;
	}

	if (strcmp(confInfo._defaultValue.c_str(), defaultInfo) != 0){
		needUpdate = true;
	}

	return needUpdate;
}

void DBCreator::appendFieldProp(ostringstream& os, FieldInfo& confInfo, bool sep){
	if (sep)
		os << ",";

	os << "`" << confInfo._fieldName << "` " << confInfo._fieldType.c_str();
	if (confInfo._null == "NO")
		os << " NOT NULL";

	if (confInfo._extra != "AUTO_INCREMENT" && confInfo._defaultValue != ""){
		if (isupper(confInfo._defaultValue[0])){
			os << " DEFAULT " << confInfo._defaultValue;
		}
		else{
			os << " DEFAULT " << "\'" << confInfo._defaultValue << "\'";
		}
	}

	if (strcmp(confInfo._extra.c_str(), "") != 0)
		os << " " << confInfo._extra;
}

bool DBCreator::updateDBTableIndexs(const char* tableName, IMysqlResult* result){
	if (s_dbConfig._tables.find(tableName) == s_dbConfig._tables.end())
		return false;

	IndexsMap indexsConf = s_dbConfig._tables[tableName]._indexs;
	FieldsMap fieldsConf = s_dbConfig._tables[tableName]._fields;

	const int32 rowCount = result->rowCount();
	for (int32 i = 0; i < rowCount; i++){
		const char* indexKeyName = result->getDataString(i, "Key_name");
		const char* seqInIndex = result->getDataString(i, "Seq_in_index");
		const char* indexColumnName = result->getDataString(i, "Column_name");
		const char* noUnique = result->getDataString(i, "Non_unique");
		if (strcmp(indexKeyName, "PRIMARY") != 0 && strcmp(seqInIndex, "1") == 0){
			auto confItor = indexsConf.find(indexKeyName);
			if (confItor == indexsConf.end() || (confItor->second._indexType == "UNIQUE" && (strcmp(noUnique, "1") == 0)) ||
				(confItor->second._indexType == "NORMAL" && (strcmp(noUnique, "0") == 0))){
				s_self->dropDBTableIndex(tableName, indexKeyName);
			}
			else{
				indexsConf.erase(confItor);
			}
		}
	}

	if (!indexsConf.empty()){
		auto idxItor = indexsConf.begin();
		for (; idxItor != indexsConf.end(); ++idxItor){
			if (idxItor->second._indexType == "PRIMARY")
				continue;

			addDBTableIndex(tableName, idxItor->first.c_str(), idxItor->second);
		}
	}
	return true;
}

bool DBCreator::dropDBTableIndex(const char* tableName, const char* indexName){
	ostringstream dropItor;
	dropItor << "DROP INDEX `" << indexName << "` ON `" << tableName << "`";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_DELETE, dropItor.str().c_str(), nullptr);
	return true;
}

bool DBCreator::addDBTableIndex(const char* tableName, const char* indexName, const IndexInfo& indexInfo){
	ostringstream updateStr;
	updateStr << "ALTER TABLE `" << tableName << "` ADD ";
	appendIndexProp(updateStr, indexName, indexInfo, false);

	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_INSERT, updateStr.str().c_str(), nullptr);
	return true;
}

bool DBCreator::updateDBTablePrimaryKey(const char* tableName, IMysqlResult* result){
	if (s_dbConfig._tables.find(tableName) == s_dbConfig._tables.end())
		return false;

	IndexsMap indexsInfo = s_dbConfig._tables[tableName]._indexs;
	FieldsMap fieldsInfo = s_dbConfig._tables[tableName]._fields;

	vector<string> priKeys;
	auto priKeyItor = indexsInfo.find("PRIMARY");
	if (priKeyItor != indexsInfo.end()){
		string fields = priKeyItor->second._fields;
		sl::CStringUtils::Split(fields, ",", priKeys);
	}

	int32 dbPriKeysNum = 0;
	bool needUpdatePriKeys = false;
	vector<string> autoIncdbPriKeys;

	const int32 rowCount = result->rowCount();
	for (int32 i = 0; i < rowCount; i++){
		const char* indexKeyName = result->getDataString(i, "Key_name");
		const char* seqInIndex = result->getDataString(i, "Seq_in_index");
		const char* IndexColumnName = result->getDataString(i, "Column_name");
		
		if (strcmp(indexKeyName, "PRIMARY") != 0)
			continue;

		auto priKeyItor = indexsInfo.find("PRIMARY");
		if (priKeyItor == indexsInfo.end()){
			needUpdatePriKeys = true;
		}
		else{
			dbPriKeysNum++;
			if (dbPriKeysNum > (int32)priKeys.size()){
				needUpdatePriKeys = true;
			}
			else if (strcmp(priKeys[dbPriKeysNum - 1].c_str(), IndexColumnName) != 0){
				needUpdatePriKeys = true;
			}

			SLASSERT(fieldsInfo.find(IndexColumnName) != fieldsInfo.end(), "wtf");
			if (fieldsInfo[IndexColumnName]._extra == "AUTO_INCREMENT"){
				autoIncdbPriKeys.push_back(IndexColumnName);
			}
		}
	}

	if (needUpdatePriKeys && dbPriKeysNum != (int32)priKeys.size()){
		needUpdatePriKeys = true;
	}

	//dropd原有的primary
	if (needUpdatePriKeys && dbPriKeysNum > 0){
		dropDBTablePrimaryKey(tableName, autoIncdbPriKeys);
	}

	//恢复字段的AUTO_INCREMENT属性
	if (!needUpdatePriKeys || autoIncdbPriKeys.empty() || priKeys.empty())
		return true;

	for (auto& autoIncKey : autoIncdbPriKeys){
		bool updateCol = false;
		for (auto& confPriKey : priKeys){
			if (confPriKey == autoIncKey){
				updateCol = true;
				break;
			}
		}
		if (updateCol){
			fieldsInfo[autoIncKey]._extra = "AUTO_INCREMENT";
			updateDBTableField(tableName, autoIncKey.c_str(), fieldsInfo[autoIncKey], 0, nullptr);
		}
	}
	return true;
}

bool DBCreator::dropDBTablePrimaryKey(const char* tableName, vector<string>& autoIncdbPriKeys){
	if (s_dbConfig._tables.find(tableName) == s_dbConfig._tables.end())
		return false;

	FieldsMap fieldInfo = s_dbConfig._tables[tableName]._fields;

	//先去掉字段的AUTO_INCREMENT属性
	for (auto& autoIncKey : autoIncdbPriKeys){
		fieldInfo[autoIncKey]._extra = "";
		updateDBTableField(tableName, autoIncKey.c_str(), fieldInfo[autoIncKey], 0, nullptr);
	}

	ostringstream delStr;
	delStr << "ALTER TABLE `" << tableName << "` DROP PRIMARY KEY";
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_DELETE, delStr.str().c_str(), nullptr);
	return true;
}

void DBCreator::appendIndexProp(ostringstream& os, const char* indexName, const IndexInfo& indexInfo, bool sep){
	if (sep)
		os << ", ";

	if (indexInfo._indexType == "PRIMARY"){
		os << "PRIMARY KEY(" << indexInfo._fields << ")";
	}
	if (indexInfo._indexType == "UNIQUE"){
		os << "UNIQUE KEY `" << indexName << "` (" << indexInfo._fields << ")";
	}
	if (indexInfo._indexType == "NORMAL"){
		os << "KEY `" << indexName << "` (" << indexInfo._fields << ")";
	}

	if (indexInfo._indexType != "PRIMARY" && indexInfo._func != "")
		os << " USING " << indexInfo._func;
}

bool DBCreator::updateDBTablePartition(const char* tableName, IMysqlResult* result){
	auto tableItor = s_dbConfig._tables.find(tableName);
	if (tableItor == s_dbConfig._tables.end()){
		SLASSERT(false, "config not has table %s", tableName);
		return false;
	}

	PartitionInfo partitionInfo = s_dbConfig._tables[tableName]._partition;
	auto colItor = tableItor->second._fields.find(partitionInfo._field);
	if (colItor == tableItor->second._fields.end()){
		if (strcmp(s_lastUpdateTable.c_str(), tableName) == 0)
			broadcastDBUpdateFinished();
		return false;
	}

	bool bNotifyUpdateFinish = false;
	if (partitionInfo._type == "range"){
		if (partitionInfo._partByDay){
			setDayPartitionValue(partitionInfo, sl::getTimeMilliSecond() + 10 * MINUTE);
			setDayPartitionValue(partitionInfo, sl::getTimeMilliSecond() + 10 * MINUTE + 1 * DAY);
			setDayPartitionValue(partitionInfo, sl::getTimeMilliSecond() + 10 * MINUTE + 2 * DAY);
		}

		bool dbHasPartitions = false;
		const int32 rowCount = result->rowCount();
		for (int32 i = 0; i < rowCount; i++){
			const char* partName = result->getDataString(i, "partition_name");
			if (strcmp(partName, "") == 0)
				continue;

			dbHasPartitions = true;

			auto valItor = partitionInfo._defaultValue.find(partName);
			if (valItor != partitionInfo._defaultValue.end()){
				partitionInfo._defaultValue.erase(valItor);
			}
		}

		if (!partitionInfo._defaultValue.empty()){
			addDBTablePartition(tableName, colItor->second, partitionInfo, dbHasPartitions);
			bNotifyUpdateFinish = false;
		}

		//删除多余分区
		if (partitionInfo._partByDay && partitionInfo._limit > 0){
			int32 more = rowCount + (int32)partitionInfo._defaultValue.size() - partitionInfo._limit;
			SLASSERT(more < rowCount, "wtf");
			if (more > 0){
				for (int32 i = 0; i < more; i++){
					const char* partName = result->getDataString(i, "partition_name");
					dropDBTablePartition(tableName, partName);
				}
			}
		}
	}
	else if (partitionInfo._type == "hash"){
		SLASSERT(partitionInfo._defaultValue.size() == 1, "wtf");
		auto valItor = partitionInfo._defaultValue.begin();
		int32 dbPartNum = result->rowCount();
		if (dbPartNum == 1){
			const char* partName = result->getDataString(0, "partition_name");
			if (strcmp(partName, "") == 0)
				dbPartNum = 0;
		}

		//如果分区字段改变
		if (dbPartNum > 0){
			const char* dbExpr = result->getDataString(0, "partition_expression");
			string confField = "`" + partitionInfo._field + "`";
			SLASSERT(strcmp(dbExpr, confField.c_str()) == 0, "please del table %s self", tableName);
		}

		int32 confPartNum = sl::CStringUtils::StringAsInt32(valItor->second.c_str());
		if (confPartNum != dbPartNum){
			partitionInfo._limit = (confPartNum - dbPartNum > 0) ? (confPartNum - dbPartNum) : (dbPartNum - confPartNum);
			partitionInfo._partByDay = (confPartNum - dbPartNum > 0) ? true : false;
			addDBTablePartition(tableName, colItor->second, partitionInfo, dbPartNum > 0);
			bNotifyUpdateFinish = false;
		}
	}

	if (bNotifyUpdateFinish && strcmp(s_lastUpdateTable.c_str(), tableName) == 0)
		broadcastDBUpdateFinished();

	return true;
}

bool DBCreator::addDBTablePartition(const char* tableName, FieldInfo& fieldInfo, PartitionInfo& partInfo, bool isAdd){
	if (partInfo._defaultValue.empty())
		return true;

	ostringstream os;
	os << "ALTER TABLE `" << tableName << "` ";
	appendDBTablePartition(tableName, os, fieldInfo, partInfo, isAdd);

	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_UPDATE, os.str().c_str(), DBCreator::tableAddPartitionCB);

	return true;
}

bool DBCreator::dropDBTablePartition(const char* tableName, const string& partIdx){
	ostringstream dropStr;
	dropStr << "ALTER TABLE `" << tableName << "` DROP PARTITION " << partIdx;
	DBCall* callor = NEW DBCall(tableName, 0);
	callor->exec(DB_OPT_DELETE, dropStr.str().c_str(), nullptr);
	return true;
}

void DBCreator::appendDBTablePartition(const char* tableName, ostringstream& os, FieldInfo& fieldInfo, PartitionInfo& partInfo, bool isAdd){
	if (partInfo._defaultValue.empty())
		return;

	if (partInfo._type == "range"){
		if (isAdd)
			os << " ADD PARTITION(";
		else{
			if (fieldInfo._fieldType._dbType == DBType::DBTYPE_TIME)
				os << " PARTITION by range(UNIX_TIMESTAMP(`" << partInfo._field << "`)) (";
			else
				os << " PARTITION by range(`" << partInfo._field << "`) (";
		}

		int32 i = 1, count = (int32)(partInfo._defaultValue.size());
		auto partItor = partInfo._defaultValue.begin();
		for (; partItor != partInfo._defaultValue.end(); ++partItor){
			os << "PARTITION " << partItor->first << " VALUE LESS THAN ";
			if (fieldInfo._fieldType._dbType == DBType::DBTYPE_TIME)
				os << "(UNIX_TIMESTAMP('" << partItor->second << "'))";
			else
				os << "(" << partItor->second << ")";

			if (i < count)
				os << ",";

			i++;
		}
		os << ")";
	}
	else if (partInfo._type == "hash"){
		if (isAdd){
			if (partInfo._partByDay)
				os << " ADD PARTITION PARTITIONS " << partInfo._limit;
			else
				os << " COALESCE PARTITION " << partInfo._limit;
		}
		else{
			if (fieldInfo._fieldType._dbType == DBType::DBTYPE_TIME)
				os << " PARTITION by hash(UNIX_TIMESTAMP(`" << partInfo._field << "`)) ";
			else
				os << " PARTITION by hash(`" << partInfo._field << "`) ";

			SLASSERT(partInfo._defaultValue.size() == 1, "wtf");
			auto valItor = partInfo._defaultValue.begin();
			os << "PARTITIONS " << valItor->second;
		}
	}
}

void DBCreator::setDayPartitionValue(PartitionInfo& partInfo, int64 tick){
	string timeStr = sl::getTimeStr(tick);
	char* pos = (char*)strstr(timeStr.c_str(), " ");
	if (pos)
		*pos = 0;

	string valIdx = partInfo._field + timeStr;
	auto strPos = valIdx.find("-");
	while (strPos != string::npos){
		valIdx.replace(strPos, 1, "_");
		strPos = valIdx.find("-");
	}

	partInfo._defaultValue[valIdx] = timeStr;
}

void DBCreator::broadcastDBUpdateFinished(){
	if (s_updateFinished){
		SLASSERT(false, "has broadcast msg");
		return;
	}

	s_updateFinished = true;

	IArgs<1, 32> args;
	args.fix();
	s_harbor->broadcast(NodeProtocol::DB_MSG_UPDATE_DATABASE_FINISHED, args.out());

	logic_event::DBUpdateFinished info;
	s_eventEngine->execEvent(logic_event::EVENT_DB_UPDATE_FINISHED, &info, sizeof(info));
}

void DBCreator::onDatabaseUpdateFinished(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const OArgs& args){
	logic_event::DBUpdateFinished info;
	s_eventEngine->execEvent(logic_event::EVENT_DB_UPDATE_FINISHED, &info, sizeof(info));
}
