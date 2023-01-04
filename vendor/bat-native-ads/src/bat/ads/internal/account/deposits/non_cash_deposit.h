/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_NON_CASH_DEPOSIT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_NON_CASH_DEPOSIT_H_

#include <string>

#include "bat/ads/internal/account/deposits/deposit_interface.h"

namespace ads {

class NonCashDeposit final : public DepositInterface {
 public:
  void GetValue(const std::string& creative_instance_id,
                GetDepositCallback callback) override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_NON_CASH_DEPOSIT_H_
