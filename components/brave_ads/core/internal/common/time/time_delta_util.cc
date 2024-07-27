/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_delta_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

namespace brave_ads {

base::TimeDelta Months(const int n) {
  CHECK_GE(n, 0) << "Do not dwell in the past";

  base::Time::Exploded now_exploded;
  base::Time::Now().UTCExplode(&now_exploded);

  int year = now_exploded.year;
  int month = now_exploded.month;

  int days = 0;

  for (int i = 0; i < n; ++i) {
    days += DaysPerMonth(year, month);

    ++month;
    if (month > 12) {
      month = 1;
      ++year;
    }
  }

  return base::Days(days);
}

}  // namespace brave_ads
