/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "url/url_constants.h"
namespace ai_chat {

inline constexpr char kBraveSearchURLPrefix[] = "search";

inline constexpr char kBraveUntrustedContentOpenTag[] =
    "<brave_untrusted_content>";
inline constexpr char kBraveUntrustedContentCloseTag[] =
    "</brave_untrusted_content>";
inline constexpr char kBraveUntrustedContentTagName[] =
    "brave_untrusted_content";

inline constexpr auto kAllowedContentSchemes =
    base::MakeFixedFlatSet<std::string_view>(
        {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme,
         url::kDataScheme});

// Model key for Claude Haiku model.
inline constexpr char kClaudeHaikuModelKey[] = "chat-claude-haiku";
// Model key for Claude Sonnet model.
inline constexpr char kClaudeSonnetModelKey[] = "chat-claude-sonnet";

// Keys for custom model prefs
inline constexpr char kCustomModelItemModelKey[] = "model_request_name";
inline constexpr char kCustomModelItemEndpointUrlKey[] = "endpoint_url";

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_
