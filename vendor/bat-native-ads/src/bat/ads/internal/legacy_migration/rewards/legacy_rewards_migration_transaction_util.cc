/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_earnings_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

TransactionList GetTransactionsForThisMonth(
    const TransactionList& transactions) {
  const base::Time& from_time = GetTimeAtBeginningOfThisMonth();
  const base::Time& to_time = base::Time::Now();

  return GetTransactionsForDateRange(transactions, from_time, to_time);
}

absl::optional<TransactionInfo>
BuildTransactionForUnreconciledTransactionsForPreviousMonths(
    const TransactionList& transaction_history,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const base::Time& from_time = base::Time();
  const base::Time& to_time = GetTimeAtEndOfLastMonth();

  const double value = GetUnreconciledEarningsForDateRange(
      transaction_history, unblinded_payment_tokens, from_time, to_time);
  if (value == 0.0) {
    return absl::nullopt;
  }

  TransactionInfo transaction;
  transaction.id = rewards::kMigrationUnreconciledTransactionId;
  transaction.created_at = base::Time::Now().ToDoubleT();
  transaction.value = value;
  transaction.confirmation_type = ConfirmationType::kViewed;

  return transaction;
}

absl::optional<TransactionInfo>
BuildTransactionForReconciledTransactionsLastMonth(
    const PaymentList& payments) {
  const base::Time& time = GetTimeAtBeginningOfLastMonth();
  const double timestamp = time.ToDoubleT();

  const double value = GetPaymentBalanceForMonth(payments, time);
  if (value == 0.0) {
    return absl::nullopt;
  }

  TransactionInfo transaction;
  transaction.id = base::GenerateGUID();
  transaction.created_at = timestamp;
  transaction.value = value;
  transaction.confirmation_type = ConfirmationType::kViewed;
  transaction.reconciled_at = timestamp;

  return transaction;
}

}  // namespace rewards
}  // namespace ads
