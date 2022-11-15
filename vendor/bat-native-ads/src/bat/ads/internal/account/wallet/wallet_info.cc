/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet_info.h"

namespace ads {

bool WalletInfo::IsValid() const {
  return !id.empty() && !secret_key.empty();
}

bool WalletInfo::WasUpdated(const WalletInfo& other) const {
  return *this != other;
}

bool WalletInfo::HasChanged(const WalletInfo& other) const {
  return other.IsValid() && WasUpdated(other);
}

bool WalletInfo::operator==(const WalletInfo& other) const {
  return id == other.id && secret_key == other.secret_key;
}

bool WalletInfo::operator!=(const WalletInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
