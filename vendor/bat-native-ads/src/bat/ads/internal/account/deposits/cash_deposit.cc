/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/cash_deposit.h"

#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

CashDeposit::CashDeposit() = default;

CashDeposit::~CashDeposit() = default;

void CashDeposit::GetValue(const std::string& creative_instance_id,
                           GetDepositCallback callback) {
  database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [callback](const bool success,
                 const absl::optional<DepositInfo>& deposit_optional) {
        if (!success) {
          callback(/* success */ false, /* value */ 0.0);
          return;
        }

        if (!deposit_optional) {
          callback(/* success */ false, /* value */ 0.0);
          return;
        }

        const DepositInfo& deposit = deposit_optional.value();
        callback(/* success */ true, deposit.value);
      });
}

}  // namespace ads
