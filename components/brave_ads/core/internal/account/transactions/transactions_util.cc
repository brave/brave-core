/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_util.h"

#include <iterator>

#include "base/ranges/algorithm.h"
#include "base/time/time.h"

namespace brave_ads {

TransactionList GetTransactionsForDateRange(const TransactionList& transactions,
                                            const base::Time from_time,
                                            const base::Time to_time) {
  TransactionList filtered_transactions;
  filtered_transactions.reserve(transactions.size());

  base::ranges::copy_if(
      transactions, std::back_inserter(filtered_transactions),
      [from_time, to_time](const TransactionInfo& transaction) {
        return transaction.created_at >= from_time &&
               transaction.created_at <= to_time;
      });

  return filtered_transactions;
}

}  // namespace brave_ads
