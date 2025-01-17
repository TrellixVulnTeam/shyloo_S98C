#include "Gate.h"
#include "IAgent.h"
#include "NodeDefine.h"
#include "NodeProtocol.h"
#include "ICacheDB.h"
#include "IIdMgr.h"
#include "DBDef.h"
#include "IRoleMgr.h"
#include "ProtocolID.pb.h"
#include "Protocol.pb.h"
#include <unordered_set>
#include "sltime.h"
#include "slxml_reader.h"
#include "slstring_utils.h"
#include <algorithm>
#include "IDebugHelper.h"

#define REPORT_LOAD_INTERVAL 1000
#define TICKET_EXPIRE_TIME 10000

class GateCBMessageHandler : public IGateMessageHandler{
public:
	GateCBMessageHandler(const GATE_CB cb) : _cb(cb){}
	virtual ~GateCBMessageHandler() {}

	virtual void DealNodeMessage(sl::api::IKernel* pKernel, const int64 id, const char* pContext, const int32 size){
		_cb(pKernel, id, pContext, size);
	}
private:
	GATE_CB		_cb;
};


class GateArgsCBMessageHandler : public IGateMessageHandler{
public:
	GateArgsCBMessageHandler(const GATE_ARGS_CB cb) : _cb(cb){}
	virtual ~GateArgsCBMessageHandler() {}

	virtual void DealNodeMessage(sl::api::IKernel* pKernel, const int64 id, const char* pContext, const int32 size){
		sl::OBStream args(pContext, size);
		_cb(pKernel, id, args);
	}
private:
	GATE_ARGS_CB _cb;
};

class DelaySendTimer : public sl::api::ITimer{
public:
	DelaySendTimer(const void* context, const int32 size) { _data.assign((const char*)context, size); }
	virtual ~DelaySendTimer(){}
	
	virtual void onStart(sl::api::IKernel* pKernel, int64 timetick){}
	virtual void onTime(sl::api::IKernel* pKernel, int64 timetick){
		if (_agents.empty())
			Gate::getInstance()->broadcast(_data.data(), (int32)_data.size());
		else{
			for (auto agent : _agents){
				Gate::getInstance()->send(agent, _data.data(), (int32)_data.size());
			}
		}

	}
	virtual void onTerminate(sl::api::IKernel* pKernel, bool beForced, int64 timetick){ DEL this; }
	virtual void onPause(sl::api::IKernel* pKernel, int64 timetick){}
	virtual void onResume(sl::api::IKernel* pKernel, int64 timetick){}

	void addAgent(const int64 id){ _agents.insert(id); }
private:
	std::string	_data;
	std::unordered_set<int64> _agents;
};

bool Gate::initialize(sl::api::IKernel * pKernel){
	_self = this;
    _kernel = pKernel;
	_isQueneOn = false;

	return true;
}

