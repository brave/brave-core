/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_PROTOCOL_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_PROTOCOL_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace brave_rewards {

class RewardsProtocolNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit RewardsProtocolNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~RewardsProtocolNavigationThrottle() override;

  RewardsProtocolNavigationThrottle(const RewardsProtocolNavigationThrottle&) =
      delete;
  RewardsProtocolNavigationThrottle& operator=(
      const RewardsProtocolNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  // Implements content::NavigationThrottle.
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult MaybeRedirect();
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CONTENT_REWARDS_PROTOCOL_NAVIGATION_THROTTLE_H_
