#ifndef SL_LOGIC_SLAVE_H
#define SL_LOGIC_SLAVE_H
#include "slikernel.h"
#include "slimodule.h"
#include <unordered_map>
class IHarbor;
class OArgs;
class Slave :public sl::api::IModule
{
public:
	struct CMD_INFO{
		char cmd[256];
	};

	struct EXECUTE_INFO{
		char name[64];
		char cmd[256];
	};

	virtual bool initialize(sl::api::IKernel * pKernel);
	virtual bool launched(sl::api::IKernel * pKernel);
	virtual bool destory(sl::api::IKernel * pKernel);

	static void openNewNode(sl::api::IKernel* pKernel, int32 nodeType, int32 nodeId, const OArgs& args);
	static void startNewNode(sl::api::IKernel* pKernel, const char* name, const char* cmd, const int32 nodeType, const int32 nodeId);
	static int32 startNode(sl::api::IKernel* pKernel, const char* cmd);

private:
	static IHarbor* s_harbor;

	static int32	s_startPort;
	static int32	s_endPort;
	static int32	s_startOutPort;
	static int32	s_endOutPort;
	static std::unordered_map<int64, CMD_INFO> s_cmds;
	static std::unordered_map<int32, EXECUTE_INFO> s_executes;
};

#endif