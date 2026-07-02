// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"
#include "content/public/browser/navigation_throttle.h"

class GURL;

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace brave_shields {

class AdBlockService;
class BraveShieldsSettingsService;
class AdBlockCustomFiltersProvider;

class DomainBlockNavigationThrottle : public content::NavigationThrottle {
 public:
  struct BlockResult;
  explicit DomainBlockNavigationThrottle(
      content::NavigationThrottleRegistry& registry,
      BraveShieldsSettingsService* brave_shields_settings_service,
      AdBlockService* ad_block_service,
      AdBlockCustomFiltersProvider* ad_block_custom_filters_provider,
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const std::string& locale);
  ~DomainBlockNavigationThrottle() override;

  DomainBlockNavigationThrottle(const DomainBlockNavigationThrottle&) = delete;
  DomainBlockNavigationThrottle& operator=(
      const DomainBlockNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(
      content::NavigationThrottleRegistry& registry,
      BraveShieldsSettingsService* brave_shields_settings_service,
      AdBlockService* ad_block_service,
      AdBlockCustomFiltersProvider* ad_block_custom_filters_provider,
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      const std::string& locale);

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  content::NavigationThrottle::ThrottleCheckResult WillProcessResponse()
      override;
  const char* GetNameForLogging() override;

 private:
  void OnShouldBlockDomain(DomainBlockingType domain_blocking_type,
                           BlockResult should_block_domain);
  void ShowInterstitial();
  void Enable1PESAndResume();
  void On1PESState(bool is_1pes_enabled);
  void RestartNavigation(const GURL& url);

  const raw_ptr<BraveShieldsSettingsService> brave_shields_settings_service_ =
      nullptr;
  const raw_ptr<AdBlockService> ad_block_service_ = nullptr;
  const raw_ptr<AdBlockCustomFiltersProvider>
      ad_block_custom_filters_provider_ = nullptr;
  const raw_ptr<ephemeral_storage::EphemeralStorageService>
      ephemeral_storage_service_ = nullptr;
  std::string locale_;

  base::WeakPtrFactory<DomainBlockNavigationThrottle> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_
