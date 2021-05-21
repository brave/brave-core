/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/statement_info.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/number_util.h"

namespace ads {

StatementInfo::StatementInfo() = default;

StatementInfo::StatementInfo(const StatementInfo& info) = default;

StatementInfo::~StatementInfo() = default;

bool StatementInfo::operator==(const StatementInfo& rhs) const {
  return DoubleEquals(estimated_pending_rewards,
                      rhs.estimated_pending_rewards) &&
         next_payment_date == rhs.next_payment_date &&
         ads_received_this_month == rhs.ads_received_this_month &&
         DoubleEquals(earnings_this_month, rhs.earnings_this_month) &&
         DoubleEquals(earnings_last_month, rhs.earnings_last_month) &&
         transactions == rhs.transactions &&
         uncleared_transactions == rhs.uncleared_transactions;
}

bool StatementInfo::operator!=(const StatementInfo& rhs) const {
  return !(*this == rhs);
}

std::string StatementInfo::ToJson() const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  // Estimated pending rewards
  dictionary.SetKey("estimated_pending_rewards",
                    base::Value(estimated_pending_rewards));

  // Next payment date
  dictionary.SetKey("next_payment_date",
                    base::Value(std::to_string(next_payment_date)));

  // Ads received this month
  dictionary.SetKey("ads_received_this_month",
                    base::Value(ads_received_this_month));

  // Earnings this month
  dictionary.SetKey("earnings_this_month", base::Value(earnings_this_month));

  // Earnings last month
  dictionary.SetKey("earnings_last_month", base::Value(earnings_last_month));

  // Transactions
  base::Value transactions_list = GetTransactionsAsList();
  dictionary.SetKey("transactions", base::Value(std::move(transactions_list)));

  // Uncleared transactions
  base::Value uncleared_transactions_list = GetUnclearedTransactionsAsList();
  dictionary.SetKey("uncleared_transactions",
                    base::Value(std::move(uncleared_transactions_list)));

  // Write to JSON
  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

bool StatementInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  estimated_pending_rewards =
      GetEstimatedPendingRewardsFromDictionary(dictionary);

  next_payment_date = GetNextPaymentDateFromDictionary(dictionary);

  ads_received_this_month = GetAdsReceivedThisMonthFromDictionary(dictionary);

  earnings_this_month = GetEarningsThisMonthFromDictionary(dictionary);

  earnings_last_month = GetEarningsLastMonthFromDictionary(dictionary);

  transactions = GetTransactionsFromDictionary(dictionary);

  uncleared_transactions = GetUnclearedTransactionsFromDictionary(dictionary);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

double StatementInfo::GetEstimatedPendingRewardsFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("estimated_pending_rewards").value_or(0.0);
}

uint64_t StatementInfo::GetNextPaymentDateFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  const std::string* value = dictionary->FindStringKey("next_payment_date");
  if (!value) {
    return 0;
  }

  uint64_t value_as_uint64 = 0;
  if (!base::StringToUint64(*value, &value_as_uint64)) {
    return 0;
  }

  return value_as_uint64;
}

uint64_t StatementInfo::GetAdsReceivedThisMonthFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  return dictionary->FindIntKey("ads_received_this_month").value_or(0);
}

double StatementInfo::GetEarningsThisMonthFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("earnings_this_month").value_or(0.0);
}

double StatementInfo::GetEarningsLastMonthFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  return dictionary->FindDoubleKey("earnings_last_month").value_or(0.0);
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

  base::Value* transactions_list = dictionary->FindListKey("transactions");
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

base::Value StatementInfo::GetUnclearedTransactionsAsList() const {
  base::Value list(base::Value::Type::LIST);

  for (const auto& transaction : uncleared_transactions) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    transaction.ToDictionary(&dictionary);

    list.Append(std::move(dictionary));
  }

  return list;
}

TransactionList StatementInfo::GetUnclearedTransactionsFromDictionary(
    base::DictionaryValue* dictionary) const {
  DCHECK(dictionary);

  base::Value* transactions_list =
      dictionary->FindListKey("uncleared_transactions");
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
