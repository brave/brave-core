/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"

#include "base/containers/extend.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payment_tokens_json_reader.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_payments_json_reader.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/payment_info.h"

namespace brave_ads::rewards {

std::optional<TransactionList> BuildTransactionsFromJson(
    const std::string& json) {
  const std::optional<PaymentList> payments = json::reader::ReadPayments(json);
  if (!payments) {
    return std::nullopt;
  }

  const std::optional<TransactionList> transaction_history =
      json::reader::ReadTransactionHistory(json);
  if (!transaction_history) {
    return std::nullopt;
  }

  const std::optional<PaymentTokenList> payment_tokens =
      json::reader::ReadPaymentTokens(json);
  if (!payment_tokens) {
    return std::nullopt;
  }

  // Get a list of all unreconciled transactions
  TransactionList transactions =
      GetAllUnreconciledTransactions(*transaction_history, *payment_tokens);

  // Append a list of reconciled transactions for this month
  const std::optional<TransactionList> reconciled_transactions =
      BuildTransactionsForReconciledTransactionsThisMonth(*payments);
  if (reconciled_transactions) {
    base::Extend(transactions, *reconciled_transactions);
  }

  // Append a single transaction with an accumulated value for reconciled
  // transactions last month for calculating the next payment date
  const std::optional<TransactionInfo> reconciled_transaction =
      BuildTransactionForReconciledTransactionsLastMonth(*payments);
  if (reconciled_transaction) {
    transactions.push_back(*reconciled_transaction);
  }

  return transactions;
}

}  // namespace brave_ads::rewards
