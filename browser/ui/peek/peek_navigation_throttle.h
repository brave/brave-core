/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

// Keeps the main frame of a Peek on http(s) pages. Navigations and redirects
// to anything else, e.g. about:blank, blob:, data:, file:, browser pages or
// external protocols, are cancelled.
class PeekNavigationThrottle : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  explicit PeekNavigationThrottle(content::NavigationThrottleRegistry& registry);
  PeekNavigationThrottle(const PeekNavigationThrottle&) = delete;
  PeekNavigationThrottle& operator=(const PeekNavigationThrottle&) = delete;
  ~PeekNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillCommitWithoutUrlLoader() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult CheckURL();
};

#endif  // BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_
