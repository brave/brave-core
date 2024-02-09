/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"

#include <cstddef>

#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads::rewards {

namespace {

TransactionList GetUnreconciledTransactionsForDateRange(
    const TransactionList& transactions,
    const PaymentTokenList& payment_tokens,
    const base::Time from_time,
    const base::Time to_time) {
  const size_t payment_token_count = payment_tokens.size();

  if (transactions.size() < payment_token_count) {
    BLOG(0, "Invalid transaction history");
    return {};
  }

  const TransactionList unreconciled_transactions(
      transactions.cend() - static_cast<int>(payment_token_count),
      transactions.cend());

  return GetTransactionsForDateRange(unreconciled_transactions, from_time,
                                     to_time);
}

TransactionInfo BuildTransaction(const base::Time time, const double value) {
  TransactionInfo transaction;

  transaction.id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  transaction.created_at = time;
  transaction.creative_instance_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  transaction.value = value;
  transaction.ad_type = AdType::kNotificationAd;
  transaction.confirmation_type = ConfirmationType::kViewed;
  transaction.reconciled_at = time;

  return transaction;
}

}  // namespace

TransactionList GetAllUnreconciledTransactions(
    const TransactionList& transactions,
    const PaymentTokenList& payment_tokens) {
  const base::Time from_time = GetTimeInDistantPast();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  TransactionList unreconciled_transactions =
      GetUnreconciledTransactionsForDateRange(transactions, payment_tokens,
                                              from_time, to_time);

  for (auto& transaction : unreconciled_transactions) {
    // `created_at`, `value` and `confirmation_type` are set from legacy state
    transaction.id = base::Uuid::GenerateRandomV4().AsLowercaseString();
    transaction.creative_instance_id = kMigrationUnreconciledTransactionId;
    transaction.ad_type = AdType::kNotificationAd;
  }

  return unreconciled_transactions;
}

std::optional<TransactionList>
BuildTransactionsForReconciledTransactionsThisMonth(
    const PaymentList& payments) {
  const std::optional<PaymentInfo> payment = GetPaymentForThisMonth(payments);
  if (!payment) {
    return std::nullopt;
  }

  if (payment->balance == 0.0) {
    return std::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfThisMonth();

  TransactionList reconciled_transactions;

  // Add a transaction with the payment balance for this month as the value
  reconciled_transactions.push_back(BuildTransaction(time, payment->balance));

  // Add `transaction_count` - 1 transactions with a value of 0.0 to migrate ads
  // received this month
  for (int i = 0; i < payment->transaction_count - 1; ++i) {
    reconciled_transactions.push_back(BuildTransaction(time,
                                                       /*value=*/0.0));
  }

  return reconciled_transactions;
}

std::optional<TransactionInfo>
BuildTransactionForReconciledTransactionsLastMonth(
    const PaymentList& payments) {
  const std::optional<PaymentInfo> payment = GetPaymentForLastMonth(payments);
  if (!payment || payment->balance == 0.0) {
    return std::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfLastMonth();
  return BuildTransaction(time, payment->balance);
}

}  // namespace brave_ads::rewards
