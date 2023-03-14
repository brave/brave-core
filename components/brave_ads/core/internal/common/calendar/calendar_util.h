/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_

namespace base {
class Time;
}  // namespace base

namespace ads {

// Four digit year "2007", 1-based month (values 1 = January, etc.)
int GetLastDayOfMonth(int year, int month);

// Four digit year "2007", 1-based month (values 1 = January, etc.), 1-based day
// of month (1-31)
int GetDayOfWeek(int year, int month, int day);

int GetDayOfWeek(base::Time time, bool is_local);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
