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
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

std::optional<WalletInfo> CreateWalletFromRecoverySeed(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  const std::optional<std::vector<uint8_t>> recovery_seed =
      base::Base64Decode(recovery_seed_base64);
  if (!recovery_seed) {
    return std::nullopt;
  }

  const std::optional<crypto::KeyPairInfo> key_pair =
      crypto::GenerateSignKeyPairFromSeed(*recovery_seed);
  if (!key_pair || !key_pair->IsValid()) {
    return std::nullopt;
  }

  WalletInfo wallet{
      .payment_id = payment_id,
      .public_key_base64 = base::Base64Encode(key_pair->public_key),
      .secret_key_base64 = base::Base64Encode(key_pair->secret_key)};

  if (!wallet.IsValid()) {
    return std::nullopt;
  }

  return wallet;
}

std::optional<WalletInfo> CreateWalletFromRecoverySeed(
    const mojom::WalletInfo* const mojom_wallet) {
  CHECK(mojom_wallet);

  return CreateWalletFromRecoverySeed(mojom_wallet->payment_id,
                                      mojom_wallet->recovery_seed_base64);
}

}  // namespace brave_ads
