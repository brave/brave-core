/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_protocol_navigation_throttle.h"

#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::NavigationHandle;
using content::NavigationThrottle;
using content::WebContents;

namespace brave_rewards {

// static
std::unique_ptr<RewardsProtocolNavigationThrottle>
RewardsProtocolNavigationThrottle::MaybeCreateThrottleFor(
    NavigationHandle* navigation_handle) {
  return std::make_unique<RewardsProtocolNavigationThrottle>(navigation_handle);
}

RewardsProtocolNavigationThrottle::RewardsProtocolNavigationThrottle(
    NavigationHandle* handle)
    : NavigationThrottle(handle) {}

RewardsProtocolNavigationThrottle::~RewardsProtocolNavigationThrottle() =
    default;

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::WillStartRequest() {
  return MaybeRedirect();
}

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::WillRedirectRequest() {
  return MaybeRedirect();
}

NavigationThrottle::ThrottleCheckResult
RewardsProtocolNavigationThrottle::MaybeRedirect() {
  WebContents* web_contents = navigation_handle()->GetWebContents();
  GURL original_url = navigation_handle()->GetURL();

  if (brave_rewards::IsRewardsProtocol(original_url)) {
    brave_rewards::HandleRewardsProtocol(original_url, web_contents,
                                         ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
    return NavigationThrottle::CANCEL;
  } else {
    return NavigationThrottle::PROCEED;
  }
}

const char* RewardsProtocolNavigationThrottle::GetNameForLogging() {
  return "RewardsProtocolNavigationThrottle";
}

}  // namespace brave_rewards
