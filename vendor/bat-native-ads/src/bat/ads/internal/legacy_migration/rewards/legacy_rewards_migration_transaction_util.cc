/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"

namespace ads::rewards {

namespace {

TransactionList GetUnreconciledTransactionsForDateRange(
    const TransactionList& transactions,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
    const base::Time from_time,
    const base::Time to_time) {
  const size_t unblinded_payment_token_count = unblinded_payment_tokens.size();

  if (transactions.size() < unblinded_payment_token_count) {
    BLOG(0, "Invalid transaction history");
    return {};
  }

  const TransactionList unreconciled_transactions(
      transactions.cend() - unblinded_payment_token_count, transactions.cend());

  return GetTransactionsForDateRange(unreconciled_transactions, from_time,
                                     to_time);
}

TransactionInfo BuildTransaction(const base::Time time, const double value) {
  TransactionInfo transaction;

  transaction.id = base::GUID::GenerateRandomV4().AsLowercaseString();
  transaction.created_at = time;
  transaction.creative_instance_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  transaction.value = value;
  transaction.ad_type = AdType::kNotificationAd;
  transaction.confirmation_type = ConfirmationType::kViewed;
  transaction.reconciled_at = time;

  return transaction;
}

}  // namespace

TransactionList GetAllUnreconciledTransactions(
    const TransactionList& transactions,
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const base::Time from_time = GetTimeInDistantPast();
  const base::Time to_time = GetLocalTimeAtEndOfThisMonth();

  TransactionList unreconciled_transactions =
      GetUnreconciledTransactionsForDateRange(
          transactions, unblinded_payment_tokens, from_time, to_time);

  for (auto& transaction : unreconciled_transactions) {
    // |created_at|, |value| and |confirmation_type| are set from legacy state
    transaction.id = base::GUID::GenerateRandomV4().AsLowercaseString();
    transaction.creative_instance_id = kMigrationUnreconciledTransactionId;
    transaction.ad_type = AdType::kNotificationAd;
  }

  return unreconciled_transactions;
}

absl::optional<TransactionList>
BuildTransactionsForReconciledTransactionsThisMonth(
    const PaymentList& payments) {
  const absl::optional<PaymentInfo> payment = GetPaymentForThisMonth(payments);
  if (!payment) {
    return absl::nullopt;
  }

  if (payment->balance == 0.0) {
    return absl::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfThisMonth();

  TransactionList reconciled_transactions;

  // Add a transaction with the payment balance for this month as the value
  reconciled_transactions.push_back(BuildTransaction(time, payment->balance));

  // Add |transaction_count - 1| transactions with a value of 0.0 to migrate ads
  // received this month
  for (int i = 0; i < payment->transaction_count - 1; i++) {
    reconciled_transactions.push_back(BuildTransaction(time,
                                                       /*value*/ 0.0));
  }

  return reconciled_transactions;
}

absl::optional<TransactionInfo>
BuildTransactionForReconciledTransactionsLastMonth(
    const PaymentList& payments) {
  const absl::optional<PaymentInfo> payment = GetPaymentForLastMonth(payments);
  if (!payment || payment->balance == 0.0) {
    return absl::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfLastMonth();
  return BuildTransaction(time, payment->balance);
}

}  // namespace ads::rewards
