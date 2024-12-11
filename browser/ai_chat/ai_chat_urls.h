// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_URLS_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_URLS_H_

#include <string_view>

#include "url/gurl.h"

namespace ai_chat {

// UI that will open a conversation associated with the active Tab in the same
// browser window. The conversation will change when that Tab navigates.
GURL TabAssociatedConversationUrl();

// UI that will open to a specific conversation. The conversation will not
// change upon any navigation.
GURL ConversationUrl(std::string_view conversation_uuid);

// Extracts the conversation UUID from a conversation URL or a conversation
// entries iframe
std::string_view ConversationUUIDFromURL(const GURL& url);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_URLS_H_
