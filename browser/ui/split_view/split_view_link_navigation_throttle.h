/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationThrottleRegistry;
}  // namespace content

// NavigationThrottle that intercepts navigations from the left pane in a split
// view and redirects them to the right pane when the split view is linked.
class SplitViewLinkNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit SplitViewLinkNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~SplitViewLinkNavigationThrottle() override;

  SplitViewLinkNavigationThrottle(const SplitViewLinkNavigationThrottle&) =
      delete;
  SplitViewLinkNavigationThrottle& operator=(
      const SplitViewLinkNavigationThrottle&) = delete;

  // Creates and adds the throttle to the registry if conditions are met.
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  // Checks if the navigation should be redirected to the right pane.
  ThrottleCheckResult MaybeRedirectToRightPane();
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_NAVIGATION_THROTTLE_H_
