/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_NAVIGATION_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace brave_shields {

class ReduceLanguageNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit ReduceLanguageNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      HostContentSettingsMap* content_settings);
  ~ReduceLanguageNavigationThrottle() override;

  ReduceLanguageNavigationThrottle(const ReduceLanguageNavigationThrottle&) =
      delete;
  ReduceLanguageNavigationThrottle& operator=(
      const ReduceLanguageNavigationThrottle&) = delete;

  static std::unique_ptr<ReduceLanguageNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
                         HostContentSettingsMap* content_settings);

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

 private:
  HostContentSettingsMap* content_settings_ = nullptr;

  void UpdateHeaders();

  base::WeakPtrFactory<ReduceLanguageNavigationThrottle> weak_ptr_factory_{
      this};
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_REDUCE_LANGUAGE_NAVIGATION_THROTTLE_H_
