// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ProtocolID.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "ProtocolID.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace protobuf_ProtocolID_2eproto {


namespace {

const ::google::protobuf::EnumDescriptor* file_level_enum_descriptors[2];

}  // namespace

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTableField
    const TableStruct::entries[] = {
  {0, 0, 0, ::google::protobuf::internal::kInvalidMask, 0, 0},
};

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::AuxillaryParseTableField
    const TableStruct::aux[] = {
  ::google::protobuf::internal::AuxillaryParseTableField(),
};
PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTable const
    TableStruct::schema[] = {
  { NULL, NULL, 0, -1, -1, false },
};

const ::google::protobuf::uint32 TableStruct::offsets[] = { ~0u };
static const ::google::protobuf::internal::MigrationSchema* schemas = NULL;
static const ::google::protobuf::Message* const* file_default_instances = NULL;
namespace {

void protobuf_AssignDescriptors() {
  AddDescriptors();
  ::google::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "ProtocolID.proto", schemas, file_default_instances, TableStruct::offsets, factory,
      NULL, file_level_enum_descriptors, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

}  // namespace

void TableStruct::Shutdown() {
}

void TableStruct::InitDefaultsImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
}

void InitDefaults() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);
}
void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] = {
      "\n\020ProtocolID.proto*j\n\013ClientMsgID\022\031\n\024CLI"
      "ENT_MSG_LOGIN_REQ\020\350\007\022\037\n\032CLIENT_MSG_SELEC"
      "T_ROLE_REQ\020\351\007\022\037\n\032CLIENT_MSG_CREATE_ROLE_"
      "REQ\020\352\007*\213\001\n\013ServerMsgID\022\032\n\024SERVER_MSG_LOG"
      "IN_RSP\020\240\215\006\022 \n\032SERVER_MSG_SELECT_ROLE_RSP"
      "\020\241\215\006\022 \n\032SERVER_MSG_CREATE_ROLE_RSP\020\242\215\006\022\034"
      "\n\026SERVER_MSG_ATTRIB_SYNC\020\243\215\006"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 268);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "ProtocolID.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::OnShutdown(&TableStruct::Shutdown);
}

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;

}  // namespace protobuf_ProtocolID_2eproto

const ::google::protobuf::EnumDescriptor* ClientMsgID_descriptor() {
  protobuf_ProtocolID_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_ProtocolID_2eproto::file_level_enum_descriptors[0];
}
bool ClientMsgID_IsValid(int value) {
  switch (value) {
    case 1000:
    case 1001:
    case 1002:
      return true;
    default:
      return false;
  }
}

const ::google::protobuf::EnumDescriptor* ServerMsgID_descriptor() {
  protobuf_ProtocolID_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_ProtocolID_2eproto::file_level_enum_descriptors[1];
}
bool ServerMsgID_IsValid(int value) {
  switch (value) {
    case 100000:
    case 100001:
    case 100002:
    case 100003:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
