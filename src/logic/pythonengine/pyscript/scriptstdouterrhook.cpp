#include "scriptstdouterrhook.h"
#include "slstring_utils.h"
namespace sl{ namespace pyscript{
								
//-------------------------------------------------------------------------------------
ScriptStdOutErrHook::ScriptStdOutErrHook():
isPrint_(true)
{
}

//-------------------------------------------------------------------------------------
ScriptStdOutErrHook::~ScriptStdOutErrHook()
{
}

//-------------------------------------------------------------------------------------
void ScriptStdOutErrHook::info_msg(const wchar_t* msg, uint32 msglen)
{
	if(isPrint_)
		ScriptStdOutErr::info_msg(msg, msglen);

	std::wstring str;
	str.assign(msg, msglen);
	wbuffer_ += str;

	if(msg[0] == L'\n')
	{
		if(buffer_)
		{
			std::string out;
			sl::CStringUtils::wchar2utf8(wbuffer_, out);		
			(*buffer_) += out;
			wbuffer_ = L"";
		}
	}
}

//-------------------------------------------------------------------------------------
void ScriptStdOutErrHook::error_msg(const wchar_t* msg, uint32 msglen)
{
	if(isPrint_)
		ScriptStdOutErr::error_msg(msg, msglen);

	std::wstring str;
	str.assign(msg, msglen);
	wbuffer_ += str;

	if(msg[0] == L'\n')
	{
		if(buffer_)
		{
			std::string out;
			sl::CStringUtils::wchar2utf8(wbuffer_, out);		
			(*buffer_) += out;
			wbuffer_ = L"";
		}
	}
}

//-------------------------------------------------------------------------------------

}
}
