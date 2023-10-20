// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "brave/components/ai_chat/core/models.h"

#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

constexpr char kModelsDefaultKey[] = "chat-default";

// When adding new models, especially for display, make sure to add the UI
// strings to ai_chat_ui_strings.grdp and ai_chat/core/constants.cc.
// This also applies for modifying keys, since some of the strings are based
// on the model key.

// Llama2 Token Allocation:
// - Llama2 has a context limit: tokens + max_new_tokens <= 4096
//
// Breakdown:
// - Reserved for max_new_tokens: 400 tokens
// - Reserved for prompt: 300 tokens
// - Reserved for page content: 4096 - (400 + 300) = 3396 tokens
// - Long conversation warning threshold: 3396 * 0.80 = 2716 tokens

// Claude Token Allocation:
// - Claude has total token limit 100k tokens (75k words)

const base::flat_map<std::string_view, mojom::Model> kAllModels = {
    {"chat-default",
     {"chat-default", "llama-2-13b-chat", "llama2 13b", "Meta",
      mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT, false,
      10000, 9700}},
    {"chat-leo-expanded",
     {"chat-leo-expanded", "llama-2-70b-chat", "llama2 70b", "Meta",
      mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT, true,
      10000, 9700}},
    {"chat-claude-instant",
     {"chat-claude-instant", "claude-instant-v1", "Claude Instant", "Anthropic",
      mojom::ModelEngineType::CLAUDE_REMOTE, mojom::ModelCategory::CHAT, true,
      75000, 75000}},
};

const std::vector<std::string_view> kAllModelKeysDisplayOrder = {
    "chat-default",
    "chat-leo-expanded",
    "chat-claude-instant",
};

}  // namespace ai_chat
