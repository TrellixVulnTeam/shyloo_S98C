#ifndef SL_XML_H
#define SL_XML_H
#include "slxml.h"
#include "tinyxml.h"
#include <vector>
#include <map>
namespace sl
{
namespace xml
{

class CSLXMLNull : public ISLXmlNode
{
	virtual int8 SLAPI getAttributeInt8(const char* name){ SLASSERT(false, "this is null xml node"); return 0; }
	virtual int16 SLAPI getAttributeInt16(const char* name){ SLASSERT(false, "this is null xml node"); return 0; }
	virtual int32 SLAPI getAttributeInt32(const char* name){ SLASSERT(false, "this is null xml node"); return 0; }
	virtual int64 SLAPI getAttributeInt64(const char* name){ SLASSERT(false, "this is null xml node"); return 0; }
	virtual const char* SLAPI getAttributeString(const char* name){ SLASSERT(false, "this is null xml node"); return nullptr; }
	virtual float SLAPI getAttributeFloat(const char* name){ SLASSERT(false, "this is null xml node"); return 0.0; }
	virtual ISLXmlNode& SLAPI operator[] (const char* nodeName){ SLASSERT(false, "this is null xml node"); return *this; }
	virtual ISLXmlNode& SLAPI operator[](const int32 i){ SLASSERT(false, "this is null xml node"); return *this; }
};

class CSLXmlArray;
class CSLXmlNode : public ISLXmlNode
{
public:
	struct AttrVal
	{
		std::string AttrStr;
		int64		AttrInt64;
		float		AttrFloat;
		bool		AttrBoolean;
	};

	CSLXmlNode(const TiXmlElement* poTiXmlNode);
	virtual ~CSLXmlNode();
	virtual int8 SLAPI getAttributeInt8(const char* name);
	virtual int16 SLAPI getAttributeInt16(const char* name);
	virtual int32 SLAPI getAttributeInt32(const char* name);
	virtual int64 SLAPI getAttributeInt64(const char* name);
	virtual const char* SLAPI getAttributeString(const char* name);
	virtual float SLAPI getAttributeFloat(const char* name);
	virtual ISLXmlNode& SLAPI operator[] (const char* nodeName);
	virtual ISLXmlNode& SLAPI operator[](const int32 i){ SLASSERT(false, "this is xml node"); return m_xmlNull; }

	void loadChildren(const TiXmlElement* element);
	void loadAttributes(const TiXmlElement* element);
	AttrVal* findAttr(const char* name);
private:
	std::map<std::string, CSLXmlArray*> m_xmlChilds;
	std::map<std::string, AttrVal> m_xmlAttrs;
	CSLXMLNull	m_xmlNull;
};

class CSLXmlArray : public ISLXmlNode
{
public:
	~CSLXmlArray()
	{
		for (int32 i = 0; i < (int32)m_elements.size(); i++){
			DEL m_elements[i];
		}
		m_elements.clear();
	}
	virtual int8 SLAPI getAttributeInt8(const char* name){ SLASSERT(false, "this is xml array"); return 0; }
	virtual int16 SLAPI getAttributeInt16(const char* name){ SLASSERT(false, "this is xml array"); return 0; }
	virtual int32 SLAPI getAttributeInt32(const char* name){ SLASSERT(false, "this is xml array"); return 0; }
	virtual int64 SLAPI getAttributeInt64(const char* name){ SLASSERT(false, "this is xml array"); return 0; }
	virtual const char* SLAPI getAttributeString(const char* name){ SLASSERT(false, "this is xml array"); return nullptr; }
	virtual float SLAPI getAttributeFloat(const char* name){ SLASSERT(false, "this is xml array"); return 0.0; }
	virtual ISLXmlNode& SLAPI operator[] (const char* nodeName){ SLASSERT(false, "this is xml array"); return *this; }
	virtual ISLXmlNode& SLAPI operator[](const int32 i)
	{
		if (i < 0 || i >= (int32)m_elements.size()){
			SLASSERT(false, "invalid index");
			return m_xmlNull;
		}
		return *m_elements[i];
	}

	void addElement(CSLXmlNode* poXmlNode){
		if (poXmlNode)
			m_elements.push_back(poXmlNode);
	}
private:
	std::vector<CSLXmlNode*> m_elements;
	CSLXMLNull	m_xmlNull;
};


class CSLXmlReader : public ISLXmlReader
{
public:
	CSLXmlReader();
	~CSLXmlReader();
	virtual bool SLAPI loadXmlFile(const char* path);
	virtual ISLXmlNode& SLAPI root();
	virtual void SLAPI release();
private:
	CSLXmlNode* m_pRootNode;
};

}
}
#endif