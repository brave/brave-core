// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "brave/components/ai_chat/browser/models.h"

#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

constexpr char kModelsDefaultKey[] = "chat-default";

// When adding new models, especially for display, make sure to add the UI
// strings to ai_chat_ui_strings.grdp and ai_chat/browser/constants.cc.
// This also applies for modifying keys, since some of the strings are based
// on the model key.
const base::flat_map<std::string_view, mojom::Model> kAllModels = {
    {"chat-default",
     {"chat-default", "llama-2-13b-chat", "llama2 13b", "Meta",
      mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT, false}},
    {"chat-leo-expanded",
     {"chat-leo-expanded", "llama-2-70b-chat", "llama2 70b", "Meta",
      mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT, true}},
    {"chat-claude-instant",
     {"chat-claude-instant", "claude-instant-v1", "Claude Instant", "Anthropic",
      mojom::ModelEngineType::CLAUDE_REMOTE, mojom::ModelCategory::CHAT, true}},
};

const std::vector<std::string_view> kAllModelKeysDisplayOrder = {
    "chat-default",
    "chat-leo-expanded",
    "chat-claude-instant",
};

}  // namespace ai_chat
