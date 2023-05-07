/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"

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
  return creative_set_id == other.creative_set_id && type == other.type &&
         url_pattern == other.url_pattern &&
         observation_window == other.observation_window &&
         advertiser_public_key == other.advertiser_public_key &&
         expire_at == other.expire_at;
}

bool ConversionInfo::operator!=(const ConversionInfo& other) const {
  return !(*this == other);
}

bool ConversionInfo::IsValid() const {
  return !creative_set_id.empty() && !type.empty() && !url_pattern.empty() &&
         !expire_at.is_null();
}

}  // namespace brave_ads
