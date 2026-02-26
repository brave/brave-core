// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_DEEP_RESEARCH_PARSING_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_DEEP_RESEARCH_PARSING_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

// Parse a deep research event from a top-level brave-chat.deepResearch.*
// object. Returns nullopt if the object_type is not recognized or parsing
// fails.
std::optional<mojom::ConversationEntryEventPtr> ParseDeepResearchEvent(
    const std::string& object_type,
    const base::DictValue& params);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_DEEP_RESEARCH_PARSING_H_
