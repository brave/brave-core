// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "brave/components/ai_chat/browser/models.h"

#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

constexpr char kModelsDefaultKey[] = "chat-default";

const base::flat_map<base::StringPiece, mojom::Model> kAllModels = {
    {"chat-default",
     {"chat-default", "llama-2-13b-chat", mojom::ModelEngineType::LLAMA_REMOTE,
      mojom::ModelCategory::CHAT, false}},
    {"chat-leo-expanded",
     {"chat-leo-expanded", "llama-2-70b-chat",
      mojom::ModelEngineType::LLAMA_REMOTE, mojom::ModelCategory::CHAT, true}},
    {"chat-claude-instant",
     {"chat-claude-instant", "claude-instant-v1",
      mojom::ModelEngineType::CLAUDE_REMOTE, mojom::ModelCategory::CHAT, true}},
    // {"code-llama-basic", {"code-llama-basic",
    //   "code-llama-13b",
    //   mojom::ModelCategory::CODING,
    //   false}},
    // {"code-llama-expanded", {"code-llama-expanded",
    //   "code-llama-70b",
    //   mojom::ModelCategory::CODING,
    //   true}},
};

const std::vector<base::StringPiece> kAllModelKeysDisplayOrder = {
    "chat-default",
    "chat-leo-expanded",
    "chat-claude-instant",
};

}  // namespace ai_chat
