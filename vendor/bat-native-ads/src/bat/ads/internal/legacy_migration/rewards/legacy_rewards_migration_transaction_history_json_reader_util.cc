/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader_util.h"

#include <string>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace rewards {
namespace JSONReader {

namespace {

const char kTransactionHistoryKey[] = "transaction_history";
const char kTransactionListKey[] = "transactions";
const char kCreatedAtKey[] = "timestamp_in_seconds";
const char kValueKey[] = "estimated_redemption_value";
const char kConfirmationTypeKey[] = "confirmation_type";

absl::optional<TransactionInfo> ParseTransaction(const base::Value& value) {
  TransactionInfo transaction;

  // Id
  transaction.id = base::GenerateGUID();

  // Created at
  const std::string* created_at = value.FindStringKey(kCreatedAtKey);
  if (!created_at) {
    return absl::nullopt;
  }

  if (!base::StringToDouble(*created_at, &transaction.created_at)) {
    return absl::nullopt;
  }

  // Value
  const absl::optional<double> value_optional = value.FindDoubleKey(kValueKey);
  if (!value_optional) {
    return absl::nullopt;
  }
  transaction.value = value_optional.value();

  // Confirmation type
  const std::string* confirmation_type =
      value.FindStringKey(kConfirmationTypeKey);
  if (!confirmation_type) {
    return absl::nullopt;
  }
  transaction.confirmation_type = ConfirmationType(*confirmation_type);

  return transaction;
}

absl::optional<TransactionList> GetTransactionsFromList(
    const base::Value& value) {
  if (!value.is_list()) {
    return absl::nullopt;
  }

  TransactionList transactions;

  for (const auto& transaction_value : value.GetList()) {
    if (!transaction_value.is_dict()) {
      return absl::nullopt;
    }

    const absl::optional<TransactionInfo>& transaction_optional =
        ParseTransaction(transaction_value);
    if (!transaction_optional) {
      return absl::nullopt;
    }
    TransactionInfo transaction = transaction_optional.value();

    transactions.push_back(transaction);
  }

  return transactions;
}

}  // namespace

absl::optional<TransactionList> ParseTransactionHistory(
    const base::Value& value) {
  const base::Value* const transaction_history_value =
      value.FindDictKey(kTransactionHistoryKey);
  if (!transaction_history_value) {
    return absl::nullopt;
  }

  const base::Value* const transaction_value =
      transaction_history_value->FindListKey(kTransactionListKey);
  if (!transaction_value) {
    return absl::nullopt;
  }

  const absl::optional<TransactionList>& transactions_optional =
      GetTransactionsFromList(*transaction_value);
  if (!transactions_optional) {
    return absl::nullopt;
  }

  return transactions_optional.value();
}

}  // namespace JSONReader
}  // namespace rewards
}  // namespace ads
