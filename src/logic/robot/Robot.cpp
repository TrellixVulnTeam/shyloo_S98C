#include "Robot.h"
#include "slxml_reader.h"
#include "slbinary_stream.h"
#include "ProtocolID.pb.h"
#include "Protocol.pb.h"
#include "IDCCenter.h"
#include <time.h>
bool Robot::initialize(sl::api::IKernel * pKernel){
	_kernel = pKernel;
	_self = this;

	return true;
}

bool Robot::launched(sl::api::IKernel * pKernel){
	FIND_MODULE(_client, Client);
	
	//_self->rgsSvrMessageHandler(AgentProtocol::CLIENT_MSG_LOGIN_REQ, &Robot::onClientLoginReq);
	//_self->rgsSvrMessageHandler(AgentProtocol::CLIENT_MSG_SELECT_ROLE_REQ, &Robot::onClientSelectRoleReq);
	_self->rgsSvrMessageHandler(ServerMsgID::SERVER_MSG_LOGIN_RSP, &Robot::onServerLoginAck);
	_self->rgsSvrMessageHandler(ServerMsgID::SERVER_MSG_SELECT_ROLE_RSP, &Robot::onServerSelectRoleAck);
	_self->rgsSvrMessageHandler(ServerMsgID::SERVER_MSG_ATTRIB_SYNC, &Robot::onServerAttribSync);
	_self->rgsSvrMessageHandler(ServerMsgID::SERVER_MSG_CREATE_ROLE_RSP, &Robot::onServerCreateRoleAck);
	
	_client->setListener(this);

	char path[256] = { 0 };
	SafeSprintf(path, sizeof(path), "%s/robot.xml", pKernel->getEnvirPath());
	sl::XmlReader conf;
	if (!conf.loadXml(path)){
		SLASSERT(false, "can not load file %s", pKernel->getEnvirPath());
		return false;
	}

	_robotCount = conf.root().getAttributeInt32("count");
	_svrIp = conf.root()["svr"][0].getAttributeString("ip");
	_svrPort = conf.root()["svr"][0].getAttributeInt32("port");

	START_TIMER(_self, 0, _robotCount, 2000);
	
	return true;
}

bool Robot::destory(sl::api::IKernel * pKernel){
	DEL this;
	return true;
}

void Robot::onAgentOpen(sl::api::IKernel* pKernel, const int64 id){
	SLASSERT(_robots.find(id) == _robots.end(), "duplicate agent id��%d", id);
	static char accountName[3][64] = { "ddc", "wrd", "ds" };
	int32 nodeId = sl::CStringUtils::StringAsInt32(pKernel->getCmdArg("node_id"));
	_robots[id] = { id, accountName[nodeId -1] };

	IBStream<64> args;
	args << _robots[id].name.c_str();
	sendToSvr(pKernel, id, ClientMsgID::CLIENT_MSG_LOGIN_REQ, args.out());
}

void Robot::onAgentClose(sl::api::IKernel* pKernel, const int64 id){
	SLASSERT(_robots.find(id) != _robots.end(), "where is agent %d", id);
	_robots.erase(id);
}

int32 Robot::onAgentRecv(sl::api::IKernel* pKernel, const int64 id, const void* context, const int32 size){
	if (size < sizeof(int32)* 2){
		return 0;
	}

	int32 len = *((int32*)((const char*)context + sizeof(int32)));
	if (size < len){
		return 0;
	}

	int32 msgId = *((int32*)context);
	if (_svrProtos.find(msgId) != _svrProtos.end()){
		OBStream buf((const char*)context + sizeof(int32)* 2, len);
		(this->*_svrProtos[msgId])(pKernel, id, buf);
	}
	else{
		//SLASSERT(false, "can not find message id[%d]", msgId);
	}
	return len;
}

void Robot::rgsSvrMessageHandler(int32 messageId, svr_args_cb handler){
	SLASSERT(_svrProtos.find(messageId) == _svrProtos.end(), "duplicate agent msg %d", messageId);
	
	_svrProtos[messageId] = handler;
}