bool Gate::launched(sl::api::IKernel * pKernel){
	FIND_MODULE(_harbor, Harbor);
	//FIND_MODULE(_cacheDB, CacheDB);
	FIND_MODULE(_IdMgr, IdMgr);
	FIND_MODULE(_agent, Agent);

	_agent->setListener(this);
	_harbor->addNodeListener(this);

	//RGS_NODE_ARGS_HANDLER(_harbor, NodeProtocol::SCENEMGR_MSG_DISTRIBUTE_LOGIC_ACK, Gate::onSceneMgrDistributeLogic);
	RGS_NODE_HANDLER(_harbor, NodeProtocol::ACCOUNT_MSG_BIND_ACCOUNT_ACK, Gate::onAccountBindAccountAck);
	RGS_NODE_HANDLER(_harbor, NodeProtocol::ACCOUNT_MSG_KICK_FROM_ACCOUNT, Gate::onAccountKickFromAccount);
	//RGS_NODE_ARGS_HANDLER(_harbor, NodeProtocol::LOGIC_MSG_BIND_PLAYER_ACK, Gate::onLogicBindPlayerAck);
	RGS_NODE_HANDLER(_harbor, NodeProtocol::BALANCE_MSG_SYNC_LOGIN_TICKET, Gate::onBalanceSyncLoginTicket);
	//RGS_NODE_HANDLER(_harbor, NodeProtocol::LOGIC_MSG_TRANSFOR, Gate::onLogicTransforToAgent);
	//RGS_NODE_HANDLER(_harbor, NodeProtocol::LOGIC_MSG_BROCAST, Gate::onLogicBrocastToAgents);
	//RGS_NODE_HANDLER(_harbor, NodeProtocol::GATE_MSG_BROCAST, Gate::onLogicBrocastToAgents);
	//RGS_NODE_HANDLER(_harbor, NodeProtocol::LOGIC_MSG_ALLSVR_BROCAST, Gate::onLogicBrocastToAllAgents);

	//_self->rgsAgentMessageHandler(ClientMsgID::CLIENT_MSG_LOGIN_REQ, &Gate::onClientLoginReq);
	//_self->rgsAgentMessageHandler(ClientMsgID::CLIENT_MSG_SELECT_ROLE_REQ, &Gate::onClientSelectRoleReq);
	//_self->rgsAgentMessageHandler(ClientMsgID::CLIENT_MSG_CREATE_ROLE_REQ, &Gate::onClientCreateRoleReq);
	RGS_GATE_ARGS_HANDLER(_self, ClientMsgID::CLIENT_MSG_LOGIN_REQ, Gate::onClientLoginReq);
	//RGS_GATE_ARGS_HANDLER(_self, ClientMsgID::CLIENT_MSG_SELECT_ROLE_REQ, Gate::onClientSelectRoleReq);
	//RGS_GATE_ARGS_HANDLER(_self, ClientMsgID::CLIENT_MSG_CREATE_ROLE_REQ, Gate::onClientCreateRoleReq);


	if (_isQueneOn)
		START_TIMER(_self, 20000, TIMER_BEAT_FOREVER, REPORT_LOAD_INTERVAL);
	
	//test();
	return true;
}
bool Gate::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

void Gate::onOpen(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const char* ip, const int32 port){
	if (nodeType == NodeType::BALANCE){
		sl::BStream<256> args;
		args << pKernel->getInternetIp();
		args << sl::CStringUtils::StringAsInt32(pKernel->getCmdArg("agent"));
		_harbor->send(NodeType::BALANCE, 1, NodeProtocol::GATE_MSG_GATE_REGISTER, args.out());
	}
}

void Gate::onClose(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId){
	if (nodeType == NodeType::DATABASE){
		std::unordered_map<int64, Session> temp(_sessions);
		for (auto itor = temp.begin(); itor != temp.end(); ++itor){
			_agent->kick(itor->first);
		}
	}
	/*else if (nodeType == NodeType::LOGIC){
		std::unordered_set<int64> temp(_logicPlayers[nodeId]);
		for (auto agentId : temp){
			_agent->kick(agentId);
		}
	}*/
}

void Gate::onTime(sl::api::IKernel* pKernel, int64 timetick){
	sl::BStream<32> args;
	args << (int32)_sessions.size();
	_harbor->send(NodeType::BALANCE, 1, NodeProtocol::GATE_MSG_LOAD_REPORT, args.out());

	auto itor = _agentTickets.begin();
	while (itor != _agentTickets.end()){
		auto curr = itor;
		++itor;

		if (sl::getTimeMilliSecond() - curr->second > TICKET_EXPIRE_TIME)
			_agentTickets.erase(curr);
	}
}

void Gate::onAgentOpen(sl::api::IKernel* pKernel, const int64 id){
	SLASSERT(_sessions.find(id) == _sessions.end(), "duplicate agent id��%d", id);
	_sessions[id] = { id, 0, GATE_STATE_NONE };
}

void Gate::onAgentClose(sl::api::IKernel* pKernel, const int64 id){
	SLASSERT(_sessions.find(id) != _sessions.end(), "where is agent %d", id);

	if (_sessions[id].state != GATE_STATE_NONE){
		reset(pKernel, id, GATE_STATE_NONE);
	}

	_sessions.erase(id);
}

int32 Gate::onAgentRecv(sl::api::IKernel* pKernel, const int64 id, const void* context, const int32 size){
	if (size < (int32)(sizeof(int32)* 2)){
		return 0;
	}



	int32 len = *((int32*)((const char*)context + sizeof(int32)));
	if (size < len){
		return 0;
	}

	int32 msgId = *((int32*)context);
	if (_gateProtos.find(msgId) != _gateProtos.end()){
		_gateProtos[msgId]->DealNodeMessage(pKernel, id, (const char*)context + sizeof(int32)* 2, len);
	}
	else{
		if (_sessions[id].state == GATE_STATE_ONLINE){
			transMsgToLogic(pKernel, id, context, len);
		}
	}
	return len;
}

