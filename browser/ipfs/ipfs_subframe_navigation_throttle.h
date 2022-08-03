/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_SUBFRAME_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_IPFS_IPFS_SUBFRAME_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"

namespace ipfs {

// Prevents commiting of subframe IPFS navigations.
// IPFS urls must be changed to proper gateway urls.
// See ipfs_redirect_network_delegate_helper.h for example.
class IpfsSubframeNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<IpfsSubframeNavigationThrottle> CreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  explicit IpfsSubframeNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~IpfsSubframeNavigationThrottle() override;

  IpfsSubframeNavigationThrottle(const IpfsSubframeNavigationThrottle&) =
      delete;
  IpfsSubframeNavigationThrottle& operator=(
      const IpfsSubframeNavigationThrottle&) = delete;

  // content::NavigationThrottle implementation:
  // This is called before navigation commits with error.
  // Here we can cancel subframe navigation for ipfs:// urls.
  content::NavigationThrottle::ThrottleCheckResult WillFailRequest() override;
  const char* GetNameForLogging() override;
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_SUBFRAME_NAVIGATION_THROTTLE_H_
