/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/non_cash_deposit.h"

namespace ads {

NonCashDeposit::NonCashDeposit() = default;

NonCashDeposit::~NonCashDeposit() = default;

void NonCashDeposit::GetValue(const std::string& creative_instance_id,
                              GetDepositCallback callback) {
  callback(/* success */ true, /* value */ 0.0);
}

}  // namespace ads
