/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_DELTA_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_DELTA_UTIL_H_

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

// Returns a `base::TimeDelta` duration equivalent to `n` months from the
// current time with support for leap years.
base::TimeDelta Months(int n);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_DELTA_UTIL_H_
