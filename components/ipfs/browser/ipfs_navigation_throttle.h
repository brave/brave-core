/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "brave/components/ipfs/browser/ipfs_service_observer.h"
#include "content/public/browser/navigation_throttle.h"

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
      content::NavigationHandle* navigation_handle, IpfsService* ipfs_service);
  ~IpfsNavigationThrottle() override;

  IpfsNavigationThrottle(const IpfsNavigationThrottle&) = delete;
  IpfsNavigationThrottle& operator=(const IpfsNavigationThrottle&) = delete;

  static std::unique_ptr<IpfsNavigationThrottle>
      MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
          IpfsService* ipfs_service, bool regular_profile);

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsNavigationThrottleUnitTest,
                           DeferUntilIpfsProcessLaunched);
  // IpfsServiceObserver:
  void OnIpfsLaunched(bool result, int64_t pid) override;

  bool resume_pending_ = false;
  IpfsService* ipfs_service_ = nullptr;
  PrefService* pref_service_ = nullptr;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_NAVIGATION_THROTTLE_H_
