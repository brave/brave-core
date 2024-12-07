/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule_test_util.h"

namespace brave_ads::test {

int ConvertHoursAndMinutesToTotalMinutes(int hours, int minutes) {
  CHECK(hours >= 0 && hours <= 23);
  CHECK(minutes >= 0 && minutes <= 59);

  return hours * static_cast<int>(base::Time::kMinutesPerHour) + minutes;
}

}  // namespace brave_ads::test
