/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_TRANSACTIONS_INFO_
#define BAT_LEDGER_TRANSACTIONS_INFO_

#include <stdint.h>
#include <string>
#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/transaction_info.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ledger {

LEDGER_EXPORT struct TransactionsInfo {
  TransactionsInfo();
  explicit TransactionsInfo(const TransactionsInfo& info);
  ~TransactionsInfo();

  const std::string ToJson() const;
  bool FromJson(const std::string& json);

  double estimated_pending_rewards;
  uint64_t next_payment_date_in_seconds;
  uint64_t ad_notifications_received_this_month;
  std::vector<TransactionInfo> transactions;

 private:
  double GetEstimatedPendingRewardsFromJson(
    base::DictionaryValue* dictionary) const;

  uint64_t GetNextPaymentDateInSecondsFromJson(
    base::DictionaryValue* dictionary) const;

  uint64_t GetAdNotificationsReceivedThisMonthFromJson(
    base::DictionaryValue* dictionary) const;

  base::Value GetTransactionsAsList() const;
  std::vector<TransactionInfo> GetTransactionsFromJson(
    base::DictionaryValue* dictionary) const;
};

}  // namespace ledger

#endif  // BAT_LEDGER_TRANSACTIONS_INFO_
