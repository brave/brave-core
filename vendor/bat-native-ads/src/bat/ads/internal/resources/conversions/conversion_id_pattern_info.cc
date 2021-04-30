/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/conversion_id_pattern_info.h"

namespace ads {

ConversionIdPatternInfo::ConversionIdPatternInfo() = default;

ConversionIdPatternInfo::ConversionIdPatternInfo(
    const ConversionIdPatternInfo& info) = default;

ConversionIdPatternInfo::~ConversionIdPatternInfo() = default;

bool ConversionIdPatternInfo::operator==(
    const ConversionIdPatternInfo& rhs) const {
  return id_pattern == rhs.id_pattern && url_pattern == rhs.url_pattern &&
         search_in == rhs.search_in;
}

bool ConversionIdPatternInfo::operator!=(
    const ConversionIdPatternInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
