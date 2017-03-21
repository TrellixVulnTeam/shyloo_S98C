#ifndef SL_NODE_PROTOCOL_H
#define SL_NODE_PROTOCOL_H
enum NodeProtocol{
	ASK_FOR_ALLOC_ID_AREA = 1,
	GIVE_ID_AREA = 2,
	DB_COMMAND_EXECUTE = 3,
	MASTER_MSG_START_NODE = 4,
	MASTER_MSG_NEW_NODE = 5,
	CLUSTER_MSG_NEW_NODE_COMING = 6,
	NODE_CAPACITY_LOAD_REPORT = 7,

	GATE_MSG_TRANSMIT_MSG_TO_LOGIC = 20,
	GATE_MSG_BIND_ACCOUNT_REQ = 21,
	GATE_MSG_DISTRIBUTE_LOGIC_REQ = 22,
	GATE_MSG_BIND_PLAYER_REQ = 23,
	GATE_MSG_UNBIND_PLAYER_REQ = 24,
	GATE_MSG_UNBIND_ACCOUNT_REQ = 25,

	SCENEMGR_MSG_DISTRIBUTE_LOGIC_ACK = 1000,





};
#endif