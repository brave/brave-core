/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_

namespace brave_ads {

// Returns the number of days in `month` for `year`, accounting for leap years.
// Expects a 1-based month (1 = January, ..., 12 = December).
int DaysInMonth(int year, int month);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
