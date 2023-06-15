/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const CreativeDaypartInfo& lhs,
                const CreativeDaypartInfo& rhs) {
  const auto tie = [](const CreativeDaypartInfo& daypart) {
    return std::tie(daypart.days_of_week, daypart.start_minute,
                    daypart.end_minute);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const CreativeDaypartInfo& lhs,
                const CreativeDaypartInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
