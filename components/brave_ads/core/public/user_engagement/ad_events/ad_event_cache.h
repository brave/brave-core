/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

class ADS_EXPORT AdEventCache final {
 public:
  AdEventCache();

  AdEventCache(const AdEventCache&) = delete;
  AdEventCache& operator=(const AdEventCache&) = delete;

  AdEventCache(AdEventCache&&) noexcept = delete;
  AdEventCache& operator=(AdEventCache&&) noexcept = delete;

  ~AdEventCache();

  void AddEntryForInstanceId(const std::string& id,
                             mojom::AdType mojom_ad_type,
                             mojom::ConfirmationType mojom_confirmation_type,
                             base::Time time);

  std::vector<base::Time> Get(
      mojom::AdType mojom_ad_type,
      mojom::ConfirmationType mojom_confirmation_type) const;

  void ResetForInstanceId(const std::string& id);

 private:
  base::flat_map<
      /*id*/ std::string,
      base::flat_map</*type_id*/ std::string, std::vector<base::Time>>>
      ad_event_cache_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_CACHE_H_
