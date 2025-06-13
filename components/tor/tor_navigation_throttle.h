/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_TOR_TOR_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

class TorLauncherFactory;

namespace tor {

class TorNavigationThrottle : public content::NavigationThrottle,
                              public TorLauncherObserver {
 public:
  static std::unique_ptr<TorNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle,
      bool is_tor_profile);
  // For tests to use its own McokTorLauncherFactory
  static std::unique_ptr<TorNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle,
      TorLauncherFactory& tor_launcher_factory,
      bool is_tor_profile);
  explicit TorNavigationThrottle(content::NavigationHandle* navigation_handle);
  TorNavigationThrottle(content::NavigationHandle* navigation_handle,
                        TorLauncherFactory& tor_launcher_factory);
  TorNavigationThrottle(const TorNavigationThrottle&) = delete;
  TorNavigationThrottle& operator=(const TorNavigationThrottle&) = delete;
  ~TorNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

  static void SetSkipWaitForTorConnectedForTesting(bool skip);

 private:
  FRIEND_TEST_ALL_PREFIXES(TorNavigationThrottleUnitTest,
                           DeferUntilTorProcessLaunched);

  // TorLauncherObserver:
  void OnTorCircuitEstablished(bool result) override;

  static bool skip_wait_for_tor_connected_for_testing_;
  bool resume_pending_ = false;
  const raw_ref<TorLauncherFactory> tor_launcher_factory_;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_NAVIGATION_THROTTLE_H_
