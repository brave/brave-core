/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_

#include <vector>

#include "base/containers/fixed_flat_set.h"
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

// Llama 3 has a max context length of 8k tokens. Phi 3 Mini and Llama 2 have 4k
// tokens max. Using Phi 3 Mini as the upper bound, 1 token ≈ 4 characters, so
// the max context length is ~16k characters. To allow for follow-ups, set max
// page content length to 60% of this, i.e., 9.6k characters.
inline constexpr int kCustomModelMaxPageContentLength = 9600;
inline constexpr int kCustomModelLongConversationCharLimit = 10000;

inline constexpr char kBraveSearchURLPrefix[] = "search";

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONSTANTS_H_
