/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"

#include <memory>

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"

namespace ai_chat {

// static
std::unique_ptr<AiChatThrottle> AiChatThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  // The AI Chat WebUI won't be enabled if the feature is disabled
  if (!ai_chat::IsAIChatEnabled(user_prefs::UserPrefs::Get(
          navigation_handle->GetWebContents()->GetBrowserContext()))) {
    return nullptr;
  }

  // We don't need this throttle if the full-page feature is enabled via proxy
  // of the AIChatHistory feature flag.
  if (features::IsAIChatHistoryEnabled()) {
    return nullptr;
  }

  // We need this throttle to work only for chrome-untrusted://chat page
  if (!navigation_handle->GetURL().SchemeIs(
          content::kChromeUIUntrustedScheme) ||
      navigation_handle->GetURL().host_piece() != kChatUIHost) {
    return nullptr;
  }

  // Purpose of this throttle is to forbid loading of chrome-untrusted://chat
  // in tab.
  // Parameters check is made different for Android and Desktop because
  // there are different flags:
  // --------+---------------------------------+------------------------------
  //         | Tab                             | Panel
  // --------+---------------------------------+------------------------------
  // Android |PAGE_TRANSITION_FROM_ADDRESS_BAR | PAGE_TRANSITION_FROM_API
  // --------+---------------------------------+------------------------------
  // Desktop |PAGE_TRANSITION_TYPED|           | PAGE_TRANSITION_AUTO_TOPLEVEL
  //         |PAGE_TRANSITION_FROM_ADDRESS_BAR |
  // -------------------------------------------------------------------------
  //
  // So for Android the only allowed transition is PAGE_TRANSITION_FROM_API
  // because it is pretty unique and means the page is loaded in a custom tab
  // view.
  // And for the desktop just disallow PAGE_TRANSITION_FROM_ADDRESS_BAR
  ui::PageTransition transition = navigation_handle->GetPageTransition();
#if BUILDFLAG(IS_ANDROID)
  if (ui::PageTransitionTypeIncludingQualifiersIs(
          transition, ui::PageTransition::PAGE_TRANSITION_FROM_API)) {
    return nullptr;
  }
#else
  if (!ui::PageTransitionTypeIncludingQualifiersIs(
          ui::PageTransitionGetQualifier(transition),
          ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR)) {
    return nullptr;
  }
#endif  // BUILDFLAG(IS_ANDROID)

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
