/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"

#include <tuple>

namespace brave_ads {

CreativeSetConversionInfo::CreativeSetConversionInfo() = default;

CreativeSetConversionInfo::CreativeSetConversionInfo(
    const CreativeSetConversionInfo& other) = default;

CreativeSetConversionInfo& CreativeSetConversionInfo::operator=(
    const CreativeSetConversionInfo& other) = default;

CreativeSetConversionInfo::CreativeSetConversionInfo(
    CreativeSetConversionInfo&& other) noexcept = default;

CreativeSetConversionInfo& CreativeSetConversionInfo::operator=(
    CreativeSetConversionInfo&& other) noexcept = default;

CreativeSetConversionInfo::~CreativeSetConversionInfo() = default;

bool CreativeSetConversionInfo::IsValid() const {
  return !id.empty() && !url_pattern.empty() &&
         !observation_window.is_negative() && !expire_at.is_null();
}

bool operator==(const CreativeSetConversionInfo& lhs,
                const CreativeSetConversionInfo& rhs) {
  const auto tie =
      [](const CreativeSetConversionInfo& creative_set_conversion) {
        return std::tie(
            creative_set_conversion.id, creative_set_conversion.url_pattern,
            creative_set_conversion.verifiable_advertiser_public_key_base64,
            creative_set_conversion.observation_window,
            creative_set_conversion.expire_at);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeSetConversionInfo& lhs,
                const CreativeSetConversionInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
