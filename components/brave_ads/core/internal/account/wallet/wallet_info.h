/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_

#include <string>

namespace brave_ads {

struct WalletInfo final {
  bool operator==(const WalletInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  std::string payment_id;
  std::string public_key_base64;
  std::string secret_key_base64;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_INFO_H_
