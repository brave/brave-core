/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

namespace brave_ads {

namespace {

bool HasReconciledTransactionsForDateRange(const TransactionList& transactions,
                                           base::Time from_time,
                                           base::Time to_time) {
  return base::ranges::any_of(
      transactions, [from_time, to_time](const TransactionInfo& transaction) {
        return DidReconcileTransactionWithinDateRange(transaction, from_time,
                                                      to_time);
      });
}

}  // namespace

bool DidReconcileTransaction(const TransactionInfo& transaction) {
  return !!transaction.reconciled_at;
}

bool DidReconcileTransactionsPreviousMonth(
    const TransactionList& transactions) {
  const base::Time from_time = LocalTimeAtBeginningOfPreviousMonth();
  const base::Time to_time = LocalTimeAtEndOfPreviousMonth();

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransactionsThisMonth(const TransactionList& transactions) {
  const base::Time from_time = LocalTimeAtBeginningOfThisMonth();
  const base::Time to_time = LocalTimeAtEndOfThisMonth();

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransactionWithinDateRange(const TransactionInfo& transaction,
                                            base::Time from_time,
                                            base::Time to_time) {
  if (!DidReconcileTransaction(transaction)) {
    return false;
  }

  if (transaction.reconciled_at < from_time ||
      transaction.reconciled_at > to_time) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
