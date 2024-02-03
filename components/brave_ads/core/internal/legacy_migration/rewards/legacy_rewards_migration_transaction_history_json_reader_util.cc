/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration_transaction_history_json_reader_util.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads::rewards::json::reader {

namespace {

constexpr char kTransactionHistoryKey[] = "transaction_history";
constexpr char kTransactionListKey[] = "transactions";
constexpr char kCreatedAtKey[] = "timestamp_in_seconds";
constexpr char kRedemptionValueKey[] = "estimated_redemption_value";
constexpr char kConfirmationTypeKey[] = "confirmation_type";

std::optional<TransactionInfo> ParseTransaction(const base::Value::Dict& dict) {
  TransactionInfo transaction;

  // Id
  transaction.id = base::Uuid::GenerateRandomV4().AsLowercaseString();

  // Created at
  const std::string* const created_at = dict.FindString(kCreatedAtKey);
  if (!created_at) {
    return std::nullopt;
  }

  double created_at_as_double;
  if (!base::StringToDouble(*created_at, &created_at_as_double)) {
    return std::nullopt;
  }
  transaction.created_at =
      base::Time::FromSecondsSinceUnixEpoch(created_at_as_double);

  // Value
  const std::optional<double> redemption_value =
      dict.FindDouble(kRedemptionValueKey);
  if (!redemption_value) {
    return std::nullopt;
  }
  transaction.value = *redemption_value;

  // Confirmation type
  const std::string* const confirmation_type =
      dict.FindString(kConfirmationTypeKey);
  if (!confirmation_type) {
    return std::nullopt;
  }
  transaction.confirmation_type = ToConfirmationType(*confirmation_type);

  return transaction;
}

std::optional<TransactionList> GetTransactionsFromList(
    const base::Value::List& list) {
  TransactionList transactions;

  for (const auto& item : list) {
    if (!item.is_dict()) {
      return std::nullopt;
    }

    const std::optional<TransactionInfo> transaction =
        ParseTransaction(item.GetDict());
    if (!transaction) {
      return std::nullopt;
    }

    transactions.push_back(*transaction);
  }

  return transactions;
}

}  // namespace

std::optional<TransactionList> ParseTransactionHistory(
    const base::Value::Dict& dict) {
  const auto* const transaction_history = dict.FindDict(kTransactionHistoryKey);
  if (!transaction_history) {
    return TransactionList{};
  }

  const auto* const list = transaction_history->FindList(kTransactionListKey);
  if (!list) {
    return std::nullopt;
  }

  return GetTransactionsFromList(*list);
}

}  // namespace brave_ads::rewards::json::reader
