/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_unblinded_payment_tokens_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/payment_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads::rewards {

absl::optional<TransactionList> BuildTransactionsFromJson(
    const std::string& json) {
  const absl::optional<PaymentList> payments = json::reader::ReadPayments(json);
  if (!payments) {
    return absl::nullopt;
  }

  const absl::optional<TransactionList> transaction_history =
      json::reader::ReadTransactionHistory(json);
  if (!transaction_history) {
    return absl::nullopt;
  }

  const absl::optional<privacy::UnblindedPaymentTokenList>
      unblinded_payment_tokens = json::reader::ReadUnblindedPaymentTokens(json);
  if (!unblinded_payment_tokens) {
    return absl::nullopt;
  }

  // Get a list of all unreconciled transactions
  TransactionList transactions = GetAllUnreconciledTransactions(
      *transaction_history, *unblinded_payment_tokens);

  // Append a list of reconciled transactions for this month
  const absl::optional<TransactionList> reconciled_transactions =
      BuildTransactionsForReconciledTransactionsThisMonth(*payments);
  if (reconciled_transactions) {
    transactions.insert(transactions.cend(), reconciled_transactions->cbegin(),
                        reconciled_transactions->cend());
  }

  // Append a single transaction with an accumulated value for reconciled
  // transactions last month for calculating the next payment date
  const absl::optional<TransactionInfo> reconciled_transaction =
      BuildTransactionForReconciledTransactionsLastMonth(*payments);
  if (reconciled_transaction) {
    transactions.push_back(*reconciled_transaction);
  }

  return transactions;
}

}  // namespace ads::rewards
