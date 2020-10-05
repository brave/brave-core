/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_throttle.h"

class Profile;

namespace content {
class NavigationHandle;
}  // namespace content

namespace tor {

class OnionLocationNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<OnionLocationNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle);
  explicit OnionLocationNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~OnionLocationNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillProcessResponse() override;
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  void OnTorProfileCreated(GURL onion_location,
                           Profile* proile,
                           Profile::CreateStatus status);

  Profile* profile_;

  base::WeakPtrFactory<OnionLocationNavigationThrottle> weak_ptr_factory_{this};

  OnionLocationNavigationThrottle(const OnionLocationNavigationThrottle&) =
      delete;
  OnionLocationNavigationThrottle& operator=(
      const OnionLocationNavigationThrottle&) = delete;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_ONION_LOCATION_NAVIGATION_THROTTLE_H_
