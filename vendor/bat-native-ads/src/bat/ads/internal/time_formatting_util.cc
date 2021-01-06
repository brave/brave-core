/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_formatting_util.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"

namespace ads {

std::string LongFriendlyDateAndTime(
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
    return LongFriendlyDateAndTime(time, use_sentence_style);
  }

  // Show date and time as "in 0 hours, 21 minutes, 58 seconds at 3:07 PM"
  base::string16 time_duration;
  if (!base::TimeDurationFormatWithSeconds(time_delta,
      base::DURATION_WIDTH_WIDE, &time_duration)) {
    return LongFriendlyDateAndTime(time);
  }

  const base::string16 time_of_day = base::TimeFormatTimeOfDay(time);

  return base::StringPrintf("%s%s at %s", use_sentence_style ? "in " : "",
      base::UTF16ToUTF8(time_duration).c_str(),
          base::UTF16ToUTF8(time_of_day).c_str());
}

std::string FriendlyDateAndTime(
    const int64_t timestamp,
    const bool use_sentence_style) {
  const base::Time time = base::Time::FromDoubleT(timestamp);
  return FriendlyDateAndTime(time, use_sentence_style);
}

std::string TimeToISO8601(
    const base::Time& time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);

  return base::StringPrintf("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
      exploded.year, exploded.month, exploded.day_of_month, exploded.hour,
          exploded.minute, exploded.second, exploded.millisecond);
}

std::string TimestampToISO8601(
    const int64_t timestamp) {
  const base::Time time = base::Time::FromDoubleT(timestamp);
  return TimeToISO8601(time);
}

std::string TimeAsTimestampString(
    const base::Time& time) {
  const int64_t timestamp = static_cast<int64_t>(time.ToDoubleT());
  return base::NumberToString(timestamp);
}

}  // namespace ads
