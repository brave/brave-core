// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_

#include <optional>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

std::optional<std::vector<mojom::ContentBlockPtr>> CreateContentBlocksForText(
    const std::string& text);

std::optional<std::vector<mojom::ContentBlockPtr>> CreateContentBlocksForImage(
    const std::string& image_url);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_
