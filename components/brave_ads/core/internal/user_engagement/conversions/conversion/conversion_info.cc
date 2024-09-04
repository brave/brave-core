/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

ConversionInfo::ConversionInfo() = default;

ConversionInfo::ConversionInfo(const ConversionInfo& other) = default;

ConversionInfo& ConversionInfo::operator=(const ConversionInfo& other) =
    default;

ConversionInfo::ConversionInfo(ConversionInfo&& other) noexcept = default;

ConversionInfo& ConversionInfo::operator=(ConversionInfo&& other) noexcept =
    default;

ConversionInfo::~ConversionInfo() = default;

bool ConversionInfo::IsValid() const {
  return ad_type != mojom::AdType::kUndefined &&
         !creative_instance_id.empty() && !creative_set_id.empty() &&
         !campaign_id.empty() && !advertiser_id.empty() &&
         action_type != ConversionActionType::kUndefined;
}

}  // namespace brave_ads
