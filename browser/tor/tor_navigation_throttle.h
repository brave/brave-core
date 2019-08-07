/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace tor {

class TorNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<TorNavigationThrottle>
    MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle);
  explicit TorNavigationThrottle(content::NavigationHandle* navigation_handle);
  ~TorNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TorNavigationThrottle);
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_NAVIGATION_THROTTLE_H_
