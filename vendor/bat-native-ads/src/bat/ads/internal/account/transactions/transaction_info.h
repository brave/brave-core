/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"

namespace ads {

struct TransactionInfo final {
  TransactionInfo();

  TransactionInfo(const TransactionInfo& other);
  TransactionInfo& operator=(const TransactionInfo& other);

  TransactionInfo(TransactionInfo&& other) noexcept;
  TransactionInfo& operator=(TransactionInfo&& other) noexcept;

  ~TransactionInfo();

  bool operator==(const TransactionInfo& other) const;
  bool operator!=(const TransactionInfo& other) const;

  bool IsValid() const;

  std::string id;
  base::Time created_at;
  std::string creative_instance_id;
  double value = 0.0;
  AdType ad_type = AdType::kUndefined;
  ConfirmationType confirmation_type = ConfirmationType::kUndefined;
  base::Time reconciled_at;
};

using TransactionList = std::vector<TransactionInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_
