// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_UI_SEMANTIC_SEARCH_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_UI_SEMANTIC_SEARCH_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

struct ConversationSearchResult;

using UISearchConversationsCallback = base::OnceCallback<void(
    std::optional<std::vector<ConversationSearchResult>>)>;

// Searches the stored conversations of `context` semantically for the UI, as
// brave://history searches browsing history: a query of fewer words than
// history's minimum finds nothing, and as many conversations are found as
// history shows results. `callback` gets null while semantic search is
// unavailable.
void SearchConversationsForUI(content::BrowserContext* context,
                              const std::string& query,
                              UISearchConversationsCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_UI_SEMANTIC_SEARCH_H_