void Gate::rgsGateMessageHandler(int32 messageId, const GATE_CB& handler, const char* debug){
	SLASSERT(_gateProtos.find(messageId) == _gateProtos.end(), "duplicate agent msg %d", messageId);
	_gateProtos[messageId] = NEW GateCBMessageHandler(handler);
}

void Gate::rgsGateArgsMessageHandler(int32 messageId, const GATE_ARGS_CB& handler, const char* debug){
	SLASSERT(_gateProtos.find(messageId) == _gateProtos.end(), "duplicate agent msg %d", messageId);
	_gateProtos[messageId] = NEW GateArgsCBMessageHandler(handler);
}

void Gate::rgsGateListener(IGateListener* listener){
	_gateListeners.push_back(listener);
}

void Gate::transMsgToLogic(sl::api::IKernel* pKernel, const int64 id, const void* pContext, const int32 size){
    for(auto* listener : _gateListeners)
        listener->onGateMsgRecv(pKernel, id, pContext, size);

}

void Gate::reset(sl::api::IKernel* pKernel, int64 id, int8 state){
	SLASSERT(state == GATE_STATE_NONE || state == GATE_STATE_BINDING, "wtf");
	Session& session = _sessions[id];

	int8 oldState = session.state;
	if (oldState > GATE_STATE_BINDING){
        for(auto* listener : _gateListeners)
            listener->onGateUnbind(pKernel, id);
	}

	if (oldState > GATE_STATE_NONE && state == GATE_STATE_NONE){
		sl::BStream<128> args;
		args << session.agentId << session.accountId;
		_harbor->send(NodeType::DATABASE, 1, NodeProtocol::GATE_MSG_UNBIND_ACCOUNT_REQ, args.out());

		session.accountId = 0;
	}

	session.state = state;
}

void Gate::broadcast(const void* context, const int32 size){
	auto itor = _sessions.begin();
	for (; itor != _sessions.end(); ++itor){
		if (itor->second.state == GATE_STATE_ONLINE)
			_agent->send(itor->second.agentId, context, size);
	}
}

void Gate::send(int64 agentId, const void* context, const int32 size){
	auto itor = _sessions.find(agentId);
	if (itor != _sessions.end()){
		if (itor->second.state == GATE_STATE_ONLINE)
			_agent->send(agentId, context, size);
	}
}

void Gate::sendToClient(const int64 id, const int32 msgId, const OBStream& buf){
	int32 header[2];
	header[0] = msgId;
	header[1] = buf.getSize() + sizeof(int32)* 2;

	_agent->send(id, header, sizeof(header));
	_agent->send(id, buf.getContext(), buf.getSize());
}

void Gate::sendLoginAck(sl::api::IKernel* pKernel, Session& session, int32 errCode){
	BStream<4096> buf;
	buf << (int32)errCode;
	sendToClient(session.agentId, ServerMsgID::SERVER_MSG_LOGIN_RSP, buf.out());
}

/*void Gate::onSceneMgrDistributeLogic(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const sl::OBStream& args){
	int64 agentId = args.getInt64(0);
	int64 actorId = args.getInt64(1);
	int32 logic = args.getInt32(2);

	if (_players.find(agentId) != _players.end()){
		if (logic > 0){
			Player& player = _players[agentId];
			SLASSERT(player.state == GATE_STATE_DISTRIBUTE, "wtf");

			player.selectActorId = actorId;
			player.logic = logic;
			player.state = GATE_STATE_BINDING;

			_actors[actorId] = agentId;
			_logicPlayers[logic].insert(agentId);

			IArgs<2, 32> args;
			args << actorId << player.accountId;
			args.fix();
			_harbor->send(NodeType::LOGIC, logic, NodeProtocol::GATE_MSG_BIND_PLAYER_REQ, args.out());
		}
		else{
			reset(pKernel, agentId, GATE_STATE_ROLELOADED);

			IBStream<128> buf;
			buf << (int32)protocol::ErrorCode::ERROR_DISTRIBUTE_LOGIC_FAILED;
			sendToClient(pKernel, agentId, ServerMsgID::SERVER_MSG_SELECT_ROLE_RSP, buf.out());
		}
	}
}*/

