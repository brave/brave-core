/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"

#include <vector>

#include "base/base64.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/crypto/key_pair_info.h"

namespace brave_ads {

absl::optional<WalletInfo> ToWallet(const std::string& payment_id,
                                    const std::string& recovery_seed) {
  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(recovery_seed);
  if (!raw_recovery_seed) {
    return absl::nullopt;
  }

  const absl::optional<crypto::KeyPairInfo> key_pair =
      crypto::GenerateSignKeyPairFromSeed(*raw_recovery_seed);
  if (!key_pair || !key_pair->IsValid()) {
    return absl::nullopt;
  }

  WalletInfo wallet;
  wallet.payment_id = payment_id;
  wallet.public_key = base::Base64Encode(key_pair->public_key);
  wallet.secret_key = base::Base64Encode(key_pair->secret_key);

  if (!wallet.IsValid()) {
    return absl::nullopt;
  }

  return wallet;
}

}  // namespace brave_ads
