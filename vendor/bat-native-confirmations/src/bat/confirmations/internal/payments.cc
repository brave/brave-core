/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/confirmations/internal/payments.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/time_util.h"

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "third_party/re2/src/re2/re2.h"

namespace confirmations {

Payments::Payments() = default;

Payments::~Payments() = default;

bool Payments::SetFromJson(const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_list()) {
    return false;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    return false;
  }

  payments_ = GetPaymentsFromList(list);

  return true;
}

bool Payments::SetFromDictionary(base::DictionaryValue* dictionary) {
  DCHECK(dictionary);

  auto* value = dictionary->FindKey("payments");
  if (!value || !value->is_list()) {
    return false;
  }

  PaymentList payments;

  base::ListValue list_value(value->GetList());
  for (auto& value : list_value) {
    base::DictionaryValue* dictionary;
    if (!value.GetAsDictionary(&dictionary)) {
      return false;
    }

    PaymentInfo payment;

    // Balance
    const base::Optional<double> balance_value =
        dictionary->FindDoubleKey("balance");
    if (!balance_value) {
      return false;
    }

    payment.balance = balance_value.value_or(0.0);

    // Month
    auto* month_value = dictionary->FindKey("month");
    if (!month_value || !month_value->is_string()) {
      return false;
    }

    payment.month = month_value->GetString();

    // Transaction count
    auto* transaction_count_value = dictionary->FindKey("transaction_count");
    if (!transaction_count_value || !transaction_count_value->is_string()) {
      return false;
    }

    if (!base::StringToUint64(transaction_count_value->GetString(),
        &payment.transaction_count)) {
      return false;
    }

    payments.push_back(payment);
  }

  payments_ = payments;

  return true;
}

base::Value Payments::GetAsList() {
  base::Value list(base::Value::Type::LIST);

  for (const auto& payment : payments_) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey("balance", base::Value(payment.balance));
    dictionary.SetKey("month", base::Value(payment.month));
    dictionary.SetKey("transaction_count",
        base::Value(std::to_string(payment.transaction_count)));

    list.Append(std::move(dictionary));
  }

  return list;
}

double Payments::GetBalance() const {
  double balance = 0.0;

  for (const auto& payment : payments_) {
    balance += payment.balance;
  }

  return balance;
}

base::Time Payments::CalculateNextPaymentDate(
    const base::Time& time,
    const uint64_t next_token_redemption_date_in_seconds) const {
  base::Time::Exploded now_exploded;
  time.UTCExplode(&now_exploded);

  int month = now_exploded.month;

  if (now_exploded.day_of_month <= kNextPaymentDay) {
    auto previous_month = GetPreviousTransactionMonth(time);
    if (HasPendingBalanceForTransactionMonth(previous_month)) {
      // If last month has a pending balance, then the next payment date will
      // occur this month
    } else {
      // If last month does not have a pending balance, then the next payment
      // date will occur next month
      month++;
    }
  } else {
    auto this_month = GetTransactionMonth(time);
    if (HasPendingBalanceForTransactionMonth(this_month)) {
      // If this month has a pending balance, then the next payment date will
      // occur next month
      month++;
    } else {
      auto next_token_redemption_date =
          base::Time::FromDoubleT(next_token_redemption_date_in_seconds);

      base::Time::Exploded next_token_redemption_date_exploded;
      next_token_redemption_date.UTCExplode(
          &next_token_redemption_date_exploded);

      if (next_token_redemption_date_exploded.month == month) {
        // If this month does not have a pending balance and our next token
        // redemption date is this month, then the next payment date will occur
        // next month
        month++;
      } else {
        // If this month does not have a pending balance and our next token
        // redemption date is next month, then the next payment date will occur
        // the month after next
        month += 2;
      }
    }
  }

  int year = now_exploded.year;

  if (month > 12) {
    month -= 12;
    year++;
  }

  base::Time::Exploded next_payment_date_exploded = now_exploded;
  next_payment_date_exploded.year = year;
  next_payment_date_exploded.month = month;
  next_payment_date_exploded.day_of_month = kNextPaymentDay;
  next_payment_date_exploded.hour = 23;
  next_payment_date_exploded.minute = 59;
  next_payment_date_exploded.second = 59;
  next_payment_date_exploded.millisecond = 999;

  base::Time next_payment_date;
  bool success = base::Time::FromUTCExploded(next_payment_date_exploded,
      &next_payment_date);
  DCHECK(success);

  return next_payment_date;
}