void Gate::onAccountBindAccountAck(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const sl::OBStream& args){
	int64 agentId = 0;
	int64 accountId = 0;
	int32 errorCode = 0;
	args >> agentId >> accountId >> errorCode;

	if (_sessions.find(agentId) != _sessions.end()){
		Session& session = _sessions[agentId];
		SLASSERT(session.state == GATE_STATE_AUTHENING && session.accountId == accountId, "wtf");
		if (session.accountId != accountId)
			return;

		if (errorCode == protocol::ErrorCode::ERROR_NO_ERROR){
		   	/*bool ret = _roleMgr->getRoleList(accountId, [&player](sl::api::IKernel* pKernel, const int64 actorId, IRole* role){
				player.roles.push_back({ actorId, role });
			});

			if (ret){
				sendLoginAck(pKernel, player, protocol::ErrorCode::ERROR_NO_ERROR);
				player.state = GATE_STATE_ROLELOADED;
			}
			else{
				reset(pKernel, agentId, GATE_STATE_NONE);
				sendLoginAck(pKernel, player, protocol::ErrorCode::ERROR_GET_ROLE_LIST_FAILED);
			}*/
         
           session.state = GATE_STATE_BINDING;
           
           for (auto* listener : _gateListeners)
              listener->onGateLogined(pKernel, agentId); 
           
		}
		else{
			sendLoginAck(pKernel, session, errorCode);
		}
	}
}

void Gate::onAccountKickFromAccount(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const sl::OBStream& args){
	int64 agentId = 0;
	args >> agentId;
	if (_sessions.find(agentId) != _sessions.end()){
		SLASSERT(_sessions[agentId].state >= GATE_STATE_BINDING, "wtf");

		//reset(pKernel, agentId, GATE_STATE_NONE);
        for (auto* listener : _gateListeners)
            listener->onGateLogOnAttempt(pKernel, agentId);
	}
}

void Gate::setGateOnline(const int64 agentId){
	if (_sessions.find(agentId) != _sessions.end()){
		Session& session = _sessions[agentId];
        if(session.state != GATE_STATE_BINDING){
            return;
        }
		session.state = GATE_STATE_ONLINE;
	}
}


/*void Gate::onLogicBindPlayerAck(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const sl::OBStream& args){
	int32 errCode = args.getInt32(0);
	int64 actorId = args.getInt64(1);
	int64 accountId = args.getInt64(2);
	
	if (_actors.find(actorId) != _actors.end()){
		SLASSERT(_players.find(_actors[actorId]) != _players.end(), "wtf");
		Player& player = _players[_actors[actorId]];
		SLASSERT(player.state == GATE_STATE_BINDING && actorId == player.selectActorId, "wtf");

		if (errCode == protocol::ErrorCode::ERROR_NO_ERROR){
			player.state = GATE_STATE_ONLINE;
			player.lastActorId = actorId;

			//update DB
			_cacheDB->write("account", false, [&](sl::api::IKernel* pKernel, ICacheDBContext* context){
				context->writeInt64("lastActorId", actorId);
			}, 1, accountId);
		}
		else{
			reset(pKernel, _actors[actorId], GATE_STATE_ROLELOADED);
		}
		IBStream<128> buf;
		buf << errCode;
		sendToClient(pKernel, player.agentId, ServerMsgID::SERVER_MSG_SELECT_ROLE_RSP, buf.out());
	}
}*/

void Gate::onBalanceSyncLoginTicket(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const sl::OBStream& args){
	int64 agentId = 0;
	args >> agentId;
	_agentTickets[agentId] = sl::getTimeMilliSecond();
}

