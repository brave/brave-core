// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_NAVIGATION_THROTTLE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationThrottleRegistry;
}  // namespace content

namespace traffic_control {

class TrafficControlService;

class TrafficControlNavigationThrottle : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry,
                                TrafficControlService* service);

  TrafficControlNavigationThrottle(
      content::NavigationThrottleRegistry& registry,
      TrafficControlService& service);
  ~TrafficControlNavigationThrottle() override;

  TrafficControlNavigationThrottle(const TrafficControlNavigationThrottle&) =
      delete;
  TrafficControlNavigationThrottle& operator=(
      const TrafficControlNavigationThrottle&) = delete;

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult MaybeReroute();

  // Returns the URL to reopen at |target|. On a server redirect this is the
  // URL the request started at, so that a cross-site hand-off (e.g. a sign-in
  // redirect carrying a token) runs again with the target's cookies instead of
  // carrying the source's session across.
  GURL GetRerouteUrl(const mojom::Target& target);

  const raw_ref<TrafficControlService> service_;
};

}  // namespace traffic_control

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_NAVIGATION_THROTTLE_H_
