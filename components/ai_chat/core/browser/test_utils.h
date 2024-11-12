// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_UTILS_H_

#include <string>
#include <vector>

#include "base/location.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

void ExpectConversationEquals(base::Location location,
                              const mojom::ConversationPtr& a,
                              const mojom::ConversationPtr& b);

void ExpectAssociatedContentEquals(base::Location location,
                                   const mojom::SiteInfoPtr& a,
                                   const mojom::SiteInfoPtr& b);

void ExpectConversationEntryEquals(base::Location location,
                                   const mojom::ConversationTurnPtr& a,
                                   const mojom::ConversationTurnPtr& b,
                                   bool compare_uuid = true);

void ExpectConversationHistoryEquals(
    base::Location location,
    const std::vector<mojom::ConversationTurnPtr>& a,
    const std::vector<mojom::ConversationTurnPtr>& b,
    bool compare_uuid = true);

mojom::Conversation* GetConversation(
    base::Location location,
    const std::vector<mojom::ConversationPtr>& conversations,
    std::string uuid);

std::vector<mojom::ConversationTurnPtr> CreateSampleChatHistory(
    size_t num_query_pairs,
    int32_t future_hours = 0);

std::vector<mojom::ConversationTurnPtr> CloneHistory(
    std::vector<mojom::ConversationTurnPtr>& history);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_UTILS_H_
