/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"

namespace brave_ads {

namespace {

bool MatchDayOfWeek(const CreativeDaypartInfo& daypart,
                    const char day_of_week) {
  return daypart.days_of_week.find(day_of_week) != std::string::npos;
}

bool MatchTimeSlot(const CreativeDaypartInfo& daypart, const int minutes) {
  return minutes >= daypart.start_minute && minutes <= daypart.end_minute;
}

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  if (creative_ad.dayparts.empty()) {
    // Always respect cap if there are no dayparts specified
    return true;
  }

  const base::Time now = base::Time::Now();

  const int day_of_week = DayOfWeek(now, /*is_local=*/true);

  const int local_time_in_minutes = GetLocalTimeInMinutes(now);

  return base::ranges::any_of(
      creative_ad.dayparts,
      [day_of_week, local_time_in_minutes](const CreativeDaypartInfo& daypart) {
        return MatchDayOfWeek(daypart, static_cast<char>('0' + day_of_week)) &&
               MatchTimeSlot(daypart, local_time_in_minutes);
      });
}

}  // namespace

std::string DaypartExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string> DaypartExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded as not within a scheduled time slot",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
