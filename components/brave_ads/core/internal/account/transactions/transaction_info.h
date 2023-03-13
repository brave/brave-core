/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"

namespace brave_ads {

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

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTION_INFO_H_
