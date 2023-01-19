/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"

namespace ads {

bool operator==(const ConversionIdPatternInfo& lhs,
                const ConversionIdPatternInfo& rhs) {
  return lhs.id_pattern == rhs.id_pattern &&
         lhs.url_pattern == rhs.url_pattern && lhs.search_in == rhs.search_in;
}

bool operator!=(const ConversionIdPatternInfo& lhs,
                const ConversionIdPatternInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
