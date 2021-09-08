/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions.h"

#include <string>

#include "base/notreached.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

namespace ads {
namespace transactions {

TransactionList GetCleared(const int64_t from_timestamp,
                           const int64_t to_timestamp) {
  TransactionList transactions = ConfirmationsState::Get()->get_transactions();

  const auto iter = std::remove_if(
      transactions.begin(), transactions.end(),
      [from_timestamp, to_timestamp](const TransactionInfo& transaction) {
        return transaction.timestamp < from_timestamp ||
               transaction.timestamp > to_timestamp;
      });

  transactions.erase(iter, transactions.end());

  return transactions;
}

TransactionList GetUncleared() {
  const size_t count =
      ConfirmationsState::Get()->get_unblinded_payment_tokens()->Count();

  if (count == 0) {
    // There are no uncleared unblinded payment tokens to redeem
    return {};
  }

  // Uncleared transactions are always at the end of the transaction history
  const TransactionList transactions =
      ConfirmationsState::Get()->get_transactions();

  if (transactions.size() < count) {
    // There are fewer transactions than unblinded payment tokens which is
    // likely due to manually editing transactions in confirmations.json
    NOTREACHED();
    return transactions;
  }

  const TransactionList tail_transactions(transactions.end() - count,
                                          transactions.end());

  return tail_transactions;
}

uint64_t GetCountForMonth(const base::Time& time) {
  const TransactionList transactions =
      ConfirmationsState::Get()->get_transactions();

  uint64_t count = 0;

  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  for (const auto& transaction : transactions) {
    if (transaction.timestamp == 0) {
      // Workaround for Windows crash when passing 0 to UTCExplode
      continue;
    }

    const base::Time transaction_time =
        base::Time::FromDoubleT(transaction.timestamp);

    base::Time::Exploded transaction_time_exploded;
    transaction_time.LocalExplode(&transaction_time_exploded);

    if (transaction_time_exploded.year == exploded.year &&
        transaction_time_exploded.month == exploded.month &&
        transaction.estimated_redemption_value > 0.0 &&
        ConfirmationType(transaction.confirmation_type) ==
            ConfirmationType::kViewed) {
      count++;
    }
  }

  return count;
}

void Add(const double estimated_redemption_value,
         const ConfirmationInfo& confirmation) {
  TransactionInfo transaction;

  transaction.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());
  transaction.estimated_redemption_value = estimated_redemption_value;
  transaction.confirmation_type = std::string(confirmation.type);

  ConfirmationsState::Get()->add_transaction(transaction);
  ConfirmationsState::Get()->Save();
}

}  // namespace transactions
}  // namespace ads
