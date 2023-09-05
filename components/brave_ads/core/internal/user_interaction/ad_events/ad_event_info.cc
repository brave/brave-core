/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_info.h"

#include <tuple>

namespace brave_ads {

AdEventInfo::AdEventInfo() = default;

AdEventInfo::AdEventInfo(const AdEventInfo& other) = default;

AdEventInfo& AdEventInfo::operator=(const AdEventInfo& other) = default;

AdEventInfo::AdEventInfo(AdEventInfo&& other) noexcept = default;

AdEventInfo& AdEventInfo::operator=(AdEventInfo&& other) noexcept = default;

AdEventInfo::~AdEventInfo() = default;

bool AdEventInfo::IsValid() const {
  return type != AdType::kUndefined &&
         confirmation_type != ConfirmationType::kUndefined &&
         !placement_id.empty() && !creative_instance_id.empty() &&
         !creative_set_id.empty() && !campaign_id.empty() &&
         !created_at.is_null();
}

bool operator==(const AdEventInfo& lhs, const AdEventInfo& rhs) {
  const auto tie = [](const AdEventInfo& ad_event) {
    return std::tie(ad_event.type, ad_event.confirmation_type,
                    ad_event.placement_id, ad_event.creative_instance_id,
                    ad_event.creative_set_id, ad_event.campaign_id,
                    ad_event.advertiser_id, ad_event.segment,
                    ad_event.created_at);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const AdEventInfo& lhs, const AdEventInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
