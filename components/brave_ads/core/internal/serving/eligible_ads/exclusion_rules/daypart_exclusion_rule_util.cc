/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule_util.h"

#include "base/check.h"
#include "base/containers/contains.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"

namespace brave_ads {

bool MatchDayOfWeek(const CreativeDaypartInfo& daypart, int day_of_week) {
  CHECK(day_of_week >= 0 && day_of_week <= 6);

  const char day_of_week_as_char = static_cast<char>('0' + day_of_week);
  return base::Contains(daypart.days_of_week, day_of_week_as_char);
}

bool MatchTimeSlot(const CreativeDaypartInfo& daypart, int minutes) {
  return minutes >= daypart.start_minute && minutes <= daypart.end_minute;
}

}  // namespace brave_ads