uint64_t Payments::GetTransactionCountForMonth(const base::Time& time) const {
  auto month = GetTransactionMonth(time);
  auto payment = GetPaymentForTransactionMonth(month);
  return payment.transaction_count;
}

///////////////////////////////////////////////////////////////////////////////

PaymentList Payments::GetPaymentsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  PaymentList payments;

  for (auto& item : *list) {
    base::DictionaryValue* dictionary;
    if (!item.GetAsDictionary(&dictionary)) {
      continue;
    }

    PaymentInfo payment;

    if (!GetBalanceFromDictionary(
        dictionary, &payment.balance)) {
      continue;
    }

    if (!GetMonthFromDictionary(
        dictionary, &payment.month)) {
      continue;
    }

    if (!GetTransactionCountFromDictionary(
        dictionary, &payment.transaction_count)) {
      continue;
    }

    payments.push_back(payment);
  }

  return payments;
}

bool Payments::GetBalanceFromDictionary(
    base::DictionaryValue* dictionary,
    double* balance) const {
  DCHECK(dictionary);
  DCHECK(balance);

  auto* value = dictionary->FindKey("balance");
  if (!value || !value->is_string()) {
    return false;
  }

  auto balance_value = value->GetString();

  // Match a double, i.e. 1.23
  if (!re2::RE2::FullMatch(balance_value, "[+]?([0-9]*[.])?[0-9]+")) {
    return false;
  }

  *balance = std::stod(balance_value);

  return true;
}

bool Payments::GetMonthFromDictionary(
    base::DictionaryValue* dictionary,
    std::string* month) const {
  DCHECK(dictionary);
  DCHECK(month);

  auto* value = dictionary->FindKey("month");
  if (!value || !value->is_string()) {
    return false;
  }

  auto month_value = value->GetString();

  // Match YYYY-MM, i.e. 2019-06
  if (!re2::RE2::FullMatch(month_value, "[0-9]{4}-[0-9]{2}")) {
    return false;
  }

  *month = month_value;

  return true;
}

bool Payments::GetTransactionCountFromDictionary(
    base::DictionaryValue* dictionary,
    uint64_t* transaction_count) const {
  DCHECK(dictionary);
  DCHECK(transaction_count);

  auto* value = dictionary->FindKey("transactionCount");
  if (!value || !value->is_string()) {
    return false;
  }

  auto transaction_count_value = value->GetString();

  // Match a whole number, i.e. 42
  if (!re2::RE2::FullMatch(transaction_count_value, "[+]?[0-9]*")) {
    return false;
  }

  if (!base::StringToUint64(transaction_count_value, transaction_count)) {
    return false;
  }

  return true;
}

bool Payments::HasPendingBalanceForTransactionMonth(
    const std::string& month) const {
  auto payment = GetPaymentForTransactionMonth(month);
  if (payment.balance == 0.0) {
    return false;
  }

  return true;
}

PaymentInfo Payments::GetPaymentForTransactionMonth(
    const std::string& month) const {
  for (const auto& payment : payments_) {
    if (payment.month == month) {
      return payment;
    }
  }

  return PaymentInfo();
}

std::string Payments::GetTransactionMonth(const base::Time& time) const {
  base::Time::Exploded time_exploded;
  time.UTCExplode(&time_exploded);

  return GetFormattedTransactionMonth(time_exploded.year, time_exploded.month);
}

std::string Payments::GetPreviousTransactionMonth(
    const base::Time& time) const {
  base::Time::Exploded time_exploded;
  time.UTCExplode(&time_exploded);

  time_exploded.month--;
  if (time_exploded.month < 1) {
    time_exploded.month = 12;
    time_exploded.year--;
  }

  return GetFormattedTransactionMonth(time_exploded.year, time_exploded.month);
}

std::string Payments::GetFormattedTransactionMonth(
    const int year,
    const int month) const {
  return base::StringPrintf("%04d-%02d", year, month);
}

}  // namespace confirmations
