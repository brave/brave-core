// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "brave/components/ai_chat/core/browser/models.h"

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

// When adding new models, especially for display, make sure to add the UI
// strings to ai_chat_ui_strings.grdp and ai_chat/core/constants.cc.
// This also applies for modifying keys, since some of the strings are based
// on the model key. Also be sure to migrate prefs if changing or removing
// keys.

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
//
// Breakdown:
// - Reserverd for page content: 100k / 2 = 50k tokens
// - Long conversation warning threshold: 100k * 0.80 = 80k tokens

const std::vector<ai_chat::mojom::Model>& GetAllModels() {
  static const auto kFreemiumAccess =
      features::kFreemiumAvailable.Get() ? mojom::ModelAccess::BASIC_AND_PREMIUM
                                         : mojom::ModelAccess::PREMIUM;
  static const base::NoDestructor<std::vector<mojom::Model>> kModels({
      {"chat-leo-expanded", "mixtral-8x7b-instruct", "Mixtral", "Mistral AI",
       mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT,
       kFreemiumAccess, 8000, 9700},
      {"chat-claude-instant", "claude-instant-v1", "Claude Instant",
       "Anthropic", mojom::ModelEngineType::CLAUDE_REMOTE,
       mojom::ModelCategory::CHAT, kFreemiumAccess, 180000, 320000},
      {"chat-claude-haiku", "claude-3-haiku-20240307", "Claude 3 Haiku",
       "Anthropic", mojom::ModelEngineType::CLAUDE_REMOTE,
       mojom::ModelCategory::CHAT, kFreemiumAccess, 180000, 320000},
      {"chat-claude-sonnet", "claude-3-sonnet-20240229", "Claude 3 Sonnet",
       "Anthropic", mojom::ModelEngineType::CLAUDE_REMOTE,
       mojom::ModelCategory::CHAT, mojom::ModelAccess::PREMIUM, 180000, 320000},
      {"chat-basic", "llama-2-13b-chat", "Llama 2 13b", "Meta",
       mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT,
       mojom::ModelAccess::BASIC, 8000, 9700},
  });
  return *kModels;
}

const ai_chat::mojom::Model* GetModel(std::string_view key) {
  auto& models = GetAllModels();
  auto match = std::find_if(
      models.cbegin(), models.cend(),
      [&key](const mojom::Model& item) { return item.key == key; });
  if (match != models.cend()) {
    return &*match;
  }
  return nullptr;
}

}  // namespace ai_chat
