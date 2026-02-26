// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_serialization_utils.h"

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

}  // namespace ai_chat
