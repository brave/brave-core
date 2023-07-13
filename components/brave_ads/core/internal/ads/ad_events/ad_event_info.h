/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"

namespace brave_ads {

struct AdEventInfo final {
  AdEventInfo();

  AdEventInfo(const AdEventInfo&);
  AdEventInfo& operator=(const AdEventInfo&);

  AdEventInfo(AdEventInfo&&) noexcept;
  AdEventInfo& operator=(AdEventInfo&&) noexcept;

  ~AdEventInfo();

  [[nodiscard]] bool IsValid() const;

  AdType type = AdType::kUndefined;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  std::string placement_id;
  std::string creative_instance_id;
  std::string creative_set_id;
  std::string campaign_id;
  std::string advertiser_id;
  std::string segment;
  base::Time created_at;
};

bool operator==(const AdEventInfo&, const AdEventInfo&);
bool operator!=(const AdEventInfo&, const AdEventInfo&);

using AdEventList = std::vector<AdEventInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_AD_EVENT_INFO_H_
