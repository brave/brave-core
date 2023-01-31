/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/internal/account/wallet/wallet_info.h"

namespace ads {

class Wallet final {
 public:
  bool Set(const std::string& payment_id,
           const std::vector<uint8_t>& recovery_seed);
  const WalletInfo& Get() const;

 private:
  WalletInfo wallet_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_
