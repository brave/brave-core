/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_navigation_throttle.h"

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/common/url_constants.h"
#include "net/base/net_errors.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

// static
void BraveAccountNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (!brave_account::features::IsBraveAccountEnabled()) {
    return;
  }

  if (const GURL& url = registry.GetNavigationHandle().GetURL();
      !url.SchemeIs(content::kChromeUIScheme) ||
      url.host() != kBraveAccountHost || url.path() != "/") {
    return;
  }

  registry.AddThrottle(
      base::WrapUnique(new BraveAccountNavigationThrottle(registry)));
}

BraveAccountNavigationThrottle::~BraveAccountNavigationThrottle() = default;

BraveAccountNavigationThrottle::BraveAccountNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}

content::NavigationThrottle::ThrottleCheckResult
BraveAccountNavigationThrottle::WillStartRequest() {
  return ui::PageTransitionCoreTypeIs(navigation_handle()->GetPageTransition(),
                                      ui::PAGE_TRANSITION_AUTO_TOPLEVEL)
             ? PROCEED
             : ThrottleCheckResult{CANCEL, net::ERR_INVALID_URL};
}

const char* BraveAccountNavigationThrottle::GetNameForLogging() {
  return "BraveAccountNavigationThrottle";
}
