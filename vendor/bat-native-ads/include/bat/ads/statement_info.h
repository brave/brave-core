/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_

#include <cstdint>
#include <string>

#include "bat/ads/export.h"
#include "bat/ads/transaction_info_aliases.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

struct ADS_EXPORT StatementInfo final {
  StatementInfo();
  StatementInfo(const StatementInfo& info);
  ~StatementInfo();

  bool operator==(const StatementInfo& rhs) const;
  bool operator!=(const StatementInfo& rhs) const;

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  double next_payment_date = 0;
  int ads_received_this_month = 0;
  double earnings_this_month = 0.0;
  double earnings_last_month = 0.0;
  TransactionList cleared_transactions;
  TransactionList uncleared_transactions;

 private:
  double GetNextPaymentDateFromDictionary(
      base::DictionaryValue* dictionary) const;

  uint64_t GetAdsReceivedThisMonthFromDictionary(
      base::DictionaryValue* dictionary) const;

  double GetEarningsThisMonthFromDictionary(
      base::DictionaryValue* dictionary) const;
  double GetEarningsLastMonthFromDictionary(
      base::DictionaryValue* dictionary) const;

  base::Value GetClearedTransactionsAsList() const;
  TransactionList GetClearedTransactionsFromDictionary(
      base::DictionaryValue* dictionary) const;
  base::Value GetUnclearedTransactionsAsList() const;
  TransactionList GetUnclearedTransactionsFromDictionary(
      base::DictionaryValue* dictionary) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_STATEMENT_INFO_H_
