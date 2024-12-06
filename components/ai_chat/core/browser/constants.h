/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_

#include <limits>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

base::span<const webui::LocalizedString> GetLocalizedStrings();
std::vector<mojom::ActionGroupPtr> GetActionMenuList();

inline constexpr auto kPrintPreviewRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>({
        "docs.google.com",
    });

inline constexpr uint8_t kMaxPreviewPages = 20;
inline constexpr char kLeoModelSupportUrl[] =
    "https://support.brave.com/hc/en-us/categories/"
    "20990938292237-Brave-Leo";

// Upon registering a custom model, users have the ability to explicitly
// provide a context size (in tokens). When present, we'll use this value to
// determine the max associated content length (in chars). We will assume 4
// chars per token. When no context size has been provided, we will default to a
// conservative 4k tokens based on common models like Phi 3 Mini and Llama 2
// (both have 4k token context limits).
constexpr size_t kDefaultCharsPerToken = 4;
constexpr float kMaxContentLengthThreshold = 0.6f;
constexpr size_t kReservedTokensForPrompt = 300;
constexpr size_t kReservedTokensForMaxNewTokens = 400;

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
