/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

int LocalTimeInMinutesSinceMidnight(base::Time time);

base::Time LocalTimeAtBeginningOfPreviousMonth();
base::Time LocalTimeAtEndOfPreviousMonth();
base::Time LocalTimeAtBeginningOfThisMonth();
base::Time LocalTimeAtEndOfThisMonth();

// Formats a time compliant with ISO 8601 in UTC, e.g.,
// "2020-12-31T23:00:00.000Z", with fixed values for minutes, seconds,
// milliseconds, and time zone.
std::string TimeToPrivacyPreservingIso8601(base::Time time);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
