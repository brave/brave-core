/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PAYMENTS_H_
#define BAT_CONFIRMATIONS_INTERNAL_PAYMENTS_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "bat/confirmations/internal/payment_info.h"

#include "base/time/time.h"
#include "base/values.h"

namespace confirmations {

class ConfirmationsImpl;

class Payments {
 public:
  Payments();

  ~Payments();

  bool SetFromJson(const std::string& json);
  bool SetFromDictionary(base::DictionaryValue* dictionary);
  base::Value GetAsList();

  double GetBalance() const;

  base::Time CalculateNextPaymentDate(
      const base::Time& time,
      const uint64_t token_redemption_date_in_seconds) const;

  uint64_t GetTransactionCountForMonth(const base::Time& time) const;

 private:
  PaymentList payments_;
  PaymentList GetPaymentsFromList(base::ListValue* list) const;
  bool GetBalanceFromDictionary(
      base::DictionaryValue* dictionary,
      double* balance) const;
  bool GetMonthFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* month) const;
  bool GetTransactionCountFromDictionary(
      base::DictionaryValue* dictionary,
      uint64_t* transaction_count) const;

  bool HasPendingBalanceForTransactionMonth(const std::string& month) const;

  PaymentInfo GetPaymentForTransactionMonth(const std::string& month) const;

  std::string GetTransactionMonth(const base::Time& time) const;
  std::string GetPreviousTransactionMonth(const base::Time& time) const;

  std::string GetFormattedTransactionMonth(
      const int year,
      const int month) const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PAYMENTS_H_
