/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_info.h"

#include <tuple>

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
  return ad_type != AdType::kUndefined && !creative_instance_id.empty() &&
         !creative_set_id.empty() && !campaign_id.empty() &&
         !advertiser_id.empty() &&
         action_type != ConversionActionType::kUndefined;
}

bool operator==(const ConversionInfo& lhs, const ConversionInfo& rhs) {
  const auto tie = [](const ConversionInfo& conversion) {
    return std::tie(conversion.ad_type, conversion.creative_instance_id,
                    conversion.creative_set_id, conversion.campaign_id,
                    conversion.advertiser_id, conversion.segment,
                    conversion.action_type, conversion.verifiable);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const ConversionInfo& lhs, const ConversionInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
