/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_UTILS_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_UTILS_H_

#include "base/time/time.h"
#include "brave/vendor/brave_base/random.h"

namespace brave {

base::Time NextMonday(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalMidnight().LocalExplode(&exploded);
  // 1 stands for Monday, 0 for Sunday
  int days_till_monday = 0;
  if (exploded.day_of_week >= 1) {
    days_till_monday = 8 - exploded.day_of_week;
  } else {
    days_till_monday = 1;
  }

  // Adding few hours of padding to prevent potential problems with DST.
  base::Time result =
      (time.LocalMidnight() + base::Days(days_till_monday) + base::Hours(4))
          .LocalMidnight();
  return result;
}

base::TimeDelta GetRandomizedUploadInterval(
    base::TimeDelta average_upload_interval) {
  const auto delta = base::Seconds(
      brave_base::random::Geometric(average_upload_interval.InSecondsF()));
  return delta;
}

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_UTILS_H_
