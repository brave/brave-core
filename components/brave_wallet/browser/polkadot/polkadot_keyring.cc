/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include "base/base64.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/json/json_writer.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/browser/scrypt_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "crypto/process_bound_string.h"

namespace brave_wallet {

namespace {

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

  CHECK(IsPolkadotKeyring(keyring_id_));

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

std::optional<std::string> PolkadotKeyring::AddNewHDAccount(uint32_t index) {
  return GetAddress(index, IsTestnet() ? kWestendPrefix : kPolkadotPrefix);
}

std::array<uint8_t, kSr25519PublicKeySize> PolkadotKeyring::GetPublicKey(
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.GetPublicKey();
}

std::array<uint8_t, kSr25519Pkcs8Size>
PolkadotKeyring::GetPkcs8KeyForTesting(  // IN-TEST
    uint32_t account_index) {
  CHECK_IS_TEST();
  return EnsureKeyPair(account_index).GetExportKeyPkcs8();
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

void PolkadotKeyring::SetRandBytesForTesting(  // IN-TEST
    const std::array<uint8_t, kScryptSaltSize>& salt_bytes,
    const std::array<uint8_t, kSecretboxNonceSize>& nonce_bytes) {
  CHECK_IS_TEST();
  rand_salt_bytes_for_testing_ = salt_bytes;
  rand_nonce_bytes_for_testing_ = nonce_bytes;
}

void PolkadotKeyring::SetSignatureRngForTesting() {
  CHECK_IS_TEST();
  for (auto& [idx, keypair] : secondary_keys_) {
    keypair.UseMockRngForTesting();  // IN-TEST
  }
}

std::optional<std::string> PolkadotKeyring::EncodePrivateKeyForExport(
    uint32_t account_index,
    std::string_view password) {
  auto pkcs8_key = EnsureKeyPair(account_index).GetExportKeyPkcs8();
  std::string address = GetAddress(account_index, kSubstratePrefix);
  const std::array<uint8_t, kScryptSaltSize>* salt_for_testing = nullptr;
  const std::array<uint8_t, kSecretboxNonceSize>* nonce_for_testing = nullptr;
  if (rand_salt_bytes_for_testing_.has_value()) {
    salt_for_testing = &*rand_salt_bytes_for_testing_;
  }
  if (rand_nonce_bytes_for_testing_.has_value()) {
    nonce_for_testing = &*rand_nonce_bytes_for_testing_;
  }
  return ::brave_wallet::EncodePrivateKeyForExport(
      pkcs8_key, address, password, salt_for_testing, nonce_for_testing);
}

}  // namespace brave_wallet
