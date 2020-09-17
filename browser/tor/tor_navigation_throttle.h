/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "brave/browser/tor/tor_launcher_service_observer.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace tor {

class TorProfileService;

class TorNavigationThrottle : public content::NavigationThrottle,
                              public TorLauncherServiceObserver {
 public:
  static std::unique_ptr<TorNavigationThrottle>
    MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle);
  explicit TorNavigationThrottle(content::NavigationHandle* navigation_handle);
  ~TorNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(TorNavigationThrottleUnitTest,
                           DeferUntilTorProcessLaunched);

  // TorLauncherServiceObserver:
  void OnTorCircuitEstablished(bool result) override;

  bool resume_pending_ = false;
  TorProfileService* tor_profile_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TorNavigationThrottle);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_
