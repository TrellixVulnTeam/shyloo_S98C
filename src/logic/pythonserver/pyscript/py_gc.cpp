/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2017 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

//#include "script.h"
#include "py_gc.h"
//#include "scriptstdouterr.h"
#include "py_macros.h"
#include "slmulti_sys.h"

namespace sl{ namespace pyscript {

PyObject* PyGC::collectMethod_ = NULL;
PyObject* PyGC::set_debugMethod_ = NULL;
std::unordered_map<std::string, int> PyGC::tracingCountMap_;

uint32 PyGC::DEBUG_STATS = 0;
uint32 PyGC::DEBUG_COLLECTABLE = 0;
uint32 PyGC::DEBUG_UNCOLLECTABLE = 0;
uint32 PyGC::DEBUG_SAVEALL = 0;
uint32 PyGC::DEBUG_LEAK = 0;
	
bool PyGC::isInit = false;


//-------------------------------------------------------------------------------------
bool PyGC::initialize(void)
{
	if(isInit)
		return true;
	
	PyObject* gcModule = PyImport_ImportModule("gc");

	if(gcModule)
	{
		collectMethod_ = PyObject_GetAttrString(gcModule, "collect");
		if (!collectMethod_)
		{
			ECHO_ERROR("PyGC::initialize: get collect is error!\n");
			PyErr_PrintEx(0);
		}

		set_debugMethod_ = PyObject_GetAttrString(gcModule, "set_debug");
		if(!set_debugMethod_)
		{
			ECHO_ERROR("PyGC::init: get set_debug is error!\n");
			PyErr_PrintEx(0);
		}

		PyObject* flag = NULL;
		
		flag = PyObject_GetAttrString(gcModule, "DEBUG_STATS");
		if(!flag)
		{
			ECHO_ERROR("PyGC::init: get DEBUG_STATS is error!\n");
			PyErr_PrintEx(0);
		}
		else
		{
			DEBUG_STATS = PyLong_AsLong(flag);
			Py_DECREF(flag);
			flag = NULL;
		}
		
		flag = PyObject_GetAttrString(gcModule, "DEBUG_COLLECTABLE");
		if(!flag)
		{
			ECHO_ERROR("PyGC::init: get DEBUG_COLLECTABLE is error!\n");
			PyErr_PrintEx(0);
		}
		else
		{
			DEBUG_COLLECTABLE = PyLong_AsLong(flag);
			Py_DECREF(flag);
			flag = NULL;
		}
		
		flag = PyObject_GetAttrString(gcModule, "DEBUG_UNCOLLECTABLE");
		if(!flag)
		{
			ECHO_ERROR("PyGC::init: get DEBUG_UNCOLLECTABLE is error!\n");
			PyErr_PrintEx(0);
		}
		else
		{
			DEBUG_UNCOLLECTABLE = PyLong_AsLong(flag);
			Py_DECREF(flag);
			flag = NULL;
		}
		
		flag = PyObject_GetAttrString(gcModule, "DEBUG_SAVEALL");
		if(!flag)
		{
			ECHO_ERROR("PyGC::init: get DEBUG_SAVEALL is error!\n");
			PyErr_PrintEx(0);
		}
		else
		{
			DEBUG_SAVEALL = PyLong_AsLong(flag);
			Py_DECREF(flag);
			flag = NULL;
		}

		flag = PyObject_GetAttrString(gcModule, "DEBUG_LEAK");
		if(!flag)
		{
			ECHO_ERROR("PyGC::init: get DEBUG_LEAK is error!\n");
			PyErr_PrintEx(0);
		}
		else
		{
			DEBUG_LEAK = PyLong_AsLong(flag);
			Py_DECREF(flag);
			flag = NULL;
		}

		APPEND_SCRIPT_MODULE_METHOD(gcModule, debugTracing,	__py_debugTracing,	METH_VARARGS, 0);

		Py_DECREF(gcModule);
	}
	else
	{
		ECHO_ERROR("PyGC::initialize: can't import gc!\n");
		PyErr_PrintEx(0);
	}
	
	isInit = collectMethod_ && set_debugMethod_;
	return isInit;
}

//-------------------------------------------------------------------------------------
void PyGC::finalise(void)
{
	Py_XDECREF(collectMethod_);
	Py_XDECREF(set_debugMethod_);
	
	collectMethod_ = NULL;
	set_debugMethod_ = NULL;	
}

//-------------------------------------------------------------------------------------
void PyGC::collect(int8 generations)
{
	PyObject* pyRet = NULL;
		
	if(generations != -1)
	{
		pyRet = PyObject_CallFunction(collectMethod_, 
			const_cast<char*>("(i)"), generations);
	}
	else
	{
		pyRet = PyObject_CallFunction(collectMethod_, 
			const_cast<char*>(""));
	}
	
	SCRIPT_ERROR_CHECK();
	
	if(pyRet)
	{
		S_RELEASE(pyRet);
	}
}

//-------------------------------------------------------------------------------------
void PyGC::set_debug(uint32 flags)
{
	PyObject* pyRet = PyObject_CallFunction(set_debugMethod_, 
		const_cast<char*>("i"), flags);
	
	SCRIPT_ERROR_CHECK();
	
	if(pyRet)
	{
		S_RELEASE(pyRet);
	}
}

//-------------------------------------------------------------------------------------
void PyGC::incTracing(std::string name)
{
	std::unordered_map<std::string, int>::iterator iter = tracingCountMap_.find(name);
	if(iter == tracingCountMap_.end())
	{
		tracingCountMap_[name] = 0;
		iter = tracingCountMap_.find(name);
	}

	iter->second = iter->second + 1;
}

//-------------------------------------------------------------------------------------
void PyGC::decTracing(std::string name)
{
	std::unordered_map<std::string, int>::iterator iter = tracingCountMap_.find(name);
	if(iter == tracingCountMap_.end())
	{
		return;
	}

	iter->second = iter->second - 1;
	SLASSERT(iter->second >= 0, "wtf");
}

//-------------------------------------------------------------------------------------
void PyGC::debugTracing(bool shuttingdown)
{
	std::unordered_map<std::string, int>::iterator iter = tracingCountMap_.begin();
	for(; iter != tracingCountMap_.end(); ++iter)
	{
		if(shuttingdown)
		{
			if(iter->second == 0)
				continue;

			ECHO_ERROR("PyGC::debugTracing(): %s : leaked(%d)\n", iter->first.c_str(), iter->second);
		}
		else
		{
			//Script::getSingleton().pyStdouterr()->pyPrint(fmt::format("PyGC::debugTracing(): {} : {}", iter->first, iter->second));
		}
	}
}

//-------------------------------------------------------------------------------------
PyObject* PyGC::__py_debugTracing(PyObject* self, PyObject* args)
{
	debugTracing(false);
	S_Return;
}

//-------------------------------------------------------------------------------------

}
}
