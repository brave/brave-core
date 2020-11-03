/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet.h"

#include <stdint.h>

#include <vector>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/security/security_util.h"
#include "bat/ads/internal/string_util.h"

namespace ads {

Wallet::Wallet(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Wallet::~Wallet() = default;

bool Wallet::Set(
    const std::string& id,
    const std::string& seed) {
  const std::vector<uint8_t> secret_key =
      security::GenerateSecretKeyFromSeed(seed);

  if (secret_key.empty()) {
    BLOG(0, "Invalid wallet secret key");
    return false;
  }

  WalletInfo wallet;
  wallet.id = id;
  wallet.secret_key = BytesToHexString(secret_key);

  if (!wallet.IsValid()) {
    BLOG(0, "Invalid wallet");
    return false;
  }

  if (wallet_ != wallet) {
    wallet_ = wallet;

    ads_->ReconcileAdRewards();
  }

  return true;
}

WalletInfo Wallet::Get() const {
  return wallet_;
}

}  // namespace ads
