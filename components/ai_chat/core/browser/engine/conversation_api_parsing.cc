// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_parsing.h"

#include <string>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/conversation_api_client.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

base::Value ContentBlocksToJson(const ConversationAPIClient::Content& content) {
  // Content string or content blocks
  if (auto* content_strings = std::get_if<std::vector<std::string>>(&content)) {
    if (content_strings->empty()) {
      return base::Value("");
    } else if (content_strings->size() == 1) {
      return base::Value(content_strings->front());
    } else {
      base::Value::List content_list;
      for (const auto& content_item : *content_strings) {
        content_list.Append(content_item);
      }
      return base::Value(std::move(content_list));
    }
  } else if (auto* content_blocks =
                 std::get_if<std::vector<mojom::ContentBlockPtr>>(&content)) {
    base::Value::List content_items;
    for (const auto& content_block : *content_blocks) {
      base::Value::Dict content_item;
      switch (content_block->which()) {
        case mojom::ContentBlock::Tag::kImageContentBlock:
          content_item.Set("type", "image_url");
          content_item.SetByDottedPath(
              "image_url.url",
              content_block->get_image_content_block()->image_url.spec());
          break;
        case mojom::ContentBlock::Tag::kTextContentBlock:
          content_item.Set("type", "text");
          content_item.Set("text",
                           content_block->get_text_content_block()->text);
          break;
        default:
          NOTREACHED();
      }
      content_items.Append(std::move(content_item));
    }
    return base::Value(std::move(content_items));
  }

  NOTREACHED();
}

}  // namespace ai_chat
