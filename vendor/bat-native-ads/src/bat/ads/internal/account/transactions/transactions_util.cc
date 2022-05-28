/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions_util.h"

#include <algorithm>
#include <iterator>

#include "base/time/time.h"
#include "bat/ads/transaction_info.h"

namespace ads {

TransactionList GetTransactionsForDateRange(const TransactionList& transactions,
                                            const base::Time from_time,
                                            const base::Time to_time) {
  TransactionList filtered_transactions;

  std::copy_if(transactions.cbegin(), transactions.cend(),
               std::back_inserter(filtered_transactions),
               [from_time, to_time](const TransactionInfo& transaction) {
                 return transaction.created_at >= from_time &&
                        transaction.created_at <= to_time;
               });

  return filtered_transactions;
}

double GetEarningsForTransactions(const TransactionList& transactions) {
  double earnings = 0.0;

  for (const auto& transaction : transactions) {
    earnings += transaction.value;
  }

  return earnings;
}

}  // namespace ads
