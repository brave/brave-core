/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet.h"

#include "base/base64.h"
#include "bat/ads/internal/common/crypto/crypto_util.h"
#include "bat/ads/internal/common/crypto/key_pair_info.h"

namespace ads {

bool Wallet::Set(const std::string& payment_id,
                 const std::vector<uint8_t>& recovery_seed) {
  const absl::optional<crypto::KeyPairInfo> key_pair =
      crypto::GenerateSignKeyPairFromSeed(recovery_seed);
  if (!key_pair || !key_pair->IsValid()) {
    return false;
  }

  WalletInfo wallet;
  wallet.payment_id = payment_id;
  wallet.public_key = base::Base64Encode(key_pair->public_key);
  wallet.secret_key = base::Base64Encode(key_pair->secret_key);

  if (!wallet.IsValid()) {
    return false;
  }

  wallet_ = wallet;

  return true;
}

const WalletInfo& Wallet::Get() const {
  return wallet_;
}

}  // namespace ads
