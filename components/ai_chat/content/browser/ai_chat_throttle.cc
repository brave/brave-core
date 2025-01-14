/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_throttle.h"

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

// static
std::unique_ptr<AIChatThrottle> AIChatThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  // The throttle's only purpose is to deny navigation in a Tab.

  // The AI Chat WebUI won't be enabled if the feature or policy is disabled
  // (this is not checking a user preference).
  if (!ai_chat::IsAIChatEnabled(user_prefs::UserPrefs::Get(
          navigation_handle->GetWebContents()->GetBrowserContext()))) {
    return nullptr;
  }

  const GURL& url = navigation_handle->GetURL();

  bool is_main_page_url = url.SchemeIs(content::kChromeUIScheme) &&
                          url.host_piece() == kAIChatUIHost;

  // We allow main page navigation only if the full-page feature is enabled
  // via the AIChatHistory feature flag.
  if (is_main_page_url && features::IsAIChatHistoryEnabled()) {
    return nullptr;
  }

  bool is_ai_chat_frame =
      url.SchemeIs(content::kChromeUIUntrustedScheme) &&
      url.host_piece() == kAIChatUntrustedConversationUIHost;

  // We need this throttle to work only for AI Chat related URLs
  if (!is_main_page_url && !is_ai_chat_frame) {
    return nullptr;
  }

// On Android, we only have full page chat, so we always allow loading. On
// desktop we just disallow PAGE_TRANSITION_FROM_ADDRESS_BAR.
#if BUILDFLAG(IS_ANDROID)
  return nullptr;
#else
  ui::PageTransition transition = navigation_handle->GetPageTransition();
  if (!ui::PageTransitionTypeIncludingQualifiersIs(
          ui::PageTransitionGetQualifier(transition),
          ui::PageTransition::PAGE_TRANSITION_FROM_ADDRESS_BAR)) {
    return nullptr;
  }
  return std::make_unique<AIChatThrottle>(navigation_handle);
#endif  // BUILDFLAG(IS_ANDROID)
}

AIChatThrottle::AIChatThrottle(content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

AIChatThrottle::~AIChatThrottle() {}

AIChatThrottle::ThrottleCheckResult AIChatThrottle::WillStartRequest() {
  return CANCEL_AND_IGNORE;
}

const char* AIChatThrottle::GetNameForLogging() {
  return "AIChatThrottle";
}

}  // namespace ai_chat
