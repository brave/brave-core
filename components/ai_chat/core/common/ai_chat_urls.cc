// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/ai_chat_urls.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/constants/webui_url_constants.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

#if BUILDFLAG(IS_ANDROID)
constexpr char kBraveScheme[] = "brave";
#endif

GURL CreateAIChatUIURL(std::string_view path) {
  GURL url(base::StrCat({kAIChatUIURL, path}));
#if BUILDFLAG(IS_ANDROID)
  // TODO(https://github.com/brave/brave-browser/issues/51302): Remove this
  // override once chrome:// URLs are rewritten as brave:// URLs in the android
  // UI.
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kBraveScheme);
  url = url.ReplaceComponents(replacements);
#endif
  return url;
}

}  // namespace

GURL TabAssociatedConversationUrl() {
  return CreateAIChatUIURL("tab");
}

GURL ConversationUrl(std::string_view conversation_uuid) {
  return CreateAIChatUIURL(conversation_uuid);
}

std::string_view ConversationUUIDFromURL(const GURL& url) {
  return base::TrimString(url.path(), "/", base::TrimPositions::TRIM_ALL);
}

}  // namespace ai_chat