void Gate::sendMsgToAgent(const int64 agentId , const void* context, const int32 size, const int32 delay){
    sl::api::IKernel* pKernel = _kernel;
	Session* session = findSessionByAgentId(agentId);
	if (session != nullptr && session->state == GATE_STATE_ONLINE){
		//const int32 messageId = ((client::Header*)((const char*)context + sizeof(client::Transfor)))->messageId;
		if (delay > 0){
			DelaySendTimer* timer = NEW DelaySendTimer(context, size);
			timer->addAgent(agentId);
			START_TIMER(timer, 0, 1, delay);
		}
		else{
			//send(info->actorId, context, size);
			_agent->send(session->agentId, (const char*)context, size);
		}
	}
	else{
		ERROR_LOG("send packet to client, but agentId[agentId:%lld] is invaild ", agentId);
	}
}

/*void Gate::onLogicTransforToAgent(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const OBStream& args){
	const void* context = args.getContext();
	const int32 size = args.getSize();
	SLASSERT(size > sizeof(client::Transfor) + sizeof(client::Header), "size is not invailed");
	client::Transfor* info = (client::Transfor*)context;
	int32 delay = info->delay * 100;

	Player* player = findPlayerByActorId(info->actorId);
	if (player != nullptr && player->state == GATE_STATE_ONLINE){
		//const int32 messageId = ((client::Header*)((const char*)context + sizeof(client::Transfor)))->messageId;
		if (delay > 0){
			DelaySendTimer* timer = NEW DelaySendTimer((const char*)context + sizeof(client::Transfor), size - sizeof(client::Transfor));
			timer->addActor(info->actorId);
			START_TIMER(timer, 0, 1, delay);
		}
		else{
			//send(info->actorId, context, size);
			_agent->send(player->agentId, (const char*)context + sizeof(client::Transfor), size - sizeof(client::Transfor));
		}
	}
	else{
		ERROR_LOG("send packet to client, but player[actorid:%lld] is invaild ", info->actorId);
	}
}
*/

/*void Gate::onLogicBrocastToAgents(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const OBStream& args){
	const void* context = args.getContext();
	const int32 size = args.getSize();
	SLASSERT(size > sizeof(client::Header), "size is invaild");
	client::Header* header = (client::Header*)context;
	//const int32 messageId = header->messageId;
	const int32 packetSize = header->size;

	int32 remainSize = size - packetSize;
	const char* buff = (const char*)context + packetSize;
	while (remainSize > (int32)sizeof(client::Brocast)){
		client::Brocast* info = (client::Brocast*)buff;
		int32 delay = info->delay * 100;
		int32 gate = info->gate;
		int32 count = info->count;
		int32 needSize = sizeof(client::Brocast) + sizeof(int64)* count;
		SLASSERT(remainSize >= needSize, "invalid message packet");

		if (count > 0){
			if (gate == _harbor->getNodeId()){
				int64* actors = (int64*)(buff + sizeof(client::Brocast));
				if (delay > 0){
					DelaySendTimer* timer = NEW DelaySendTimer(context, packetSize);
					for (int32 i = 0; i < count; i++){
						timer->addActor(actors[i]);
					}
					START_TIMER(timer, 0, 1, delay);
				}
				else{
					for (int32 i = 0; i < count; i++){
						Player* player = findPlayerByActorId(actors[i]);
						if (nullptr != player && player->state == GATE_STATE_ONLINE){
							_agent->send(player->agentId, context, packetSize);
						}
					}
				}
			}
			else{
				_harbor->prepareSend(NodeType::GATE, gate, NodeProtocol::GATE_MSG_BROCAST, packetSize + needSize);
				_harbor->send(NodeType::GATE, gate, context, packetSize);
				_harbor->send(NodeType::GATE, gate, buff, needSize);
			}
		}

		buff += needSize;
		remainSize -= needSize;
	}
}

void Gate::onLogicBrocastToAllAgents(sl::api::IKernel* pKernel, const int32 nodeType, const int32 nodeId, const OBStream& args){
	const void* context = args.getContext();
	const int32 size = args.getSize();
	SLASSERT(size > sizeof(int8) + sizeof(client::Header), "size is invaild");
	int32 delay = (*(int8*)context) * 100;
	if (delay > 0){
		DelaySendTimer* timer = NEW DelaySendTimer((const char*)context + sizeof(int8), size - sizeof(int8));
		START_TIMER(timer, 0, 1, delay);
	}
	else{
		forEach([&](Player& player){
			if (player.state == GATE_STATE_ONLINE){
				_agent->send(player.agentId, (const char*)context + sizeof(int8), size - sizeof(int8));
			}
		});
	}
}
*/
void Gate::onClientLoginReq(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	const char* accountName = nullptr;
	int64 ticket = 0;
    const char* password = nullptr;
	if (!args.readString(accountName) || !args.readString(password))
		return;

	SLASSERT(_sessions.find(id) != _sessions.end(), "not find agent!");
	Session& session = _sessions[id];

	if (session.state != GATE_STATE_NONE)
		return;

	if (_isQueneOn){
		if (_agentTickets.find(ticket) == _agentTickets.end()){
			sendLoginAck(pKernel, session, protocol::ErrorCode::ERROR_LOGIN_CHECK_TICKET_FAILED);
			return;
		}
		_agentTickets.erase(ticket);
	}

	int64 accountId = 34465756878;
	/*bool success = _cacheDB->readByIndex("account", [&](sl::api::IKernel* pKernel, ICacheDBReader* reader){
		reader->readColumn("accountId");
	}, [&accountId](sl::api::IKernel* pKernel, ICacheDBReadResult* result){
		if (result->count() == 1)
			accountId = result->getInt64(0, 0);
	}, accountName);
	SLASSERT(success, "read cacheDB failed");

	if (!accountId){
		accountId = (int64)_IdMgr->allocID();
		success = _cacheDB->write("account", true, [&](sl::api::IKernel* pKernel, ICacheDBContext* context){
			context->writeString("username", accountName);
		}, 1, accountId);
		SLASSERT(success, "write db failed");
	}
	SLASSERT(accountId, "can't get accountID");
*/
	session.accountId = accountId;
	session.state = GATE_STATE_AUTHENING;

    for (auto* listener : _gateListeners)
        listener->onGateLogined(pKernel, session.agentId); 

	/*IArgs<3, 128> inArgs;
	inArgs << session.agentId << session.accountId;
	inArgs.fix();

	_harbor->send(NodeType::DATABASE, 1, NodeProtocol::GATE_MSG_BIND_ACCOUNT_REQ, inArgs.out());
*/
	printf("client[%s:%lld] login...\n", accountName, accountId);
}

