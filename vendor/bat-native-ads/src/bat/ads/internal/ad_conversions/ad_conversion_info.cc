/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_conversions/ad_conversion_info.h"

namespace ads {

AdConversionInfo::AdConversionInfo() = default;

AdConversionInfo::AdConversionInfo(
    const AdConversionInfo& info) = default;

AdConversionInfo::~AdConversionInfo() = default;

bool AdConversionInfo::operator==(
    const AdConversionInfo& rhs) const {
  return creative_set_id == rhs.creative_set_id &&
      type == rhs.type &&
      url_pattern == rhs.url_pattern &&
      observation_window == rhs.observation_window &&
      expiry_timestamp == rhs.expiry_timestamp;
}

bool AdConversionInfo::operator!=(
    const AdConversionInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
