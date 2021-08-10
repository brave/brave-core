/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_H_

#include <cstdint>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/account/ad_rewards/payments/payment_info.h"

namespace ads {

class Payments {
 public:
  Payments();

  ~Payments();

  bool SetFromJson(const std::string& json);
  bool SetFromDictionary(base::Value* dictionary);
  base::Value GetAsList();

  double GetBalance() const;

  base::Time CalculateNextPaymentDate(
      const base::Time& time,
      const base::Time& token_redemption_date) const;

  PaymentInfo GetForMonth(const base::Time& time) const;

  void reset() { payments_ = {}; }

 private:
  PaymentList payments_;

  double GetBalanceForPayments(const PaymentList& payments) const;

  bool ShouldForceReconciliation(const PaymentList& payments) const;

  bool DidReconcile(const PaymentList& payments) const;

  PaymentList GetFromList(base::ListValue* list) const;

  bool GetBalanceFromDictionary(base::DictionaryValue* dictionary,
                                double* balance) const;
  bool GetMonthFromDictionary(base::DictionaryValue* dictionary,
                              std::string* month) const;
  bool GetTransactionCountFromDictionary(base::DictionaryValue* dictionary,
                                         uint64_t* transaction_count) const;

  bool HasPendingBalanceForTransactionMonth(const std::string& month) const;

  PaymentInfo GetPaymentForTransactionMonth(const std::string& month) const;

  std::string GetTransactionMonth(const base::Time& time) const;
  std::string GetPreviousTransactionMonth(const base::Time& time) const;

  std::string GetFormattedTransactionMonth(const int year,
                                           const int month) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_PAYMENTS_PAYMENTS_H_