/*void Gate::onClientSelectRoleReq(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	int64 actorId = 0;
	if (!args.readInt64(actorId))
		return;

	SLASSERT(_players.find(id) != _players.end(), "not find agent");
	Player& player = _players[id];

	if (player.state == GATE_STATE_ROLELOADED){
		if (std::find_if(player.roles.begin(), player.roles.end(), [actorId](const Role& role){
			return role.actorId == actorId;
		}) == player.roles.end()){
			return;
		}

		player.state = GATE_STATE_DISTRIBUTE;

		IArgs<2, 32> args;
		args << id << actorId;
		args.fix();
		_harbor->send(NodeType::SCENEMGR, 1, NodeProtocol::GATE_MSG_DISTRIBUTE_LOGIC_REQ, args.out());

		ECHO_TRACE("client[%lld] select role[%lld] success", _players[id].accountId, actorId);
	}
}*/

/*void Gate::onClientCreateRoleReq(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	SLASSERT(_players.find(id) != _players.end(), "where is agent?");
	Player& player = _players[id];
	if (player.state == GATE_STATE_ROLELOADED){
		if ((int32)player.roles.size() >= _maxRoleNum){
			IBStream<128> buf;
			buf << protocol::ErrorCode::ERROR_TOO_MUCH_ROLE;
			sendToClient(pKernel, id, ServerMsgID::SERVER_MSG_CREATE_ROLE_RSP, buf.out());
			return;
		}

		
		IRole* role = _roleMgr->createRole(player.accountId, args);
		if (role){
			player.roles.push_back({ role->getRoleId(), role });

			sl::IBStream<128> rsp;
			rsp << protocol::ErrorCode::ERROR_NO_ERROR << role->getRoleId();
			role->pack();
			sendToClient(pKernel, id, ServerMsgID::SERVER_MSG_CREATE_ROLE_RSP, rsp.out());
		}
		else{
			IBStream<128> buf;
			buf << protocol::ErrorCode::ERROR_CREATE_ROLE_FAILED;
			sendToClient(pKernel, id, ServerMsgID::SERVER_MSG_CREATE_ROLE_RSP, buf.out());
			return;
		}
	}
}*/

