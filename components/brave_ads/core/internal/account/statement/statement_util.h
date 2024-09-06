/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_UTIL_H_

#include <cstdint>
#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

base::Time GetNextPaymentDate(const TransactionList& transactions);

std::pair</*range_low*/ double, /*range_high*/ double>
GetEstimatedEarningsForThisMonth(const TransactionList& transactions);

std::pair</*range_low*/ double, /*range_high*/ double>
GetEstimatedEarningsForLastMonth(const TransactionList& transactions);

int32_t GetAdsReceivedThisMonth(const TransactionList& transactions);

base::flat_map<mojom::AdType, /*count*/ int32_t> GetAdsSummaryThisMonth(
    const TransactionList& transactions);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_STATEMENT_UTIL_H_
