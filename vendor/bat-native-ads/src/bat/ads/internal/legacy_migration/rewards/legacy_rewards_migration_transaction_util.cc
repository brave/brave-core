/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/transactions/transactions_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/time_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_constants.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

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

  TransactionList unreconciled_transactions(
      transactions.end() - unblinded_payment_token_count, transactions.end());

  return GetTransactionsForDateRange(unreconciled_transactions, from_time,
                                     to_time);
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
    transaction.ad_type = AdType::kAdNotification;
  }

  return unreconciled_transactions;
}

absl::optional<TransactionList>
BuildTransactionsForReconciledTransactionsThisMonth(
    const PaymentList& payments) {
  const absl::optional<PaymentInfo>& payment_optional =
      GetPaymentForThisMonth(payments);
  if (!payment_optional) {
    return absl::nullopt;
  }

  const PaymentInfo& payment = payment_optional.value();

  if (payment.balance == 0.0) {
    return absl::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfThisMonth();
  const double timestamp = time.ToDoubleT();

  TransactionList reconciled_transactions;

  // Add a transaction with the payment balance for this month as the value
  TransactionInfo reconciled_transaction;
  reconciled_transaction.id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  reconciled_transaction.created_at = timestamp;
  reconciled_transaction.creative_instance_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  reconciled_transaction.value = payment.balance;
  reconciled_transaction.ad_type = AdType::kAdNotification;
  reconciled_transaction.confirmation_type = ConfirmationType::kViewed;
  reconciled_transaction.reconciled_at = timestamp;
  reconciled_transactions.push_back(reconciled_transaction);

  // Add |transaction_count - 1| transactions with a value of 0.0 to migrate ads
  // received this month
  for (int i = 0; i < payment.transaction_count - 1; i++) {
    TransactionInfo reconciled_transaction;
    reconciled_transaction.id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    reconciled_transaction.created_at = timestamp;
    reconciled_transaction.creative_instance_id =
        base::GUID::GenerateRandomV4().AsLowercaseString();
    reconciled_transaction.value = 0.0;
    reconciled_transaction.ad_type = AdType::kAdNotification;
    reconciled_transaction.confirmation_type = ConfirmationType::kViewed;
    reconciled_transaction.reconciled_at = timestamp;
    reconciled_transactions.push_back(reconciled_transaction);
  }

  return reconciled_transactions;
}

absl::optional<TransactionInfo>
BuildTransactionForReconciledTransactionsLastMonth(
    const PaymentList& payments) {
  const absl::optional<PaymentInfo>& payment_optional =
      GetPaymentForLastMonth(payments);
  if (!payment_optional) {
    return absl::nullopt;
  }

  const PaymentInfo& payment = payment_optional.value();

  if (payment.balance == 0.0) {
    return absl::nullopt;
  }

  const base::Time time = GetLocalTimeAtBeginningOfLastMonth();
  const double timestamp = time.ToDoubleT();

  TransactionInfo reconciled_transaction;
  reconciled_transaction.id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  reconciled_transaction.created_at = timestamp;
  reconciled_transaction.creative_instance_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  reconciled_transaction.value = payment.balance;
  reconciled_transaction.ad_type = AdType::kAdNotification;
  reconciled_transaction.confirmation_type = ConfirmationType::kViewed;
  reconciled_transaction.reconciled_at = timestamp;

  return reconciled_transaction;
}

}  // namespace rewards
}  // namespace ads
