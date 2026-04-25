/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Returns the number of minutes elapsed since local midnight for `time`.
int MinutesElapsedSinceLocalMidnight(base::Time time);

// Returns midnight on the first day of the previous calendar month in local
// time, e.g., 2020-10-01 00:00:00.000 local when now is in November 2020.
base::Time LocalTimeAtBeginningOfPreviousMonth();

// Returns 23:59:59.999 on the last day of the previous calendar month in local
// time, e.g., 2020-10-31 23:59:59.999 local when now is in November 2020.
base::Time LocalTimeAtEndOfPreviousMonth();

// Returns midnight on the first day of the current calendar month in local
// time, e.g., 2020-11-01 00:00:00.000 local when now is in November 2020.
base::Time LocalTimeAtBeginningOfThisMonth();

// Returns 23:59:59.999 on the last day of the current calendar month in local
// time, e.g., 2020-11-30 23:59:59.999 local when now is in November 2020.
base::Time LocalTimeAtEndOfThisMonth();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
