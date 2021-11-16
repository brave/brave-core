/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

class GURL;
class PrefService;

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace tor {

class OnionLocationNavigationThrottle : public content::NavigationThrottle {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OpenInTorWindow(content::WebContents* context,
                                 GURL onion_location) = 0;
  };
  static std::unique_ptr<OnionLocationNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
                         bool is_tor_disabled,
                         std::unique_ptr<Delegate> delegate,
                         bool is_tor_profile);
  explicit OnionLocationNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      std::unique_ptr<Delegate> delegate,
      bool is_tor_profile);
  ~OnionLocationNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillProcessResponse() override;
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

  static void BlockOnionRequestsOutsideTorForTesting(bool block) {
    block_onion_requests_outside_tor_for_testing_ = block;
  }

 private:
  static bool block_onion_requests_outside_tor_for_testing_;
  bool is_tor_profile_ = false;

  PrefService* pref_service_ = nullptr;

  std::unique_ptr<Delegate> delegate_;

  OnionLocationNavigationThrottle(const OnionLocationNavigationThrottle&) =
      delete;
  OnionLocationNavigationThrottle& operator=(
      const OnionLocationNavigationThrottle&) = delete;
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
