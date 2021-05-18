/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/payments/payments.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/features/ad_rewards/ad_rewards_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/number_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

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

  payments_ = GetFromList(list);

  return true;
}

bool Payments::SetFromDictionary(base::Value* dictionary) {
  DCHECK(dictionary);

  base::Value* payments_list = dictionary->FindListKey("payments");
  if (!payments_list) {
    return false;
  }

  PaymentList payments;

  for (auto& value : payments_list->GetList()) {
    base::DictionaryValue* dictionary = nullptr;
    if (!value.GetAsDictionary(&dictionary)) {
      continue;
    }

    PaymentInfo payment;

    // Balance
    const base::Optional<double> balance = dictionary->FindDoubleKey("balance");
    if (!balance) {
      continue;
    }
    payment.balance = *balance;

    // Month
    const std::string* month = dictionary->FindStringKey("month");
    if (!month) {
      continue;
    }
    payment.month = *month;

    // Transaction count
    const std::string* transaction_count =
        dictionary->FindStringKey("transaction_count");
    if (!transaction_count) {
      continue;
    }

    if (!base::StringToUint64(*transaction_count, &payment.transaction_count)) {
      continue;
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

bool Payments::DidReconcileBalance(
    const double last_balance,
    const double unreconciled_estimated_pending_rewards) const {
  if (unreconciled_estimated_pending_rewards == 0.0) {
    return true;
  }

  const double delta = GetBalance() - last_balance;
  if (DoubleIsGreaterEqual(delta, unreconciled_estimated_pending_rewards)) {
    return true;
  }

  return false;
}

base::Time Payments::CalculateNextPaymentDate(
    const base::Time& time,
    const base::Time& next_token_redemption_date) const {
  base::Time::Exploded now_exploded;
  time.UTCExplode(&now_exploded);

  int month = now_exploded.month;

  if (now_exploded.day_of_month <= features::GetAdRewardsNextPaymentDay()) {
    const std::string previous_month = GetPreviousTransactionMonth(time);
    if (HasPendingBalanceForTransactionMonth(previous_month)) {
      // If last month has a pending balance, then the next payment date will
      // occur this month
    } else {
      // If last month does not have a pending balance, then the next payment
      // date will occur next month
      month++;
    }
  } else {
    const std::string this_month = GetTransactionMonth(time);
    if (HasPendingBalanceForTransactionMonth(this_month)) {
      // If this month has a pending balance, then the next payment date will
      // occur next month
      month++;
    } else {
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
  next_payment_date_exploded.day_of_month =
      features::GetAdRewardsNextPaymentDay();
  next_payment_date_exploded.hour = 23;
  next_payment_date_exploded.minute = 59;
  next_payment_date_exploded.second = 59;
  next_payment_date_exploded.millisecond = 999;

  base::Time next_payment_date;
  const bool success = base::Time::FromUTCExploded(next_payment_date_exploded,
                                                   &next_payment_date);
  DCHECK(success);

  return next_payment_date;
}

PaymentInfo Payments::GetForThisMonth(const base::Time& time) const {
  const std::string month = GetTransactionMonth(time);
  const PaymentInfo payment = GetPaymentForTransactionMonth(month);
  return payment;
}

///////////////////////////////////////////////////////////////////////////////

PaymentList Payments::GetFromList(base::ListValue* list) const {
  DCHECK(list);

  PaymentList payments;

  for (auto& value : *list) {
    base::DictionaryValue* dictionary = nullptr;
    if (!value.GetAsDictionary(&dictionary)) {
      continue;
    }

    PaymentInfo payment;

    if (!GetBalanceFromDictionary(dictionary, &payment.balance)) {
      continue;
    }

    if (!GetMonthFromDictionary(dictionary, &payment.month)) {
      continue;
    }

    if (!GetTransactionCountFromDictionary(dictionary,
                                           &payment.transaction_count)) {
      continue;
    }

    payments.push_back(payment);
  }

  return payments;
}

bool Payments::GetBalanceFromDictionary(base::DictionaryValue* dictionary,
                                        double* balance) const {
  DCHECK(dictionary);
  DCHECK(balance);

  const std::string* value = dictionary->FindStringKey("balance");
  if (!value) {
    return false;
  }

  // Match a double, i.e. 1.23
  if (!re2::RE2::FullMatch(*value, "[+]?([0-9]*[.])?[0-9]+")) {
    return false;
  }

  if (!base::StringToDouble(*value, balance)) {
    return false;
  }

  return true;
}

bool Payments::GetMonthFromDictionary(base::DictionaryValue* dictionary,
                                      std::string* month) const {
  DCHECK(dictionary);
  DCHECK(month);

  const std::string* value = dictionary->FindStringKey("month");
  if (!value) {
    return false;
  }

  // Match YYYY-MM, i.e. 2019-06
  if (!re2::RE2::FullMatch(*value, "[0-9]{4}-[0-9]{2}")) {
    return false;
  }

  *month = *value;

  return true;
}

bool Payments::GetTransactionCountFromDictionary(
    base::DictionaryValue* dictionary,
    uint64_t* transaction_count) const {
  DCHECK(dictionary);
  DCHECK(transaction_count);

  const std::string* value = dictionary->FindStringKey("transactionCount");
  if (!value) {
    return false;
  }

  // Match a whole number, i.e. 42
  if (!re2::RE2::FullMatch(*value, "[+]?[0-9]*")) {
    return false;
  }

  if (!base::StringToUint64(*value, transaction_count)) {
    return false;
  }

  return true;
}

bool Payments::HasPendingBalanceForTransactionMonth(
    const std::string& month) const {
  const PaymentInfo payment = GetPaymentForTransactionMonth(month);
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

std::string Payments::GetFormattedTransactionMonth(const int year,
                                                   const int month) const {
  return base::StringPrintf("%04d-%02d", year, month);
}

}  // namespace ads
