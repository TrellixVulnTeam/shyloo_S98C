// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ProtocolID.proto

#ifndef PROTOBUF_ProtocolID_2eproto__INCLUDED
#define PROTOBUF_ProtocolID_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3003000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3003000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)

namespace protobuf_ProtocolID_2eproto {
// Internal implementation detail -- do not call these.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[];
  static const ::google::protobuf::uint32 offsets[];
  static void InitDefaultsImpl();
  static void Shutdown();
};
void AddDescriptors();
void InitDefaults();
}  // namespace protobuf_ProtocolID_2eproto

enum ClientMsgID {
  CLIENT_MSG_LOGIN_REQ = 1,
  CLIENT_MSG_SELECT_ROLE_REQ = 2,
  CLIENT_MSG_CREATE_ROLE_REQ = 3,
  CLIENT_MSG_ENTER_SCENE_REQ = 4,
  CLIENT_MSG_TEST = 5,
  CLIENT_MSG_GD_COMMAND = 6,
  CLIENT_MSG_REMOTE_METHOD_CALL = 7
};
bool ClientMsgID_IsValid(int value);
const ClientMsgID ClientMsgID_MIN = CLIENT_MSG_LOGIN_REQ;
const ClientMsgID ClientMsgID_MAX = CLIENT_MSG_GD_COMMAND;
const int ClientMsgID_ARRAYSIZE = ClientMsgID_MAX + 1;

const ::google::protobuf::EnumDescriptor* ClientMsgID_descriptor();
inline const ::std::string& ClientMsgID_Name(ClientMsgID value) {
  return ::google::protobuf::internal::NameOfEnum(
    ClientMsgID_descriptor(), value);
}
inline bool ClientMsgID_Parse(
    const ::std::string& name, ClientMsgID* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ClientMsgID>(
    ClientMsgID_descriptor(), name, value);
}
enum ServerMsgID {
  SERVER_MSG_LOGIN_RSP = 1,
  SERVER_MSG_SELECT_ROLE_RSP = 2,
  SERVER_MSG_CREATE_ROLE_RSP = 3,
  SERVER_MSG_ATTRIB_SYNC = 4,
  SERVER_MSG_NEW_ROLE_NOTIFY = 5,
  SERVER_MSG_REMOVE_ROLE_NOTIFY = 6,
  SERVER_MSG_GIVE_GATE_ADDRESS_RSP = 7,
  SERVER_MSG_NOTIFY_TICKET_INFO_RSP = 8,
  SERVER_MSG_GD_COMMAND_RSP = 9,
  SERVER_MSG_PROXY_CREATED = 10,
  SERVER_MSG_OBJECT_DEF_EXPORT = 11,
  SERVER_MSG_REMOTE_NEW_ENTITY_MAIL = 12
};
bool ServerMsgID_IsValid(int value);
const ServerMsgID ServerMsgID_MIN = SERVER_MSG_LOGIN_RSP;
const ServerMsgID ServerMsgID_MAX = SERVER_MSG_GD_COMMAND_RSP;
const int ServerMsgID_ARRAYSIZE = ServerMsgID_MAX + 1;

const ::google::protobuf::EnumDescriptor* ServerMsgID_descriptor();
inline const ::std::string& ServerMsgID_Name(ServerMsgID value) {
  return ::google::protobuf::internal::NameOfEnum(
    ServerMsgID_descriptor(), value);
}
inline bool ServerMsgID_Parse(
    const ::std::string& name, ServerMsgID* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ServerMsgID>(
    ServerMsgID_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)


#ifndef SWIG
namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::ClientMsgID> : ::google::protobuf::internal::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::ClientMsgID>() {
  return ::ClientMsgID_descriptor();
}
template <> struct is_proto_enum< ::ServerMsgID> : ::google::protobuf::internal::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::ServerMsgID>() {
  return ::ServerMsgID_descriptor();
}

}  // namespace protobuf
}  // namespace google
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_ProtocolID_2eproto__INCLUDED
