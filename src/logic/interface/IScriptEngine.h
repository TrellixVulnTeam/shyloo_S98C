#ifndef _SL_INTERFACE_SCRIPT_ENGINE_H__
#define _SL_INTERFACE_SCRIPT_ENGINE_H__
#include "slimodule.h"
#include <functional>

class IScriptParamsReader{
public:
	virtual ~IScriptParamsReader() {}

	virtual int32 count() const = 0;
	virtual bool getBoolean(const int32 index) const = 0;
	virtual int8 getInt8(const int32 index) const = 0;
	virtual int16 getInt16(const int32 index) const = 0;
	virtual int32 getInt32(const int32 index) const = 0;
	virtual int64 getInt64(const int32 index) const = 0;
	virtual float getFloat(const int32 index) const = 0;
	virtual const char* getString(const int32 index) const = 0;
	virtual void* getPointer(const int32 index) const = 0;
};

class IScriptParamsWriter{
public:
	virtual ~IScriptParamsWriter() {}

	virtual int32 count() = 0;

	virtual void addBoolean(const bool value) = 0;
	virtual void addInt8(const int8 value) = 0;
	virtual void addInt16(const int16 value) = 0;
	virtual void addInt32(const int32 value) = 0;
	virtual void addInt64(const int64 value) = 0;
	virtual void addFloat(const float value) = 0;
	virtual void addString(const char* value) = 0;
	virtual void addPointer(void* value) = 0;
};

typedef std::function<void(sl::api::IKernel* pKernel, IScriptParamsReader* reader, IScriptParamsWriter* writer)> ScriptFuncType;

class IScriptResult{
public:
	virtual ~IScriptResult(){}

	virtual int32 count() = 0;
	virtual bool getBool(const int32 index) = 0;
	virtual int8 getInt8(const int32 index) = 0;
	virtual int16 getInt16(const int32 index) = 0;
	virtual int32 getInt32(const int32 index) = 0;
	virtual int64 getInt64(const int32 index) = 0;
	virtual float getFloat(const int32 index) = 0;
	virtual const char* getString(const int32 index) = 0;
	virtual void* getPointer(const int32 index) = 0;
};

typedef std::function<void(sl::api::IKernel* pKernel, const IScriptResult* result)> ScriptResultReadFuncType;

class IScriptCallor{
public:
	virtual ~IScriptCallor(){}

	virtual void addBool(bool value) = 0;
	virtual void addInt8(int8 value) = 0;
	virtual void addInt16(int16 value) = 0;
	virtual void addInt32(int32 value) = 0;
	virtual void addInt64(int64 value) = 0;
	virtual void addFloat(float value) = 0;
	virtual void addDouble(double value) = 0;
	virtual void addString(const char* value) = 0;
	virtual void addPointer(void* value) = 0;

	virtual bool call(sl::api::IKernel* pKernel, const ScriptResultReadFuncType& f) = 0;
};

class IScriptEngine: public sl::api::IModule{
public:
	virtual ~IScriptEngine() {}

	virtual IScriptCallor* prepareCall(const char* module, const char* func) = 0;
	virtual void RsgModuleFunc(const char* module, const char* func, const ScriptFuncType& f) = 0;
};
#endif