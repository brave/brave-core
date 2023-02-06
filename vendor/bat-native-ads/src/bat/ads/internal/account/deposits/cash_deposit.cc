/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/cash_deposit.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"

namespace ads {

void CashDeposit::GetValue(const std::string& creative_instance_id,
                           GetDepositCallback callback) {
  const database::table::Deposits database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          [](GetDepositCallback callback, const bool success,
             const absl::optional<DepositInfo>& deposit) {
            if (!success) {
              std::move(callback).Run(/*success */ false, /* value*/ 0.0);
              return;
            }

            if (!deposit) {
              std::move(callback).Run(/*success */ false, /* value*/ 0.0);
              return;
            }

            std::move(callback).Run(/*success*/ true, deposit->value);
          },
          std::move(callback)));
}

}  // namespace ads
