/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_

#include <vector>

#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

// Returns the history with a modified server reply in edits.
std::vector<mojom::ConversationTurnPtr> GetHistoryWithModifiedReply();

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_TEST_UTILS_H_
