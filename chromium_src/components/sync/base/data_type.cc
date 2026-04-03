/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_SPECIFICS_FIELD_NUMBER_TO_DATA_TYPE            \
  {sync_pb::EntitySpecifics::kAiChatConversationFieldNumber, \
   AI_CHAT_CONVERSATION},

#define BRAVE_DATA_TYPE_HISTOGRAM_VALUE \
  case AI_CHAT_CONVERSATION:            \
    return DataTypeForHistograms::kAIChatConversation;

#define BRAVE_DATA_TYPE_TO_STABLE_LOWER_CASE_STRING \
  case AI_CHAT_CONVERSATION:                        \
    return "ai_chat_conversation";

#define AddDefaultFieldValue AddDefaultFieldValue_ChromiumImpl
#define GetSpecificsFieldNumberFromDataType \
  GetSpecificsFieldNumberFromDataType_ChromiumImpl
#define DataTypeToDebugString DataTypeToDebugString_ChromiumImpl
#define DataTypeToHistogramSuffix DataTypeToHistogramSuffix_ChromiumImpl
#define EncryptableUserTypes EncryptableUserTypes_ChromiumImpl

// Suppress -Wswitch for the renamed ChromiumImpl functions whose switch
// statements don't handle AI_CHAT_CONVERSATION (we handle it in our
// override wrappers instead).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#include <components/sync/base/data_type.cc>
#pragma clang diagnostic pop

#undef EncryptableUserTypes
#undef DataTypeToHistogramSuffix
#undef DataTypeToDebugString
#undef GetSpecificsFieldNumberFromDataType
#undef AddDefaultFieldValue
#undef BRAVE_DATA_TYPE_TO_STABLE_LOWER_CASE_STRING
#undef BRAVE_DATA_TYPE_HISTOGRAM_VALUE
#undef BRAVE_SPECIFICS_FIELD_NUMBER_TO_DATA_TYPE

namespace syncer {

void AddDefaultFieldValue(DataType type, sync_pb::EntitySpecifics* specifics) {
  if (type == AI_CHAT_CONVERSATION) {
    specifics->mutable_ai_chat_conversation();
    return;
  }
  AddDefaultFieldValue_ChromiumImpl(type, specifics);
}

int GetSpecificsFieldNumberFromDataType(DataType data_type) {
  if (data_type == AI_CHAT_CONVERSATION) {
    return sync_pb::EntitySpecifics::kAiChatConversationFieldNumber;
  }
  return GetSpecificsFieldNumberFromDataType_ChromiumImpl(data_type);
}

const char* DataTypeToDebugString(DataType data_type) {
  if (data_type == AI_CHAT_CONVERSATION) {
    return "AI Chat Conversation";
  }
  return DataTypeToDebugString_ChromiumImpl(data_type);
}

const char* DataTypeToHistogramSuffix(DataType data_type) {
  if (data_type == AI_CHAT_CONVERSATION) {
    return "AI_CHAT_CONVERSATION";
  }
  return DataTypeToHistogramSuffix_ChromiumImpl(data_type);
}

DataTypeSet EncryptableUserTypes() {
  DataTypeSet encryptable_user_types = EncryptableUserTypes_ChromiumImpl();
  // Brave sync has encryption setup ready when sync chain created
  encryptable_user_types.Put(DEVICE_INFO);
  encryptable_user_types.Put(HISTORY);
  encryptable_user_types.Put(AI_CHAT_CONVERSATION);
  return encryptable_user_types;
}

}  // namespace syncer
