/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"

namespace ads {

std::string LongFormatFriendlyDateAndTime(
    const base::Time& time,
    const bool use_sentence_style) {
  const base::string16 friendly_date_and_time =
      base::TimeFormatFriendlyDateAndTime(time);

  // Show date and time as "on Tuesday, 5 May 2020 at 2:44:30 PM" if over 1 day
  return base::StringPrintf("%s%s", use_sentence_style ? "on " : "",
      base::UTF16ToUTF8(friendly_date_and_time).c_str());
}

std::string FriendlyDateAndTime(
    const base::Time& time,
    const bool use_sentence_style) {
  const base::TimeDelta time_delta = time - base::Time::Now();

  if (time_delta.InDays() > 0) {
    return LongFormatFriendlyDateAndTime(time, use_sentence_style);
  }

  // Show date and time as "in 0 hours, 21 minutes, 58 seconds at 3:07 PM"
  base::string16 time_duration;
  if (!base::TimeDurationFormatWithSeconds(time_delta,
      base::DURATION_WIDTH_WIDE, &time_duration)) {
    return LongFormatFriendlyDateAndTime(time);
  }

  const base::string16 time_of_day = base::TimeFormatTimeOfDay(time);

  return base::StringPrintf("%s%s at %s", use_sentence_style ? "in " : "",
      base::UTF16ToUTF8(time_duration).c_str(),
          base::UTF16ToUTF8(time_of_day).c_str());
}

std::string FriendlyDateAndTime(
    const uint64_t timestamp_in_seconds,
    const bool use_sentence_style) {
  const base::Time time = base::Time::FromDoubleT(timestamp_in_seconds);
  return FriendlyDateAndTime(time, use_sentence_style);
}

uint64_t MigrateTimestampToDoubleT(
    const uint64_t timestamp_in_seconds) {
  if (timestamp_in_seconds < 10000000000) {
    // Already migrated as DoubleT will never reach 10000000000 in our lifetime
    // and legacy timestamps are above 10000000000
    return timestamp_in_seconds;
  }

  // Migrate date to DoubleT
  const base::Time now = base::Time::Now();

  const uint64_t now_in_seconds =
      static_cast<uint64_t>((now - base::Time()).InSeconds());

  const uint64_t delta = timestamp_in_seconds - now_in_seconds;

  const base::Time time = now + base::TimeDelta::FromSeconds(delta);
  return static_cast<uint64_t>(time.ToDoubleT());
}

std::string NowAsString() {
  const int64_t timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());
  return base::NumberToString(timestamp);
}

std::string GetLocalDayOfWeek(
    const base::Time& time) {
  base::Time::Exploded exploded;

  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return base::NumberToString(exploded.day_of_week);
}

int ConvertTimeToLocalMinutesForToday(
    const base::Time& time) {
  base::Time::Exploded exploded;

  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return (exploded.hour * base::Time::kMinutesPerHour) + exploded.minute;
}

}  // namespace ads
