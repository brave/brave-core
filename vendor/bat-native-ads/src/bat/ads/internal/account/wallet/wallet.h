/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_

#include <string>

#include "bat/ads/internal/account/wallet/wallet_info.h"

namespace ads {

class Wallet final {
 public:
  bool Set(const std::string& payment_id, const std::string& recovery_seed);

  const WalletInfo& Get() const { return wallet_; }
  const std::string& GetId() const { return wallet_.id; }
  const std::string& GetSecretKey() const { return wallet_.secret_key; }

 private:
  WalletInfo wallet_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_H_
