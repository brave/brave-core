/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

DaypartFrequencyCap::DaypartFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

DaypartFrequencyCap::~DaypartFrequencyCap() = default;

bool DaypartFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s excluded as not "
        "within the scheduled timeslot", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string DaypartFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DaypartFrequencyCap::DoesRespectCap(
    const CreativeAdInfo& ad) const {
  // If there's no day part specified, let it be displayed
  if (ad.dayparts.empty()) {
    return true;
  }

  const std::string current_dow = DaypartFrequencyCap::GetCurrentDayOfWeek();
  std::string days_of_week;
  int current_minutes_from_start =
    DaypartFrequencyCap::GetCurrentLocalMinutesFromStart();
  int start_time;
  int end_time;

  for (const CreativeDaypartInfo& daypart : ad.dayparts) {
      days_of_week = daypart.dow;
      start_time = daypart.start_minute;
      end_time = daypart.end_minute;

      if (DaypartFrequencyCap::HasDayOfWeekMatch(current_dow, days_of_week)
          && DaypartFrequencyCap::HasTimeSlotMatch(
              current_minutes_from_start,
              start_time, end_time)) {
          return true;
      }
  }
  return false;
}

bool DaypartFrequencyCap::HasDayOfWeekMatch(
    const std::string& current_dow,
    const std::string& days_of_week) const {
  return days_of_week.find(current_dow) != std::string::npos;
}

bool DaypartFrequencyCap::HasTimeSlotMatch(
    const int current_minutes_from_start,
    const int start_time,
    const int end_time) const {

    return start_time <= current_minutes_from_start &&
      current_minutes_from_start <= end_time;
}

std::string DaypartFrequencyCap::GetCurrentDayOfWeek() const {
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  return base::NumberToString(exploded.day_of_week);
}

int DaypartFrequencyCap::GetCurrentLocalMinutesFromStart() const {
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  return (base::Time::kMinutesPerHour * exploded.hour) + exploded.minute;
}

}  // namespace ads
