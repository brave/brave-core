/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationThrottleRegistry;
}  // namespace content

// Cancels user-initiated cross-site link navigations in pinned tabs and opens
// them in Peek instead, so the pinned tab keeps its page.
class PeekNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit PeekNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~PeekNavigationThrottle() override;

  PeekNavigationThrottle(const PeekNavigationThrottle&) = delete;
  PeekNavigationThrottle& operator=(const PeekNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;
};

#endif  // BRAVE_BROWSER_UI_PEEK_PEEK_NAVIGATION_THROTTLE_H_
