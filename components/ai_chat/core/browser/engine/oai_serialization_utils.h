// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_SERIALIZATION_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_SERIALIZATION_UTILS_H_

// Shared utilities for serializing OAI content block types (mojom) to
// base::DictValue.

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace ai_chat {

// Converts a MemoryContentBlock's memory map to a base::DictValue.
// String values are stored directly; list values become base::ListValue.
base::DictValue MemoryContentBlockToDict(
    const mojom::MemoryContentBlock& block);

// Converts a FileContentBlock to {"filename": ..., "file_data": ...}.
base::DictValue FileContentBlockToDict(const mojom::FileContentBlock& block);

// Converts an ImageContentBlock to {"url": ...}.
base::DictValue ImageContentBlockToDict(const mojom::ImageContentBlock& block);

// Serializes tool_calls and tool_call_id from an OAIMessage onto message_dict.
void SerializeToolCallsOnMessageDict(const OAIMessage& message,
                                     base::DictValue& message_dict);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_SERIALIZATION_UTILS_H_
