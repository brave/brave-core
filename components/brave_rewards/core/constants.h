/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONSTANTS_H_

#include "base/time/time.h"

namespace brave_rewards::internal {
namespace constant {

inline constexpr char kClearFavicon[] = "clear";

inline constexpr char kIgnorePublisherBlob[] = "ignore";

const uint64_t kReconcileInterval =
    30 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const uint64_t kPromotionRefreshInterval =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const uint64_t kPendingContributionExpirationInterval =
    90 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const double kVotePrice = 0.25;

const int kMinVisitTime = 8;

}  // namespace constant
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONSTANTS_H_
