/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_subframe_navigation_throttle.h"

#include "brave/components/ipfs/ipfs_constants.h"

namespace ipfs {

// static
std::unique_ptr<IpfsSubframeNavigationThrottle>
IpfsSubframeNavigationThrottle::CreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  return std::make_unique<IpfsSubframeNavigationThrottle>(navigation_handle);
}

IpfsSubframeNavigationThrottle::IpfsSubframeNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

IpfsSubframeNavigationThrottle::~IpfsSubframeNavigationThrottle() = default;

// content::NavigationThrottle implementation:
content::NavigationThrottle::ThrottleCheckResult
IpfsSubframeNavigationThrottle::WillFailRequest() {
  // Ignores subframe ipfs:// navigation. It is ok to commit toplevel
  // navigation.
  if (!navigation_handle()->IsInMainFrame() &&
      (navigation_handle()->GetURL().SchemeIs(ipfs::kIPFSScheme) ||
       navigation_handle()->GetURL().SchemeIs(ipfs::kIPNSScheme))) {
    return {content::NavigationThrottle::CANCEL_AND_IGNORE,
            navigation_handle()->GetNetErrorCode()};
  }
  return content::NavigationThrottle::PROCEED;
}

const char* IpfsSubframeNavigationThrottle::GetNameForLogging() {
  return "IpfsSubframeNavigationThrottle";
}

}  // namespace ipfs
