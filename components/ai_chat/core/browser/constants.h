/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_

#include <stddef.h>

#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

base::span<const webui::LocalizedString> GetLocalizedStrings();
std::vector<mojom::ActionGroupPtr> GetActionMenuList();

extern const base::fixed_flat_set<std::string_view, 1>
    kPrintPreviewRetrievalHosts;

inline constexpr uint8_t kMaxPreviewPages = 20;
extern const char kLeoModelSupportUrl[];

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

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
