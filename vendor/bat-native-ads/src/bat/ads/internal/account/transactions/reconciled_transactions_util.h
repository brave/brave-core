/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_RECONCILED_TRANSACTIONS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_RECONCILED_TRANSACTIONS_UTIL_H_

#include "bat/ads/transaction_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

struct TransactionInfo;

bool DidReconcileTransaction(const TransactionInfo& transaction);
bool DidReconcileTransactionsLastMonth(const TransactionList& transactions);
bool DidReconcileTransactionsThisMonth(const TransactionList& transactions);
bool DidReconcileTransactionWithinDateRange(const TransactionInfo& transaction,
                                            const base::Time from_time,
                                            const base::Time to_time);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_RECONCILED_TRANSACTIONS_UTIL_H_
