/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_HELPER_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/user_engagement/ad_events/ad_event_cache.h"

namespace base {
template <typename T>
class NoDestructor;
class Time;
}  // namespace base

namespace brave_ads {

class AdEventCacheHelper {
 public:
  AdEventCacheHelper(const AdEventCacheHelper&) = delete;
  AdEventCacheHelper& operator=(const AdEventCacheHelper&) = delete;

  AdEventCacheHelper(AdEventCacheHelper&&) noexcept = delete;
  AdEventCacheHelper& operator=(AdEventCacheHelper&&) noexcept = delete;

  static AdEventCacheHelper* GetInstance();

  void CacheAdEventForInstanceId(
      const std::string& id,
      mojom::AdType mojom_ad_type,
      mojom::ConfirmationType mojom_confirmation_type,
      base::Time time);

  std::vector<base::Time> GetCachedAdEvents(
      mojom::AdType mojom_ad_type,
      mojom::ConfirmationType mojom_confirmation_type) const;

  void ResetAdEventCacheForInstanceId(const std::string& id);

 private:
  friend base::NoDestructor<AdEventCacheHelper>;

  AdEventCacheHelper();

  ~AdEventCacheHelper();

  AdEventCache ad_event_cache_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_HELPER_H_
