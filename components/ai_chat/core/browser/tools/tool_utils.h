// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_

#include <string>
#include <string_view>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

std::vector<mojom::ContentBlockPtr> CreateContentBlocksForText(
    const std::string& text);

std::vector<mojom::ContentBlockPtr> CreateContentBlocksForImage(
    const GURL& image_url);

// Reduces `text` to an allowlist of `[0-9A-Za-z/_-.:]`, dropping every other
// character (including all whitespace), then backslash-escapes the punctuation
// that survives. Used for third-party names and hosts (e.g. WebMCP or MCP tool
// names) shown in the permission prompt, so their owner cannot inject markdown
// formatting, a link, or extra prompt lines into the challenge text.
std::string EscapeMarkdown(std::string_view text);

// Escapes third-party prose, such as a tool's description, while keeping it
// readable. Every run of whitespace, including line breaks, becomes a single
// space, control characters are dropped and ASCII punctuation is
// backslash-escaped, so the text can't add markdown formatting, links or lines
// of its own.
std::string EscapeMarkdownText(std::string_view text);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_UTILS_H_
