/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

class GURL;

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tor {

class OnionLocationNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<OnionLocationNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
                         bool is_tor_disabled,
                         bool is_tor_profile);
  explicit OnionLocationNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      bool is_tor_profile);
  ~OnionLocationNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillProcessResponse() override;
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  bool is_tor_profile_ = false;

  OnionLocationNavigationThrottle(const OnionLocationNavigationThrottle&) =
      delete;
  OnionLocationNavigationThrottle& operator=(
      const OnionLocationNavigationThrottle&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
