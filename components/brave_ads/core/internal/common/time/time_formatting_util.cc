/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"

namespace brave_ads {

std::string LongFriendlyDateAndTime(base::Time time, bool use_sentence_style) {
  const std::u16string friendly_date_and_time =
      base::TimeFormatFriendlyDateAndTime(time);

  // Show date and time as "on Tuesday, 5 May 2020 at 2:44:30 PM" if over 1 day
  return base::StrCat({use_sentence_style ? "on " : "",
                       base::UTF16ToUTF8(friendly_date_and_time)});
}

std::string FriendlyDateAndTime(base::Time time, bool use_sentence_style) {
  base::TimeDelta time_delta = time - base::Time::Now();
  if (time_delta.is_negative()) {
    time_delta = {};
  }

  if (time_delta.InDays() > 0) {
    return LongFriendlyDateAndTime(time, use_sentence_style);
  }

  // Show date and time as "in 0 hours, 21 minutes, 58 seconds at 15:07:30.568"
  std::u16string time_duration;
  if (!base::TimeDurationFormatWithSeconds(
          time_delta, base::DURATION_WIDTH_WIDE, &time_duration)) {
    return LongFriendlyDateAndTime(time);
  }

  const std::u16string time_of_day =
      base::TimeFormatTimeOfDayWithMilliseconds(time);

  return base::ReplaceStringPlaceholders(
      "$1$2 at $3",
      {use_sentence_style ? "in " : "", base::UTF16ToUTF8(time_duration),
       base::UTF16ToUTF8(time_of_day)},
      nullptr);
}

}  // namespace brave_ads
