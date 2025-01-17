#ifndef __GAME_DEFINE_H__
#define __GAME_DEFINE_H__

#include "NodeDefine.h"

enum game{
	NODE_INVALID_ID = -1,
	MAX_PROP_NAME_LEN = 64,
	MAX_PATH_LEN = 256,
	MAX_CAPACITY_LOAD = 1000,
	MAX_ROLE_NAME_LEN = 64,
	INVAILD_GATE_NODE_ID = -1,
	MAX_PACKET_SIZE = 65536,
	MAX_DEBUG_INFO_SIZE = 256,
	MAX_SCENE_LEN = 64,
	MAX_PROP_VALUE_LEN = 64,
	MAX_OBJECT_TYPE_LEN = 64,
	MAX_IP_LEN = 128,
	MAX_TOKEN_LEN = 128,
};

enum logFilter{
	EFatal = 0x1,
	EError = 0x2,
	EWarning = 0x4,
	EInfo = 0x8,
	EDebug = 0x10,
	ETrace = 0x20,
};

enum ShutdownStep{
	SHUTDOWN_PRE_MSG = 0,
	SHUTDOWN_GATE_CLOSE,
	SHUTDOWN_OTHER_CLUSTER_CLOSE,
	SHUTDOWN_LOGIC_NOTIFY,
	SHUTDOWN_LOGIC_CLOSE,
	SHUTDOWN_DB_NOTIFY,
	SHUTDOWN_DB_CLOSE,
	SHUTDOWN_SLAVE_CLOSE
};

enum EntityMailBoxType{
    MAILBOX_TYPE_CELL = 0,
    MAILBOX_TYPE_BASE = 1,
    MAILBOX_TYPE_CLIENT = 2,
    MAILBOX_TYPE_CELL_VIA_BASE = 3,
    MAILBOX_TYPE_BASE_VIA_CELL = 4,
    MAILBOX_TYPE_CLIENT_VIA_CELL = 5,
    MAILBOX_TYPE_CLIENT_VIA_BASE = 6,
};

const int32 ENTITY_MAILBOX_NODE_TYPE_MAPPING[] = {
	NodeType::SCENE,
	NodeType::LOGIC,
	NodeType::CLIENT,
	NodeType::SCENE,
	NodeType::LOGIC,
	NodeType::CLIENT,
	NodeType::CLIENT,
};

const char ENTITY_MAILBOX_NAME_MAPPING[][8] = {
	"cell",
	"base",
	"client",
	"cell",
	"base",
	"client",
	"client",
};

enum ServerState{
	S_NONE = -1,
	S_INIT = 0,
	S_READY,
	S_READY_FOR_LOGIN,
	S_RUNNING,
	S_PRE_SHUTDOWN,
	S_SHUTDOWN,
};

#endif
