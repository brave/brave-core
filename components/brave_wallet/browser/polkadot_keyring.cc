/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot_keyring.h"

#include "base/containers/span.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
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

PolkadotKeyring::PolkadotKeyring(
    base::span<const uint8_t, kSr25519SeedSize> seed,
    mojom::KeyringId keyring_id)
    : root_account_key_(HDKeySr25519::GenerateFromSeed(seed)),
      keyring_id_(keyring_id) {
  // can be useful to remember:
  // https://wiki.polkadot.com/learn/learn-account-advanced/#derivation-paths

  CHECK(IsPolkadotKeyring(keyring_id));

  if (IsTestNet()) {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span("\x1c"
                                                        "westend"));
  } else {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span("\x20"
                                                        "polkadot"));
  }
}

PolkadotKeyring::~PolkadotKeyring() = default;

bool PolkadotKeyring::IsTestNet() const noexcept {
  return keyring_id_ == mojom::KeyringId::kPolkadotTestnet;
}

std::array<uint8_t, kSr25519PublicKeySize> PolkadotKeyring::GetPublicKey(
    uint32_t key_id) {
  auto const& keypair = GetKeypairOrInsert(key_id);
  return keypair.GetPublicKey();
}

std::string PolkadotKeyring::GetUnifiedAddress(uint32_t key_id) {
  auto& keypair = GetKeypairOrInsert(key_id);

  Ss58Address addr;
  addr.prefix = 0;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(
          base::span<uint8_t const>(keypair.GetPublicKey()));

  return addr.Encode().value();
}

std::array<uint8_t, kSr25519SignatureSize> PolkadotKeyring::SignMessage(
    base::span<const uint8_t> message,
    uint32_t key_id) {
  auto const& keypair = GetKeypairOrInsert(key_id);
  return keypair.SignMessage(message);
}

[[nodiscard]] bool PolkadotKeyring::VerifyMessage(
    base::span<const uint8_t, kSr25519SignatureSize> signature,
    base::span<const uint8_t> message,
    uint32_t key_id) {
  auto const& keypair = GetKeypairOrInsert(key_id);
  return keypair.VerifyMessage(signature, message);
}

HDKeySr25519& PolkadotKeyring::GetKeypairOrInsert(uint32_t key_id) {
  auto pos = secondary_keys_.find(key_id);
  if (pos == secondary_keys_.end()) {
    auto [it, inserted] = secondary_keys_.emplace(
        key_id, root_account_key_.DeriveHard(base::U32ToLittleEndian(key_id)));
    pos = it;
  }
  return pos->second;
}

}  // namespace brave_wallet
