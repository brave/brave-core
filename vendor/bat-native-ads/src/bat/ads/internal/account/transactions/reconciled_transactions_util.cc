/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/reconciled_transactions_util.h"

#include <algorithm>

#include "base/time/time.h"
#include "bat/ads/internal/base/time/time_util.h"

namespace ads {

namespace {

bool HasReconciledTransactionsForDateRange(const TransactionList& transactions,
                                           const base::Time from_time,
                                           const base::Time to_time) {
  const int count =
      std::count_if(transactions.cbegin(), transactions.cend(),
                    [from_time, to_time](const TransactionInfo& transaction) {
                      return DidReconcileTransactionWithinDateRange(
                          transaction, from_time, to_time);
                    });

  if (count == 0) {
    return false;
  }

  return true;
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
