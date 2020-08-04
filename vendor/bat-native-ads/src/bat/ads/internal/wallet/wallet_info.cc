/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/wallet/wallet_info.h"

namespace ads {

WalletInfo::WalletInfo() = default;

WalletInfo::WalletInfo(
    const WalletInfo& info) = default;

WalletInfo::~WalletInfo() = default;

bool WalletInfo::IsValid() const {
  return !payment_id.empty() && !secret_key.empty();
}

bool WalletInfo::operator==(
    const WalletInfo& rhs) const {
  return (payment_id == rhs.payment_id && secret_key == rhs.secret_key);
}

bool WalletInfo::operator!=(
    const WalletInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
