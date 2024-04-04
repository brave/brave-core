/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_

#include <cstdint>
#include <string>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

int64_t ToChromeTimestampFromTime(base::Time time);
base::Time ToTimeFromChromeTimestamp(int64_t timestamp);

int GetLocalTimeInMinutes(base::Time time);

base::Time AdjustLocalTimeToBeginningOfPreviousMonth(base::Time time);
base::Time AdjustLocalTimeToEndOfPreviousMonth(base::Time time);
base::Time AdjustLocalTimeToBeginningOfMonth(base::Time time);
base::Time AdjustLocalTimeToEndOfMonth(base::Time time);

base::Time GetTimeInDistantPast();

base::Time GetLocalTimeAtBeginningOfLastMonth();
base::Time GetLocalTimeAtEndOfLastMonth();
base::Time GetLocalTimeAtBeginningOfThisMonth();
base::Time GetLocalTimeAtEndOfThisMonth();

std::string TimeToPrivacyPreservingIso8601(base::Time time);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
