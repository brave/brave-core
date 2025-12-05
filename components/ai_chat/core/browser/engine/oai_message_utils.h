// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

struct OAIMessage {
  OAIMessage();
  OAIMessage(const OAIMessage&) = delete;
  OAIMessage& operator=(const OAIMessage&) = delete;
  OAIMessage(OAIMessage&&);
  OAIMessage& operator=(OAIMessage&&);
  ~OAIMessage();

  std::string role;
  std::vector<ExtendedContentBlock> content;
};

std::vector<OAIMessage> BuildOAIRewriteSuggestionMessages(
    std::string_view text,
    mojom::ActionType action_type);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_MESSAGE_UTILS_H_
