/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet.h"

#include <stdint.h>

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/security/security_util.h"

namespace ads {

Wallet::Wallet() = default;

Wallet::~Wallet() = default;

bool Wallet::Set(
    const std::string& id,
    const std::string& seed) {
  const std::vector<uint8_t> secret_key =
      security::GenerateSecretKeyFromSeed(seed);

  if (secret_key.empty()) {
    return false;
  }

  WalletInfo wallet;
  wallet.id = id;
  wallet.secret_key = base::HexEncode(secret_key.data(), secret_key.size());

  if (!wallet.IsValid()) {
    return false;
  }

  wallet_ = wallet;

  return true;
}

WalletInfo Wallet::Get() const {
  return wallet_;
}

}  // namespace ads
