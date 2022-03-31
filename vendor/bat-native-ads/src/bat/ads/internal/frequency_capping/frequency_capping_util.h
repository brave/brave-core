/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_

#include <deque>

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace ads {

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<base::Time>& history,
    const base::TimeDelta time_constraint,
    const int cap);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UTIL_H_
