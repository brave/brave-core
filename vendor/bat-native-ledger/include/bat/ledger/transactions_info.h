/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_TRANSACTIONS_INFO_
#define BAT_LEDGER_TRANSACTIONS_INFO_

#include <string>
#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/transaction_info.h"

namespace ledger {

LEDGER_EXPORT struct TransactionsInfo {
  TransactionsInfo();
  explicit TransactionsInfo(const TransactionsInfo& info);
  ~TransactionsInfo();

  const std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::vector<TransactionInfo> transactions;
};

}  // namespace ledger

#endif  // BAT_LEDGER_TRANSACTIONS_INFO_
