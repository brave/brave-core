/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

namespace ads {

Transactions::Transactions(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Transactions::~Transactions() = default;

TransactionList Transactions::Get(
    const int64_t from_timestamp,
    const int64_t to_timestamp) const {
  TransactionList transactions = ads_->get_confirmations()->get_transactions();

  const auto iter = std::remove_if(transactions.begin(), transactions.end(),
      [from_timestamp, to_timestamp](TransactionInfo& transaction) {
    return transaction.timestamp < from_timestamp ||
        transaction.timestamp > to_timestamp;
  });

  transactions.erase(iter, transactions.end());

  return transactions;
}

TransactionList Transactions::GetUncleared() {
  const size_t count = ads_->get_confirmations()->
      get_unblinded_payment_tokens()->Count();

  if (count == 0) {
    // There are no outstanding unblinded payment tokens to redeem
    return {};
  }

  // Unredeemed transactions are always at the end of the transaction history
  const TransactionList transactions =
      ads_->get_confirmations()->get_transactions();

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

}  // namespace ads
