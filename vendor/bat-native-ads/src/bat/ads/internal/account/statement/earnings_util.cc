/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/earnings_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/transactions/reconciled_transactions_util.h"
#include "bat/ads/transaction_info.h"

namespace ads {

namespace {

bool WasCreatedWithinDateRange(const TransactionInfo& transaction,
                               const base::Time& from_time,
                               const base::Time& to_time) {
  const base::Time& created_at =
      base::Time::FromDoubleT(transaction.created_at);

  if (created_at >= from_time && created_at <= to_time) {
    return true;
  }

  return false;
}

}  // namespace

double GetEarningsForDateRange(const TransactionList& transactions,
                               const base::Time& from_time,
                               const base::Time& to_time) {
  double earnings = 0.0;

  for (const auto& transaction : transactions) {
    if (WasCreatedWithinDateRange(transaction, from_time, to_time)) {
      earnings += transaction.value;
    }
  }

  return earnings;
}

double GetUnreconciledEarningsForDateRange(const TransactionList& transactions,
                                           const base::Time& from_time,
                                           const base::Time& to_time) {
  double earnings = 0.0;

  for (const auto& transaction : transactions) {
    if (WasCreatedWithinDateRange(transaction, from_time, to_time) &&
        !DidReconcileTransaction(transaction)) {
      earnings += transaction.value;
    }
  }

  return earnings;
}

double GetReconciledEarningsForDateRange(const TransactionList& transactions,
                                         const base::Time& from_time,
                                         const base::Time& to_time) {
  double earnings = 0.0;

  for (const auto& transaction : transactions) {
    if (WasCreatedWithinDateRange(transaction, from_time, to_time) &&
        DidReconcileTransaction(transaction)) {
      earnings += transaction.value;
    }
  }

  return earnings;
}

}  // namespace ads
