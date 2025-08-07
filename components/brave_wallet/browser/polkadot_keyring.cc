/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot_keyring.h"

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet {

std::optional<std::array<uint8_t, kSr25519SeedSize>>
PolkadotKeyring::MnemonicToSeed(std::string_view mnemonic,
                                std::string_view password) {
  // https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki#from-mnemonic-to-seed
  constexpr uint32_t kPBKDF2Iterations = 2048;
  constexpr uint32_t kSeedSize = 64;

  auto entropy = bip39::MnemonicToEntropy(mnemonic);
  if (!entropy) {
    return std::nullopt;
  }

  crypto::SecureString salt;
  salt.reserve(8 + password.size());
  salt.append("mnemonic");
  salt.append(password);

  std::array<uint8_t, kSeedSize> seed = {};

  if (!crypto::kdf::DeriveKeyPbkdf2HmacSha512({.iterations = kPBKDF2Iterations},
                                              *entropy,
                                              base::as_byte_span(salt), seed)) {
    return std::nullopt;
  }

  std::array<uint8_t, kSr25519SeedSize> sr25519_seed = {};
  base::span(sr25519_seed)
      .copy_from_nonoverlapping(base::span(seed).first(kSr25519SeedSize));

  return sr25519_seed;
}

PolkadotKeyring::~PolkadotKeyring() = default;

}  // namespace brave_wallet