void Robot::onTime(sl::api::IKernel* pKernel, int64 timetick){
	_client->connect(_svrIp.c_str(), _svrPort);
}

void Robot::sendToSvr(sl::api::IKernel* pKernel, const int64 id, const int32 msgId, const OBStream& buf){
	int32 header[2];
	header[0] = msgId;
	header[1] = buf.getSize() + sizeof(int32)* 2;

	_client->send(id, header, sizeof(header));
	_client->send(id, buf.getContext(), buf.getSize());
	ECHO_ERROR("send msg[%d] to svr", msgId);
}

void Robot::onServerLoginAck(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	int32 errCode = protocol::ErrorCode::ERROR_NO_ERROR;
	if (!args.readInt32(errCode) || errCode != protocol::ErrorCode::ERROR_NO_ERROR){
		SLASSERT(false, "login ack failed");
		return;
	}

	int32 roleCount = 0;
	if (!args.readInt32(roleCount)){
		SLASSERT(false, "wtf");
		return;
	}

	if (roleCount <= 0){
		static char roleName[3][64] = { "ddcddd", "wrdzzz", "dsrrr" };
		int32 nodeId = sl::CStringUtils::StringAsInt32(pKernel->getCmdArg("node_id"));
		IBStream<256> ask;
		ask << roleName[nodeId-1] << (int8)1 << (int8)1;
		sendToSvr(pKernel, id, ClientMsgID::CLIENT_MSG_CREATE_ROLE_REQ, ask.out());
	}
	else{
		int64 actorId = 0;
		if (!args.readInt64(actorId)){
			SLASSERT(false, "wtf");
			return;
		}

		IBStream<64> ask;
		ask << actorId;
		sendToSvr(pKernel, id, ClientMsgID::CLIENT_MSG_SELECT_ROLE_REQ, ask.out());
	}

}

void Robot::onServerCreateRoleAck(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	int32 errCode = 0;
	int64 actorId = 0;
	if (!args.readInt32(errCode) || !args.readInt64(actorId))
		return;

	if (errCode == 0){
		IBStream<64> ask;
		ask << actorId;
		sendToSvr(pKernel, id, ClientMsgID::CLIENT_MSG_SELECT_ROLE_REQ, ask.out());
	}
}

void Robot::onServerSelectRoleAck(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	int32 errCode = 0;
	if (!args.readInt32(errCode))
		return;

	if (errCode == 0)
		test(pKernel, id);
}

void Robot::onServerAttribSync(sl::api::IKernel* pKernel, const int64 id, const OBStream& args){
	ECHO_ERROR("start sync attrib...");
	int64 actorId = 0; 
	int32 propCount = 0; 
	if (!args.readInt64(actorId) || !args.readInt32(propCount))
		return;

	for (int32 i = 0; i < propCount; i++){
		int16 idAndType = 0;
		args.readInt16(idAndType);
		int32 idx = ((idAndType >> 3) & 0x1fff);
		int32 type = (int32)(idAndType & 0x07);
		//if (type == DTYPE_STRING){
			const char* str = nullptr;
			args.readString(str);
			ECHO_ERROR("name %s", str);
		//}

	}
}

void Robot::test(sl::api::IKernel* pKernel, const int64 id){
	IBStream<64> ask;
	sendToSvr(pKernel, id, ClientMsgID::CLIENT_MSG_TEST, ask.out());
}

void Robot::genRandomString(char* str, const int32 size){
	int flag, i;
	srand((unsigned)time(NULL));
	for (i = 0; i < size - 1; i++){
		flag = rand() % 3;
		switch (flag){
		case 0:
			str[i] = 'A' + rand() % 26;
			break;
		case 1:
			str[i] = 'a' + rand() % 26;
			break;
		case 2:
			str[i] = '0' + rand() % 10;
			break;
		default:
			str[i] = 'x';
			break;
		}
	}
	str[size - 1] = '\0';
}