// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_AI_CHAT_URLS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_AI_CHAT_URLS_H_

#include <optional>
#include <string_view>

#include "base/files/file_path.h"
#include "url/gurl.h"

namespace ai_chat {

// UI that will open a conversation associated with the active Tab in the same
// browser window. The conversation will change when that Tab navigates.
COMPONENT_EXPORT(AI_CHAT_COMMON) GURL TabAssociatedConversationUrl();

// UI that will open to a specific conversation. The conversation will not
// change upon any navigation.
COMPONENT_EXPORT(AI_CHAT_COMMON)
GURL ConversationUrl(std::string_view conversation_uuid);

// Extracts the conversation UUID from a conversation URL or a conversation
// entries iframe
COMPONENT_EXPORT(AI_CHAT_COMMON)
std::string_view ConversationUUIDFromURL(const GURL& url);

// Records |folder| as a Leo workspace's associated content URL:
// chrome-untrusted://leo-workspace/?folder=<escaped folder>. The URL is already
// persisted with every associated content, so this needs no database column.
//
// This is a browser-side record, not a page URL: the workspace page is never
// navigated to it, so the renderer can neither read the folder nor rewrite it
// into one the user never picked.
COMPONENT_EXPORT(AI_CHAT_COMMON)
GURL LeoWorkspaceContentURL(const base::FilePath& folder);

// Inverse of LeoWorkspaceContentURL. Returns nullopt if |url| is not a Leo
// workspace URL or carries no usable absolute folder path.
COMPONENT_EXPORT(AI_CHAT_COMMON)
std::optional<base::FilePath> LeoWorkspaceFolderFromURL(const GURL& url);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_AI_CHAT_URLS_H_
