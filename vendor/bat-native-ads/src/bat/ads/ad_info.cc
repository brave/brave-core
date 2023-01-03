/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

namespace ads {

AdInfo::AdInfo() = default;

AdInfo::AdInfo(const AdInfo& other) = default;

AdInfo& AdInfo::operator=(const AdInfo& other) = default;

AdInfo::AdInfo(AdInfo&& other) noexcept = default;

AdInfo& AdInfo::operator=(AdInfo&& other) noexcept = default;

AdInfo::~AdInfo() = default;

bool AdInfo::operator==(const AdInfo& other) const {
  return type == other.type && placement_id == other.placement_id &&
         creative_instance_id == other.creative_instance_id &&
         creative_set_id == other.creative_set_id &&
         campaign_id == other.campaign_id &&
         advertiser_id == other.advertiser_id && segment == other.segment &&
         target_url == other.target_url;
}

bool AdInfo::operator!=(const AdInfo& other) const {
  return !(*this == other);
}

bool AdInfo::IsValid() const {
  return type != AdType::kUndefined && !placement_id.empty() &&
         !creative_instance_id.empty() && !creative_set_id.empty() &&
         !campaign_id.empty() && !advertiser_id.empty() && !segment.empty() &&
         target_url.is_valid();
}

}  // namespace ads
