// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

const std::vector<Tool*> GetToolsForConversation(bool has_associated_content,
                                                 const mojom::Model& model);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_TOOLS_H_
