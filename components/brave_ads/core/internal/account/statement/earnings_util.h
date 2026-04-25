/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_EARNINGS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_EARNINGS_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"

namespace brave_ads {

double GetUnreconciledEarnings(const TransactionList& transactions);
double GetReconciledEarningsForThisMonth(const TransactionList& transactions);
double GetReconciledEarningsForPreviousMonth(
    const TransactionList& transactions);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_STATEMENT_EARNINGS_UTIL_H_
