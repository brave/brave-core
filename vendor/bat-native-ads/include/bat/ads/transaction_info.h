/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_TRANSACTION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_TRANSACTION_INFO_H_

#include <string>

#include "bat/ads/export.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

struct ADS_EXPORT TransactionInfo final {
  TransactionInfo();
  TransactionInfo(const TransactionInfo& info);
  ~TransactionInfo();

  bool operator==(const TransactionInfo& rhs) const;
  bool operator!=(const TransactionInfo& rhs) const;

  double timestamp = 0.0;
  double estimated_redemption_value = 0.0;
  std::string confirmation_type;

  void ToDictionary(base::Value* dictionary) const;

  void FromDictionary(base::DictionaryValue* dictionary);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_TRANSACTION_INFO_H_
