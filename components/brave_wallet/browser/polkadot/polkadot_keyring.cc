/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

namespace {

constexpr uint8_t kPolkadotPrefix = 0u;
constexpr uint8_t kWestendPrefix = 42u;

inline constexpr char const kPolkadotTestnet[] =
    "\x1c"
    "westend";

inline constexpr char const kPolkadotMainnet[] =
    "\x20"
    "polkadot";

}  // namespace

PolkadotKeyring::PolkadotKeyring(
    base::span<const uint8_t, kPolkadotSeedSize> seed,
    mojom::KeyringId keyring_id)
    : root_account_key_(HDKeySr25519::GenerateFromSeed(seed)),
      keyring_id_(keyring_id) {
  // Can be useful to remember:
  // https://wiki.polkadot.com/learn/learn-account-advanced/#derivation-paths

  CHECK(IsPolkadotKeyring(keyring_id));

  if (IsTestnet()) {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span(kPolkadotTestnet));
  } else {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span(kPolkadotMainnet));
  }
}

PolkadotKeyring::~PolkadotKeyring() = default;

bool PolkadotKeyring::IsTestnet() const {
  return keyring_id_ == mojom::KeyringId::kPolkadotTestnet;
}

std::array<uint8_t, kSr25519PublicKeySize> PolkadotKeyring::GetPublicKey(
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.GetPublicKey();
}

std::string PolkadotKeyring::GetAddress(uint32_t account_index,
                                        uint16_t prefix) {
  auto& keypair = EnsureKeyPair(account_index);

  Ss58Address addr;
  addr.prefix = prefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(
          base::span<uint8_t const>(keypair.GetPublicKey()));

  return addr.Encode().value();
}

std::array<uint8_t, kSr25519SignatureSize> PolkadotKeyring::SignMessage(
    base::span<const uint8_t> message,
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.SignMessage(message);
}

[[nodiscard]] bool PolkadotKeyring::VerifyMessage(
    base::span<const uint8_t, kSr25519SignatureSize> signature,
    base::span<const uint8_t> message,
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.VerifyMessage(signature, message);
}

HDKeySr25519& PolkadotKeyring::EnsureKeyPair(uint32_t account_index) {
  auto pos = secondary_keys_.find(account_index);
  if (pos == secondary_keys_.end()) {
    auto [it, inserted] = secondary_keys_.emplace(
        account_index,
        root_account_key_.DeriveHard(base::byte_span_from_ref(account_index)));
    pos = it;
  }
  return pos->second;
}

std::optional<std::string> PolkadotKeyring::AddNewHDAccount(uint32_t index) {
  return GetAddress(index, IsTestnet() ? kWestendPrefix : kPolkadotPrefix);
}

}  // namespace brave_wallet
