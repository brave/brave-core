/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader_util.h"

#include <string>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/confirmation_type.h"

namespace ads::rewards::json::reader {

namespace {

constexpr char kTransactionHistoryKey[] = "transaction_history";
constexpr char kTransactionListKey[] = "transactions";
constexpr char kCreatedAtKey[] = "timestamp_in_seconds";
constexpr char kRedemptionValueKey[] = "estimated_redemption_value";
constexpr char kConfirmationTypeKey[] = "confirmation_type";

absl::optional<TransactionInfo> ParseTransaction(
    const base::Value::Dict& value) {
  TransactionInfo transaction;

  // Id
  transaction.id = base::GUID::GenerateRandomV4().AsLowercaseString();

  // Created at
  const std::string* const created_at = value.FindString(kCreatedAtKey);
  if (!created_at) {
    return absl::nullopt;
  }

  double created_at_as_double = 0.0;
  if (!base::StringToDouble(*created_at, &created_at_as_double)) {
    return absl::nullopt;
  }
  transaction.created_at = base::Time::FromDoubleT(created_at_as_double);

  // Value
  const absl::optional<double> redemption_value =
      value.FindDouble(kRedemptionValueKey);
  if (!redemption_value) {
    return absl::nullopt;
  }
  transaction.value = *redemption_value;

  // Confirmation type
  const std::string* const confirmation_type =
      value.FindString(kConfirmationTypeKey);
  if (!confirmation_type) {
    return absl::nullopt;
  }
  transaction.confirmation_type = ConfirmationType(*confirmation_type);

  return transaction;
}

absl::optional<TransactionList> GetTransactionsFromList(
    const base::Value::List& list) {
  TransactionList transactions;

  for (const auto& item : list) {
    if (!item.is_dict()) {
      return absl::nullopt;
    }

    const absl::optional<TransactionInfo> transaction =
        ParseTransaction(item.GetDict());
    if (!transaction) {
      return absl::nullopt;
    }

    transactions.push_back(*transaction);
  }

  return transactions;
}

}  // namespace

absl::optional<TransactionList> ParseTransactionHistory(
    const base::Value::Dict& value) {
  const base::Value::Dict* const transaction_history =
      value.FindDict(kTransactionHistoryKey);
  if (!transaction_history) {
    return TransactionList{};
  }

  const base::Value::List* const list =
      transaction_history->FindList(kTransactionListKey);
  if (!list) {
    return absl::nullopt;
  }

  return GetTransactionsFromList(*list);
}

}  // namespace ads::rewards::json::reader
