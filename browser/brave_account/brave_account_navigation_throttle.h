/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationThrottleRegistry;
}  // namespace content

class BraveAccountNavigationThrottle : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  BraveAccountNavigationThrottle(const BraveAccountNavigationThrottle&) =
      delete;
  BraveAccountNavigationThrottle& operator=(
      const BraveAccountNavigationThrottle&) = delete;

  ~BraveAccountNavigationThrottle() override;

 private:
  explicit BraveAccountNavigationThrottle(
      content::NavigationThrottleRegistry& registry);

  // content::NavigationThrottle:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;
};

#endif  // BRAVE_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_NAVIGATION_THROTTLE_H_
