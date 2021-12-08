/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_earnings_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/account/statement/earnings_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace rewards {

double GetUnreconciledEarningsForDateRange(
    const TransactionList& transaction_history,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    const base::Time& from_time,
    const base::Time& to_time) {
  const size_t unblinded_payment_token_count = unblinded_payment_tokens.size();

  if (transaction_history.size() < unblinded_payment_token_count) {
    BLOG(0, "Invalid transaction history");
    return 0.0;
  }

  TransactionList unreconciled_transactions(
      transaction_history.end() - unblinded_payment_token_count,
      transaction_history.end());

  return GetEarningsForDateRange(unreconciled_transactions, from_time, to_time);
}

}  // namespace rewards
}  // namespace ads
