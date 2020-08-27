/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_IPFS_IPFS_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

#include "brave/browser/ipfs/ipfs_service_observer.h"

namespace content {
class NavigationHandle;
}  // namespace content

class PrefService;

namespace ipfs {

class IpfsService;
class IpfsServiceObserver;

class IpfsNavigationThrottle : public content::NavigationThrottle,
                               public IpfsServiceObserver {
 public:
  explicit IpfsNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~IpfsNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  // IpfsServiceObserver:
  void OnIpfsLaunched(bool result, int64_t pid) override;

  bool resume_pending_ = false;
  IpfsService* ipfs_service_ = nullptr;
  PrefService* pref_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(IpfsNavigationThrottle);
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_NAVIGATION_THROTTLE_H_
