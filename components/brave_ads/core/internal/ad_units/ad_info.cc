/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

AdInfo::AdInfo() = default;

AdInfo::AdInfo(const AdInfo& other) = default;

AdInfo& AdInfo::operator=(const AdInfo& other) = default;

AdInfo::AdInfo(AdInfo&& other) noexcept = default;

AdInfo& AdInfo::operator=(AdInfo&& other) noexcept = default;

AdInfo::~AdInfo() = default;

bool AdInfo::IsValid() const {
  return type != mojom::AdType::kUndefined && !placement_id.empty() &&
         !creative_instance_id.empty() && !creative_set_id.empty() &&
         !campaign_id.empty() && !advertiser_id.empty() && !segment.empty() &&
         target_url.is_valid();
}

}  // namespace brave_ads
