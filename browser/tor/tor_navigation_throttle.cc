/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_navigation_throttle.h"

#include "content/public/browser/navigation_handle.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"

namespace tor {

TorNavigationThrottle::TorNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

TorNavigationThrottle::~TorNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
TorNavigationThrottle::WillStartRequest() {
  GURL url = navigation_handle()->GetURL();
  if (url.SchemeIsHTTPOrHTTPS() ||
      url.SchemeIs(content::kChromeUIScheme) ||
      url.SchemeIs(extensions::kExtensionScheme) ||
      url.SchemeIs(content::kChromeDevToolsScheme))
    return content::NavigationThrottle::PROCEED;
  return content::NavigationThrottle::BLOCK_REQUEST;
}

const char* TorNavigationThrottle::GetNameForLogging() {
  return "TorNavigationThrottle";
}

}  // namespace tor
