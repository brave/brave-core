/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_

#include <stddef.h>

#include <cstdint>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

base::span<const webui::LocalizedString> GetLocalizedStrings();
std::vector<mojom::ActionGroupPtr> GetActionMenuList();

inline constexpr auto kPrintPreviewRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>({
        "docs.google.com",
        "watermark.silverchair.com",
    });

inline constexpr char kLeoModelSupportUrl[] =
    "https://support.brave.app/hc/en-us/articles/26727364100493-"
    "What-are-the-differences-between-Leo-s-AI-Models";

inline constexpr char kLeoGoPremiumUrl[] =
    "https://account.brave.com/account/?intent=checkout&product=leo";

inline constexpr char kLeoRefreshPremiumSessionUrl[] =
    "https://account.brave.com/?intent=recover&product=leo";

inline constexpr char kLeoStorageSupportUrl[] =
    "https://support.brave.app/hc/en-us/articles/"
    "32663367857549-How-do-I-use-Chat-History-in-Brave-Leo";

inline constexpr char kLeoBraveSearchSupportUrl[] =
    "https://support.brave.app/hc/en-us/articles/"
    "27586048343309-How-does-Leo-get-current-information";

inline constexpr char kBraveAIChatCustomizationSubPage[] =
    "leo-ai/customization";

// Upon registering a custom model, users have the ability to explicitly
// provide a context size (in tokens). When present, we'll use this value to
// determine the max associated content length (in chars). We will assume 4
// chars per token. When no context size has been provided, we will default to a
// conservative 4k tokens based on common models like Phi 3 Mini and Llama 2
// (both have 4k token context limits).
inline constexpr size_t kDefaultCharsPerToken = 4;
inline constexpr float kMaxContentLengthThreshold = 0.6f;
inline constexpr size_t kReservedTokensForPrompt = 300;
inline constexpr size_t kReservedTokensForMaxNewTokens = 400;

// Maximum characters per content for title generation to avoid overly long
// context.
inline constexpr uint32_t kMaxContextCharsForTitleGeneration = 1200u;

// Model name to send to the server for Claude Haiku model.
inline constexpr char kClaudeHaikuModelName[] = "claude-3-haiku";
// Model name to send to the server for Claude Sonnet model.
inline constexpr char kClaudeSonnetModelName[] = "claude-3-sonnet";

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
