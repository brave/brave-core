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

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time GetLocalMidnight(base::Time time);

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
void SetFromLocalExplodedFailedForTesting(bool set_failed);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_UTIL_H_
