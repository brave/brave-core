/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace brave_search {

class BackupResultsNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit BackupResultsNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~BackupResultsNavigationThrottle() override;

  BackupResultsNavigationThrottle(const BackupResultsNavigationThrottle&) =
      delete;
  BackupResultsNavigationThrottle& operator=(
      const BackupResultsNavigationThrottle&) = delete;

  static std::unique_ptr<BackupResultsNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle);

 private:
  // content::NavigationThrottle overrides:
  ThrottleCheckResult WillStartRequest() override;

  const char* GetNameForLogging() override;
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_NAVIGATION_THROTTLE_H_
