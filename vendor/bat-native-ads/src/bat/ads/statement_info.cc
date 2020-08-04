/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/statement_info.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace ads {

StatementInfo::StatementInfo() = default;

StatementInfo::StatementInfo(
    const StatementInfo& info) = default;

StatementInfo::~StatementInfo() = default;

std::string StatementInfo::ToJson() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Estimated pending rewards
  dictionary.SetKey("estimated_pending_rewards",
      base::Value(estimated_pending_rewards));

  // Next payment date in seconds
  dictionary.SetKey("next_payment_date_in_seconds",
      base::Value(std::to_string(next_payment_date_in_seconds)));

  // Ad notifications received this month
  dictionary.SetKey("ad_notifications_received_this_month",
      base::Value(std::to_string(ad_notifications_received_this_month)));

  // Transactions
  base::Value transactions_list = GetTransactionsAsList();
  dictionary.SetKey("transactions", base::Value(std::move(transactions_list)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool StatementInfo::FromJson(
    const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  estimated_pending_rewards =
      GetEstimatedPendingRewardsFromDictionary(dictionary);

  next_payment_date_in_seconds =
      GetNextPaymentDateInSecondsFromDictionary(dictionary);

  ad_notifications_received_this_month =
      GetAdNotificationsReceivedThisMonthFromDictionary(dictionary);

  transactions = GetTransactionsFromDictionary(dictionary);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

double StatementInfo::GetEstimatedPendingRewardsFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("estimated_pending_rewards").value_or(0.0);
}

uint64_t StatementInfo::GetNextPaymentDateInSecondsFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  const std::string* value =
      dictionary->FindStringKey("next_payment_date_in_seconds");
  if (!value) {
    return 0;
  }

  uint64_t value_as_uint64 = 0;
  if (!base::StringToUint64(*value, &value_as_uint64)) {
    return 0;
  }

  return value_as_uint64;
}

uint64_t StatementInfo::GetAdNotificationsReceivedThisMonthFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  const std::string* value =
      dictionary->FindStringKey("ad_notifications_received_this_month");
  if (!value) {
    return 0;
  }

  uint64_t value_as_uint64 = 0;
  if (!base::StringToUint64(*value, &value_as_uint64)) {
    return 0;
  }

  return value_as_uint64;
}

base::Value StatementInfo::GetTransactionsAsList() const {
  base::Value list(base::Value::Type::LIST);

  for (const auto& transaction : transactions) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    transaction.ToDictionary(&dictionary);

    list.Append(std::move(dictionary));
  }

  return list;
}

TransactionList StatementInfo::GetTransactionsFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  base::Value* transactions_list =
      dictionary->FindListKey("transactions");
  if (!transactions_list) {
    return {};
  }

  TransactionList transactions;

  for (auto& value : transactions_list->GetList()) {
    base::DictionaryValue* transaction_dictionary = nullptr;
    if (!value.GetAsDictionary(&transaction_dictionary)) {
      continue;
    }

    TransactionInfo transaction;
    transaction.FromDictionary(transaction_dictionary);

    transactions.push_back(transaction);
  }

  return transactions;
}

}  // namespace ads
