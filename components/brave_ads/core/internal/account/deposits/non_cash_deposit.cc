/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/non_cash_deposit.h"

#include <utility>

#include "base/functional/callback.h"

namespace brave_ads {

void NonCashDeposit::GetValue(const std::string& /*creative_instance_id=*/,
                              GetDepositCallback callback) {
  std::move(callback).Run(/*success=*/true, /* value=*/0.0);
}

}  // namespace brave_ads
