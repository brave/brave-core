/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_NAVIGATION_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

class PrefService;

namespace decentralized_dns {

class DecentralizedDnsNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit DecentralizedDnsNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      PrefService* local_state,
      const std::string& locale);
  ~DecentralizedDnsNavigationThrottle() override;

  DecentralizedDnsNavigationThrottle(
      const DecentralizedDnsNavigationThrottle&) = delete;
  DecentralizedDnsNavigationThrottle& operator=(
      const DecentralizedDnsNavigationThrottle&) = delete;

  static std::unique_ptr<DecentralizedDnsNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
                         PrefService* local_state,
                         const std::string& locale);

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  void ShowInterstitial();

  PrefService* user_prefs_ = nullptr;
  PrefService* local_state_ = nullptr;
  std::string locale_;
  base::WeakPtrFactory<DecentralizedDnsNavigationThrottle> weak_ptr_factory_{
      this};
};

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_NAVIGATION_THROTTLE_H_
