/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"

#include <memory>

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/sessions/core/session_id.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"

namespace ai_chat {

// static
std::unique_ptr<AiChatThrottle> AiChatThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  auto* web_contents = navigation_handle->GetWebContents();

  if (!ai_chat::IsAIChatEnabled(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()))) {
    return nullptr;
  }

  // We need this throttle to work only for chrome-untrusted://chat page
  if (!navigation_handle->GetURL().SchemeIs(
          content::kChromeUIUntrustedScheme) ||
      navigation_handle->GetURL().host_piece() != kChatUIHost) {
    return nullptr;
  }

  SessionID session_id = sessions::SessionTabHelper::IdForTab(web_contents);
  if (!session_id.is_valid()) {
    // Open in non-tab
    return nullptr;
  }

  // Open in a tab
  return std::make_unique<AiChatThrottle>(navigation_handle);
}

AiChatThrottle::AiChatThrottle(content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

AiChatThrottle::~AiChatThrottle() {}

AiChatThrottle::ThrottleCheckResult AiChatThrottle::WillStartRequest() {
  return CANCEL_AND_IGNORE;
}

const char* AiChatThrottle::GetNameForLogging() {
  return "AiChatThrottle";
}

}  // namespace ai_chat
