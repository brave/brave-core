/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/reconciled_transactions_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/time/time_util.h"

namespace ads {

namespace {

bool HasReconciledTransactionsForDateRange(const TransactionList& transactions,
                                           const base::Time from_time,
                                           const base::Time to_time) {
  const int count = base::ranges::count_if(
      transactions, [from_time, to_time](const TransactionInfo& transaction) {
        return DidReconcileTransactionWithinDateRange(transaction, from_time,
                                                      to_time);
      });

  return count != 0;
}

}  // namespace

bool DidReconcileTransaction(const TransactionInfo& transaction) {
  return !transaction.reconciled_at.is_null();
}

bool DidReconcileTransactionsLastMonth(const TransactionList& transactions) {
  const base::Time from_time = GetLocalTimeAtBeginningOfLastMonth();
  const base::Time to_time = GetLocalTimeAtEndOfLastMonth();

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransactionsThisMonth(const TransactionList& transactions) {
  const base::Time from_time = GetLocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransactionWithinDateRange(const TransactionInfo& transaction,
                                            const base::Time from_time,
                                            const base::Time to_time) {
  if (!DidReconcileTransaction(transaction)) {
    return false;
  }

  if (transaction.reconciled_at < from_time ||
      transaction.reconciled_at > to_time) {
    return false;
  }

  return true;
}

}  // namespace ads
