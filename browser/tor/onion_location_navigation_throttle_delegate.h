/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_DELEGATE_H_
#define BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_DELEGATE_H_

#include "brave/components/tor/onion_location_navigation_throttle.h"

class GURL;

namespace content {
class WebContents;
}  // namespace content

namespace tor {

class OnionLocationNavigationThrottleDelegate
    : public OnionLocationNavigationThrottle::Delegate {
 public:
  OnionLocationNavigationThrottleDelegate();
  ~OnionLocationNavigationThrottleDelegate() override;

  void OpenInTorWindow(content::WebContents* web_contents,
                       GURL onion_location) override;

 private:
  OnionLocationNavigationThrottleDelegate(
      const OnionLocationNavigationThrottleDelegate&) = delete;
  OnionLocationNavigationThrottleDelegate& operator=(
      const OnionLocationNavigationThrottleDelegate&) = delete;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_DELEGATE_H_
