/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/transactions_info.h"

namespace ledger {

TransactionsInfo::TransactionsInfo() :
    transactions({}) {}

TransactionsInfo::TransactionsInfo(const TransactionsInfo& info) :
    transactions(info.transactions) {}

TransactionsInfo::~TransactionsInfo() = default;

const std::string TransactionsInfo::ToJson() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  base::Value list(base::Value::Type::LIST);
  for (const auto& transaction : transactions) {
    base::Value transaction_dictionary(base::Value::Type::DICTIONARY);

    transaction_dictionary.SetKey("timestamp_in_seconds",
        base::Value(std::to_string(transaction.timestamp_in_seconds)));

    transaction_dictionary.SetKey("estimated_redemption_value",
        base::Value(transaction.estimated_redemption_value));

    transaction_dictionary.SetKey("confirmation_type",
        base::Value(transaction.confirmation_type));

    list.GetList().push_back(std::move(transaction_dictionary));
  }

  dictionary.SetKey("transactions", base::Value(std::move(list)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool TransactionsInfo::FromJson(
    const std::string& json) {
  base::Optional<base::Value> dictionary =
    base::JSONReader::Read(json);
  if (!dictionary || !dictionary->is_dict()) {
    return false;
  }

  auto* transactions_value = dictionary->FindKey("transactions");
  if (!transactions_value) {
    return false;
  }

  std::vector<TransactionInfo> new_transactions;

  base::ListValue transactions_list_value(transactions_value->GetList());
  for (auto& transaction_value : transactions_list_value) {
    base::DictionaryValue* transaction_dictionary;
    if (!transaction_value.GetAsDictionary(&transaction_dictionary)) {
      return false;
    }

    TransactionInfo info;

    // Timestamp
    auto* timestamp_in_seconds_value =
        transaction_dictionary->FindKey("timestamp_in_seconds");
    if (!timestamp_in_seconds_value) {
      return false;
    }
    info.timestamp_in_seconds =
        std::stoull(timestamp_in_seconds_value->GetString());

    // Estimated redemption value
    auto* estimated_redemption_value_value =
        transaction_dictionary->FindKey("estimated_redemption_value");
    if (!estimated_redemption_value_value) {
      return false;
    }
    info.estimated_redemption_value =
        estimated_redemption_value_value->GetDouble();

    // Confirmation type
    auto* confirmation_type_value =
        transaction_dictionary->FindKey("confirmation_type");
    if (!confirmation_type_value) {
      return false;
    }
    info.confirmation_type = confirmation_type_value->GetString();

    new_transactions.push_back(info);
  }

  transactions = new_transactions;

  return true;
}

}  // namespace ledger
