/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_util.h"

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_payments_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_util.h"
#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_unblinded_payment_tokens_json_reader.h"
#include "bat/ads/internal/legacy_migration/rewards/payment_info_aliases.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {

absl::optional<TransactionList> BuildTransactionsFromJson(
    const std::string& json) {
  const absl::optional<PaymentList>& payments_optional =
      JSONReader::ReadPayments(json);
  if (!payments_optional) {
    return absl::nullopt;
  }
  const PaymentList& payments = payments_optional.value();

  const absl::optional<TransactionList>& transaction_history_optional =
      JSONReader::ReadTransactionHistory(json);
  if (!transaction_history_optional) {
    return absl::nullopt;
  }
  const TransactionList& transaction_history =
      transaction_history_optional.value();

  const absl::optional<privacy::UnblindedPaymentTokenList>&
      unblinded_payment_tokens_optional =
          JSONReader::ReadUnblindedPaymentTokens(json);
  if (!unblinded_payment_tokens_optional) {
    return absl::nullopt;
  }
  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      unblinded_payment_tokens_optional.value();

  // Get a list of transactions for this month
  TransactionList transactions =
      GetTransactionsForThisMonth(transaction_history);

  // Append a single transaction with an accumulated value for unreconciled
  // transactions for previous months
  const absl::optional<TransactionInfo>& unreconciled_transaction_optional =
      BuildTransactionForUnreconciledTransactionsForPreviousMonths(
          transaction_history, unblinded_payment_tokens);
  if (unreconciled_transaction_optional) {
    const TransactionInfo& unreconciled_transaction =
        unreconciled_transaction_optional.value();
    transactions.push_back(unreconciled_transaction);
  }

  // Append a single transaction with an accumulated value for reconciled
  // transactions last month for calculating the next payment date
  const absl::optional<TransactionInfo>& reconciled_transaction_optional =
      BuildTransactionForReconciledTransactionsLastMonth(payments);
  if (reconciled_transaction_optional) {
    const TransactionInfo& reconciled_transaction =
        reconciled_transaction_optional.value();
    transactions.push_back(reconciled_transaction);
  }

  return transactions;
}

}  // namespace rewards
}  // namespace ads
