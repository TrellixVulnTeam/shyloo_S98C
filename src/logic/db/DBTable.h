#ifndef __SL_DB_TABLE_H__
#define __SL_DB_TABLE_H__
#include "IObjectDef.h"
#include "IMysqlMgr.h"
#include <map>
//在数据表中的一个字段
#define TABLE_ARRAY_ITEM_VALUES_CONST_STR		"values"
#define TABLE_ARRAY_ITEM_VALUE_CONST_STR		"value"
#define TABLE_PARENTID_CONST_STR				"parentID"
#define TABLE_PERFIX							"tbl"
class DBTable;
class DBTableItem{
public:
	typedef std::vector<std::pair<std::string, DBTableItem*>> KEYTYPE_MAP;
	DBTableItem(const char* itemName, IDataType* dataType);
	virtual ~DBTableItem(){}

	inline void setItemName(const char* name) { _itemName = name;}
	inline const char* itemName() const {return _itemName.c_str();}
	inline  void setTableName(const char* name) {_tableName = name;}
	inline const char* tableName() const {return _tableName.c_str();}
	inline const int32 uType() const {return _dataType->getUid();}
	inline void setParentTable(DBTable* table) { _parentTable = table;}
	inline DBTable* parentTable() const {return _parentTable;}
	inline void setParentTableItem(DBTableItem* parentItem) {_parentTableItem = parentItem;}
	inline DBTableItem* parentTableItem() const {return _parentTableItem;}

	virtual bool initialize() = 0;
	virtual bool syncToDB(IDBInterface* pdbi) = 0;

protected:
	std::string		_itemName;			//字段名称
	std::string		_tableName;			//表名称
	int32			_tableId;			//表id
	DBTable*		_parentTable;		//父表
	DBTableItem*	_parentTableItem;	//父表对应的key字段
	IDataType*		_dataType;			//字段类型
	const IProp*	_prop;				//字段所属的属性
	std::string		_dbItemType;		//数据库创建时的属性
	int32			_flag;				//其他字段flag
};

class DBTable{
public:
	typedef std::map<int32, DBTableItem*> TABLEITEM_MAP;
	DBTable(const char* tableName);
	virtual ~DBTable(){}

	inline void setTableName(const char* name) {_tableName = name;}
	inline const char* tableName() const {return _tableName.c_str();}
	inline void setTableId(const int32 tableId) {_tableId = tableId;}
	inline const int32 tableId() const {return _tableId;} 
	inline void setIsChild(bool isChild) {_isChild = isChild;}
	inline bool isChild() const {return _isChild;}
	inline void setDataBase(DataBase* db) {_database = db;}
	inline DataBase* dataBase() {return _database;}

	void addItem(DBTableItem* item);
	DBTableItem* findItem(int32 utype);

	virtual bool initialize() = 0;
	virtual DBTableItem* createItem(const char* itemName, IDataType* dataType, const char* defaultVal) = 0;
	virtual bool syncToDB(IDBInterface* pdbi) = 0;

protected:
	std::string			_tableName;      //表名称
	int32				_tableId;		 //表id		
	TABLEITEM_MAP		_tableItems;	 //所有的字段	
	DataBase*			_database;		 //所属数据库
	std::vector<DBTableItem*> _tableFixedOrderItems;
	bool				_isChild;		//是否是子表
	bool				_sync;			//是否已经同步

	
};

class DataBase{
public:
	DataBase(IDBInterface* dbInferface);
	virtual ~DataBase(){}

	virtual bool initialize() = 0;
	virtual bool syncToDB() = 0;
	void addTable(DBTable* pTable);
	DBTable* findTable(const char* table);

protected:
	IDBInterface*	_dbInterface;
	std::unordered_map<std::string, DBTable*> _tables;
};


#endif
