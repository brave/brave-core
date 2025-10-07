// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PROTO_CONVERSION_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PROTO_CONVERSION_H_

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/proto/store.pb.h"

namespace ai_chat {

COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::WebSourcesEventPtr DeserializeWebSourcesEvent(
    const store::WebSourcesEventProto& proto_event);

COMPONENT_EXPORT(AI_CHAT_COMMON)
void SerializeWebSourcesEvent(const mojom::WebSourcesEventPtr& mojom_event,
                              store::WebSourcesEventProto* proto_event);

COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::ToolUseEventPtr DeserializeToolUseEvent(
    const store::ToolUseEventProto& proto_event);

COMPONENT_EXPORT(AI_CHAT_COMMON)
bool SerializeToolUseEvent(const mojom::ToolUseEventPtr& mojom_event,
                           store::ToolUseEventProto* proto_event);

COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::SmartModeEntryPtr DeserializeSmartModeEntry(
    const store::SmartModeEntryProto& proto_entry);

COMPONENT_EXPORT(AI_CHAT_COMMON)
void SerializeSmartModeEntry(const mojom::SmartModeEntryPtr& mojom_entry,
                             store::SmartModeEntryProto* proto_entry);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PROTO_CONVERSION_H_
