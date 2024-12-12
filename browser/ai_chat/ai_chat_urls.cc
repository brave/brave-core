// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_urls.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/constants/webui_url_constants.h"
#include "url/gurl.h"

namespace ai_chat {

GURL TabAssociatedConversationUrl() {
  return GURL(base::StrCat({kAIChatUIURL, "tab"}));
}

GURL ConversationUrl(std::string_view conversation_uuid) {
  return GURL(base::StrCat({kAIChatUIURL, conversation_uuid}));
}

std::string_view ConversationUUIDFromURL(const GURL& url) {
  return base::TrimString(url.path_piece(), "/", base::TrimPositions::TRIM_ALL);
}

}  // namespace ai_chat
