// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/navigation_throttle_registry.h"
namespace query_filter {

// This class allows to intercept navigations that will be intercepted by
// a service worker.
class QueryFilterNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit QueryFilterNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~QueryFilterNavigationThrottle() override;

  QueryFilterNavigationThrottle(const QueryFilterNavigationThrottle&) = delete;
  QueryFilterNavigationThrottle& operator=(
      const QueryFilterNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  // `content::NavigationThrottle`:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;
};
}  // namespace query_filter
#endif  // BRAVE_COMPONENTS_QUERY_FILTER_CONTENT_QUERY_FILTER_NAVIGATION_THROTTLE_H_
