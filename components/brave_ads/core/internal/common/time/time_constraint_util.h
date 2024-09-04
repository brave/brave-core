/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_CONSTRAINT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_CONSTRAINT_UTIL_H_

#include <cstddef>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

// Check if the `history` of time points respects a certain `time_constraint`
// and a `cap` on the number of elements. Assumes that `history` is in
// chronological order.
bool DoesHistoryRespectRollingTimeConstraint(
    const std::vector<base::Time>& history,
    base::TimeDelta time_constraint,
    size_t cap);

bool DoesHistoryRespectRollingTimeConstraint(mojom::AdType mojom_ad_type,
                                             base::TimeDelta time_constraint,
                                             size_t cap);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TIME_TIME_CONSTRAINT_UTIL_H_
