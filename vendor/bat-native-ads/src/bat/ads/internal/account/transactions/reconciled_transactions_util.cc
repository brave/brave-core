/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/reconciled_transactions_util.h"

#include <algorithm>
#include <iterator>

#include "base/time/time.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/transaction_info.h"

namespace ads {

namespace {

bool HasReconciledTransactionsForDateRange(const TransactionList& transactions,
                                           const base::Time& from_time,
                                           const base::Time& to_time) {
  const int count =
      std::count_if(transactions.cbegin(), transactions.cend(),
                    [&from_time, &to_time](const TransactionInfo& transaction) {
                      return DidReconcileTransactionWithinDateRange(
                          transaction, from_time, to_time);
                    });

  if (count == 0) {
    return false;
  }

  return true;
}

}  // namespace

bool DidReconcileTransactionsLastMonth(const TransactionList& transactions) {
  const base::Time& now = base::Time::Now();

  const base::Time& from_time = AdjustTimeToBeginningOfPreviousMonth(now);
  const base::Time& to_time = AdjustTimeToEndOfPreviousMonth(now);

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransactionsThisMonth(const TransactionList& transactions) {
  const base::Time& now = base::Time::Now();

  const base::Time& from_time = AdjustTimeToBeginningOfMonth(now);
  const base::Time& to_time = now;

  return HasReconciledTransactionsForDateRange(transactions, from_time,
                                               to_time);
}

bool DidReconcileTransaction(const TransactionInfo& transaction) {
  const base::Time& reconciled_at =
      base::Time::FromDoubleT(transaction.reconciled_at);

  return !reconciled_at.is_null();
}

bool DidReconcileTransactionWithinDateRange(const TransactionInfo& transaction,
                                            const base::Time& from_time,
                                            const base::Time& to_time) {
  const base::Time& created_at =
      base::Time::FromDoubleT(transaction.created_at);

  if (created_at < from_time || created_at > to_time) {
    return false;
  }

  return DidReconcileTransaction(transaction);
}

}  // namespace ads
