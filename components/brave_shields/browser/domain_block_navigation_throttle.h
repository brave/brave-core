/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_

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

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace brave_shields {

class AdBlockService;
class AdBlockCustomFiltersSourceProvider;

class DomainBlockNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit DomainBlockNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      AdBlockService* ad_block_service,
      AdBlockCustomFiltersSourceProvider*
          ad_block_custom_filters_source_provider,
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      HostContentSettingsMap* content_settings,
      const std::string& locale);
  ~DomainBlockNavigationThrottle() override;

  DomainBlockNavigationThrottle(const DomainBlockNavigationThrottle&) = delete;
  DomainBlockNavigationThrottle& operator=(
      const DomainBlockNavigationThrottle&) = delete;

  static std::unique_ptr<DomainBlockNavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle,
      AdBlockService* ad_block_service,
      AdBlockCustomFiltersSourceProvider*
          ad_block_custom_filters_source_provider,
      ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
      HostContentSettingsMap* content_settings,
      const std::string& locale);

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  content::NavigationThrottle::ThrottleCheckResult WillProcessResponse()
      override;
  const char* GetNameForLogging() override;

 private:
  void OnShouldBlockDomain(bool should_block_domain);
  void ShowInterstitial();
  void Enable1PESAndResume();

  AdBlockService* ad_block_service_ = nullptr;
  AdBlockCustomFiltersSourceProvider* ad_block_custom_filters_source_provider_ =
      nullptr;
  ephemeral_storage::EphemeralStorageService* ephemeral_storage_service_ =
      nullptr;
  HostContentSettingsMap* content_settings_ = nullptr;
  std::string locale_;

  DomainBlockingType domain_blocking_type_ = DomainBlockingType::kNone;

  base::WeakPtrFactory<DomainBlockNavigationThrottle> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DOMAIN_BLOCK_NAVIGATION_THROTTLE_H_
