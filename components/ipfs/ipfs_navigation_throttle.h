/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPFS_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_IPFS_IPFS_NAVIGATION_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ipfs/ipfs_service_observer.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

class PrefService;

namespace ipfs {

class IpfsService;

class IpfsNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit IpfsNavigationThrottle(content::NavigationHandle* navigation_handle,
                                  IpfsService* ipfs_service,
                                  PrefService* pref_service,
                                  const std::string& locale);
  ~IpfsNavigationThrottle() override;

  IpfsNavigationThrottle(const IpfsNavigationThrottle&) = delete;
  IpfsNavigationThrottle& operator=(const IpfsNavigationThrottle&) = delete;

  static std::unique_ptr<IpfsNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle,
      IpfsService* ipfs_service,
      PrefService* pref_service,
      const std::string& locale);

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillFailRequest() override;
  const char* GetNameForLogging() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsNavigationThrottleUnitTest,
                           DeferUntilIpfsProcessLaunched);
  FRIEND_TEST_ALL_PREFIXES(IpfsNavigationThrottleUnitTest,
                           DeferUntilPeersFetched);
  FRIEND_TEST_ALL_PREFIXES(IpfsNavigationThrottleUnitTest, SequentialRequests);
  FRIEND_TEST_ALL_PREFIXES(IpfsNavigationThrottleUnitTest,
                           DeferMultipleUntilIpfsProcessLaunched);

  void ShowInterstitial();
  content::NavigationThrottle::ThrottleCheckResult
  ShowIPFSOnboardingInterstitial();

  void LoadPublicGatewayURL();
  void GetConnectedPeers();
  void OnGetConnectedPeers(bool success, const std::vector<std::string>& peers);
  void OnIpfsLaunched(bool result);
  bool ShouldAsk();

  bool resume_pending_ = false;
  IpfsService* ipfs_service_ = nullptr;
  PrefService* pref_service_ = nullptr;
  std::string locale_;
  base::WeakPtrFactory<IpfsNavigationThrottle> weak_ptr_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_IPFS_NAVIGATION_THROTTLE_H_
