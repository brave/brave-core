/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_TRANSACTION_INFO_
#define BAT_ADS_TRANSACTION_INFO_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT TransactionInfo {
  TransactionInfo();
  TransactionInfo(
        const TransactionInfo& info);
  ~TransactionInfo();

  uint64_t timestamp_in_seconds = 0;
  double estimated_redemption_value = 0.0;
  std::string confirmation_type;

  void ToDictionary(
      base::Value* dictionary) const;

  void FromDictionary(
      base::DictionaryValue* dictionary);
};

using TransactionList = std::vector<TransactionInfo>;

}  // namespace ads

#endif  // BAT_LEDGER_TRANSACTION_INFO_
