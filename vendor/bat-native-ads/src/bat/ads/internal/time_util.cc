/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include "base/check.h"
#include "base/strings/string_number_conversions.h"

namespace ads {

std::string GetLocalWeekDay(const base::Time& time) {
  base::Time::Exploded exploded;

  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return base::NumberToString(exploded.day_of_week);
}

int ConvertHoursAndMinutesToMinutes(const base::Time& time) {
  base::Time::Exploded exploded;

  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return (exploded.hour * base::Time::kMinutesPerHour) + exploded.minute;
}

}  // namespace ads
