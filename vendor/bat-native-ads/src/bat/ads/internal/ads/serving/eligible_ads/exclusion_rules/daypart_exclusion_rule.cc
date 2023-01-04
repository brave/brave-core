/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/calendar/calendar_util.h"
#include "bat/ads/internal/common/time/time_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

namespace {

bool MatchDayOfWeek(const CreativeDaypartInfo& daypart,
                    const std::string& day_of_week) {
  return daypart.dow.find(day_of_week) != std::string::npos;
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

  const int local_time_as_minutes = GetLocalTimeAsMinutes(now);

  const int day_of_week = GetDayOfWeek(now, /*is_local*/ true);
  const std::string day_of_week_as_string = base::NumberToString(day_of_week);

  return base::ranges::any_of(
      creative_ad.dayparts, [day_of_week_as_string, local_time_as_minutes](
                                const CreativeDaypartInfo& daypart) {
        return MatchDayOfWeek(daypart, day_of_week_as_string) &&
               MatchTimeSlot(daypart, local_time_as_minutes);
      });
}

}  // namespace

std::string DaypartExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool DaypartExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded as not within a scheduled time slot",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

const std::string& DaypartExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
