/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/payments.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/confirmations_impl.h"

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "third_party/re2/src/re2/re2.h"

namespace confirmations {

Payments::Payments(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    payments_({}),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing payments";
  (void)confirmations_;
}

Payments::~Payments() {
  BLOG(INFO) << "Deinitializing payments";
}

bool Payments::ParseJson(const std::string& json) {
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
  time.LocalExplode(&now_exploded);

  int month = now_exploded.month;

  if (now_exploded.day_of_month <= kNextPaymentDay) {
    if (HasPendingBalanceForLastMonth()) {
      // If last month has a pending balance, then the next payment date will
      // occur this month
    } else {
      // If last month does not have a pending balance, then the next payment
      // date will occur next month
      month++;
    }
  } else {
    if (HasPendingBalanceForThisMonth()) {
      // If this month has a pending balance, then the next payment date will
      // occur next month
      month++;
    } else {
      auto next_token_redemption_date =
          base::Time::FromDoubleT(next_token_redemption_date_in_seconds);

      base::Time::Exploded next_token_redemption_date_exploded;
      next_token_redemption_date.LocalExplode(
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

  auto date = base::StringPrintf("%d-%d-%d", year, month, kNextPaymentDay);

  base::Time next_payment_date;
  bool success = base::Time::FromString(date.c_str(), &next_payment_date);
  DCHECK(success);

  return next_payment_date;
}

uint64_t Payments::GetTransactionCountForThisMonth() const {
  if (payments_.empty()) {
    return 0;
  }

  PaymentInfo payment(payments_.front());
  return payment.transaction_count;
}

///////////////////////////////////////////////////////////////////////////////

std::vector<PaymentInfo> Payments::GetPaymentsFromList(
    base::ListValue* list) const {
  DCHECK(list);

  std::vector<PaymentInfo> payments;

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

  if (!re2::RE2::FullMatch(balance_value, "[+]?([0-9]*[.])?[0-9]+")) {
    return false;
  }

  *balance = std::stod(balance_value);

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

  if (!re2::RE2::FullMatch(transaction_count_value, "[+]?[0-9]*")) {
    return false;
  }

  *transaction_count = std::stoull(transaction_count_value);

  return true;
}

bool Payments::HasPendingBalanceForLastMonth() const {
  if (payments_.size() < 2) {
    return false;
  }

  PaymentInfo payment(payments_.at(1));
  if (payment.balance == 0.0) {
    return false;
  }

  return true;
}

bool Payments::HasPendingBalanceForThisMonth() const {
  if (payments_.empty()) {
    return false;
  }

  PaymentInfo payment(payments_.front());
  if (payment.balance == 0.0) {
    return false;
  }

  return true;
}

}  // namespace confirmations
