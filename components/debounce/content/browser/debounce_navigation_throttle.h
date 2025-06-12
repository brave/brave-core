// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_DEBOUNCE_CONTENT_BROWSER_DEBOUNCE_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_DEBOUNCE_CONTENT_BROWSER_DEBOUNCE_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}

namespace debounce {

class DebounceService;

class DebounceNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit DebounceNavigationThrottle(content::NavigationHandle* handle,
                                      DebounceService& debounce_service);
  ~DebounceNavigationThrottle() override;

  DebounceNavigationThrottle(const DebounceNavigationThrottle&) = delete;
  DebounceNavigationThrottle& operator=(const DebounceNavigationThrottle&) =
      delete;

  static std::unique_ptr<DebounceNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* handle,
      DebounceService* debounce_service);

  // Implements content::NavigationThrottle.
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult MaybeRedirect();

  const raw_ref<DebounceService> debounce_service_;
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_CONTENT_BROWSER_DEBOUNCE_NAVIGATION_THROTTLE_H_
