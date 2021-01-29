/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_NAVIGATION_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

class PrefService;

namespace unstoppable_domains {

class UnstoppableDomainsService;

class UnstoppableDomainsNavigationThrottle
    : public content::NavigationThrottle {
 public:
  explicit UnstoppableDomainsNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      UnstoppableDomainsService* unstoppable_domains_service,
      PrefService* local_state,
      const std::string& locale);
  ~UnstoppableDomainsNavigationThrottle() override;

  UnstoppableDomainsNavigationThrottle(
      const UnstoppableDomainsNavigationThrottle&) = delete;
  UnstoppableDomainsNavigationThrottle& operator=(
      const UnstoppableDomainsNavigationThrottle&) = delete;

  static std::unique_ptr<UnstoppableDomainsNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle,
                         UnstoppableDomainsService* unstoppable_domains_service,
                         PrefService* local_state,
                         const std::string& locale);

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  void ShowInterstitial();

  UnstoppableDomainsService* unstoppable_domains_service_ = nullptr;
  PrefService* user_prefs_ = nullptr;
  PrefService* local_state_ = nullptr;
  std::string locale_;
  base::WeakPtrFactory<UnstoppableDomainsNavigationThrottle> weak_ptr_factory_{
      this};
};

}  // namespace unstoppable_domains

#endif  // BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_NAVIGATION_THROTTLE_H_
