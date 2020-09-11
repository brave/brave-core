/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONSTANTS_H_
#define BRAVELEDGER_CONSTANTS_H_

#include "base/time/time.h"

namespace ledger {
namespace constant {

const char kClearFavicon[] = "clear";

const char kIgnorePublisherBlob[] = "ignore";

const uint64_t kReconcileInterval =
    30 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const uint64_t kPromotionRefreshInterval =
    base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const uint64_t kPendingContributionExpirationInterval =
    90 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour;

const double kVotePrice = 0.25;

const int kMinVisitTime = 8;

}  // namespace constant
}  // namespace ledger

#endif  // BRAVELEDGER_CONSTANTS_H_
