/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_ADS_SUMMARY_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_ADS_SUMMARY_UTIL_H_

#include <cstdint>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

base::flat_map<mojom::AdType, /*count*/ int32_t> GetAdsSummaryForDateRange(
    const TransactionList& transactions,
    base::Time from_time,
    base::Time to_time);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_ADS_SUMMARY_UTIL_H_
