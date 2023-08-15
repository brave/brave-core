/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/resource/conversion_resource_id_pattern_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const ConversionResourceIdPatternInfo& lhs,
                const ConversionResourceIdPatternInfo& rhs) {
  const auto tie =
      [](const ConversionResourceIdPatternInfo& conversion_id_pattern) {
        return std::tie(conversion_id_pattern.url_pattern,
                        conversion_id_pattern.search_in_type,
                        conversion_id_pattern.id_pattern);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const ConversionResourceIdPatternInfo& lhs,
                const ConversionResourceIdPatternInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
