// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_serialization_utils.h"

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

base::DictValue MemoryContentBlockToDict(
    const mojom::MemoryContentBlock& block) {
  base::DictValue memory_dict;
  for (const auto& [key, memory_value] : block.memory) {
    if (memory_value->is_string_value()) {
      memory_dict.Set(key, memory_value->get_string_value());
    } else if (memory_value->is_list_value()) {
      base::ListValue list;
      for (const auto& val : memory_value->get_list_value()) {
        list.Append(val);
      }
      memory_dict.Set(key, std::move(list));
    }
  }
  return memory_dict;
}

base::DictValue FileContentBlockToDict(const mojom::FileContentBlock& block) {
  base::DictValue file_dict;
  file_dict.Set("filename", block.filename);
  file_dict.Set("file_data", block.file_data.spec());
  return file_dict;
}

base::DictValue ImageContentBlockToDict(const mojom::ImageContentBlock& block) {
  base::DictValue image_url;
  image_url.Set("url", block.image_url.spec());
  return image_url;
}

void SerializeToolCallsOnMessageDict(const OAIMessage& message,
                                     base::DictValue& message_dict) {
  if (!message.tool_calls.empty()) {
    base::ListValue tool_call_dicts;
    for (const auto& tool_event : message.tool_calls) {
      base::DictValue tool_call_dict;
      tool_call_dict.Set("id", tool_event->id);
      tool_call_dict.Set("type", "function");

      base::DictValue function_dict;
      function_dict.Set("name", tool_event->tool_name);
      function_dict.Set("arguments", tool_event->arguments_json);

      tool_call_dict.Set("function", std::move(function_dict));
      tool_call_dicts.Append(std::move(tool_call_dict));
    }

    message_dict.Set("tool_calls", std::move(tool_call_dicts));
  }

  if (!message.tool_call_id.empty()) {
    message_dict.Set("tool_call_id", message.tool_call_id);
  }
}

}  // namespace ai_chat
