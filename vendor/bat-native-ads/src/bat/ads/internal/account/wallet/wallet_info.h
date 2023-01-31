/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_

#include <string>

namespace ads {

struct WalletInfo final {
  bool IsValid() const;

  bool WasUpdated(const WalletInfo& other) const;
  bool HasChanged(const WalletInfo& other) const;

  bool operator==(const WalletInfo& other) const;
  bool operator!=(const WalletInfo& other) const;

  std::string payment_id;
  std::string public_key;
  std::string secret_key;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_
