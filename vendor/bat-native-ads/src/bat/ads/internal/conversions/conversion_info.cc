/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

ConversionInfo::ConversionInfo() = default;

ConversionInfo::ConversionInfo(
    const ConversionInfo& info) = default;

ConversionInfo::~ConversionInfo() = default;

bool ConversionInfo::operator==(
    const ConversionInfo& rhs) const {
  return creative_set_id == rhs.creative_set_id &&
      type == rhs.type &&
      url_pattern == rhs.url_pattern &&
      observation_window == rhs.observation_window &&
      expiry_timestamp == rhs.expiry_timestamp;
}

bool ConversionInfo::operator!=(
    const ConversionInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
