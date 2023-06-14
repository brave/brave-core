/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"

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

bool ConversionInfo::operator==(const ConversionInfo& other) const {
  const auto tie = [](const ConversionInfo& conversion) {
    return std::tie(conversion.creative_set_id, conversion.type,
                    conversion.url_pattern, conversion.observation_window,
                    conversion.advertiser_public_key, conversion.expire_at);
  };

  return tie(*this) == tie(other);
}

bool ConversionInfo::operator!=(const ConversionInfo& other) const {
  return !(*this == other);
}

bool ConversionInfo::IsValid() const {
  return !creative_set_id.empty() && !type.empty() && !url_pattern.empty() &&
         !expire_at.is_null();
}

}  // namespace brave_ads
