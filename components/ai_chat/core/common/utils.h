/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_

#include "base/component_export.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

class GURL;

namespace ai_chat {

COMPONENT_EXPORT(AI_CHAT_COMMON)
bool IsBraveSearchURL(const GURL& url);

COMPONENT_EXPORT(AI_CHAT_COMMON)
bool IsOpenAIChatButtonFromBraveSearchURL(const GURL& url);

// Returns the most recent edit of |turn|, or |turn| itself if it has none.
COMPONENT_EXPORT(AI_CHAT_COMMON)
const mojom::ConversationTurnPtr& GetLatestTurn(
    const mojom::ConversationTurnPtr& turn);

COMPONENT_EXPORT(AI_CHAT_COMMON)
mojom::ConversationTurnPtr& GetLatestTurn(mojom::ConversationTurnPtr& turn);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_
