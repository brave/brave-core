/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_RANDOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_RANDOM_UTIL_H_

// Jitters a time delta by a random factor in [0.5, 1.5). Used to prevent
// thundering-herd effects when scheduling ads-related timers.

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

base::TimeDelta RandTimeDeltaWithJitter(base::TimeDelta time_delta);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RANDOM_RANDOM_UTIL_H_
