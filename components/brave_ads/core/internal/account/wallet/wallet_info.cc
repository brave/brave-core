/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"

#include <tuple>

namespace brave_ads {

bool WalletInfo::IsValid() const {
  return !payment_id.empty() && !public_key.empty() && !secret_key.empty();
}

bool WalletInfo::operator==(const WalletInfo& other) const {
  const auto tie = [](const WalletInfo& wallet) {
    return std::tie(wallet.payment_id, wallet.public_key, wallet.secret_key);
  };

  return tie(*this) == tie(other);
}

bool WalletInfo::operator!=(const WalletInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
