/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include "base/check_is_test.h"
#include "base/containers/map_util.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/browser/scrypt_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"

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
    mojom::KeyringId keyring_id,
    base::RepeatingCallback<bool(const std::string&)> is_address_allowed)
    : root_account_key_(HDKeySr25519::GenerateFromSeed(seed)),
      keyring_id_(keyring_id),
      is_address_allowed_(std::move(is_address_allowed)) {
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

std::optional<std::array<uint8_t, kSr25519PublicKeySize>>
PolkadotKeyring::GetPublicKey(uint32_t account_index) {
  const auto* keypair = GetKeypair(account_index);
  if (!keypair) {
    return std::nullopt;
  }
  return keypair->GetPublicKey();
}

std::array<uint8_t, kSr25519Pkcs8Size>
PolkadotKeyring::GetPkcs8KeyForTesting(  // IN-TEST
    uint32_t account_index) {
  CHECK_IS_TEST();
  const auto* keypair = GetKeypair(account_index);
  // This is a test, so we can require correct inputs unconditionally.
  CHECK(keypair);
  return keypair->GetExportKeyPkcs8();
}

std::optional<std::string> PolkadotKeyring::GetAddress(uint32_t account_index,
                                                       uint16_t prefix) {
  auto* keypair = GetKeypair(account_index);
  if (!keypair) {
    return std::nullopt;
  }

  Ss58Address addr;
  addr.prefix = prefix;
  addr.public_key = keypair->GetPublicKey();
  return addr.Encode();
}

std::optional<std::array<uint8_t, kSr25519SignatureSize>>
PolkadotKeyring::SignMessage(base::span<const uint8_t> message,
                             uint32_t account_index) {
  auto const* keypair = GetKeypair(account_index);
  if (!keypair) {
    return std::nullopt;
  }

  return keypair->SignMessage(message);
}

[[nodiscard]] bool PolkadotKeyring::VerifyMessage(
    base::span<const uint8_t, kSr25519SignatureSize> signature,
    base::span<const uint8_t> message,
    uint32_t account_index) {
  const auto* keypair = GetKeypair(account_index);
  if (!keypair) {
    return false;
  }

  return keypair->VerifyMessage(signature, message);
}

HDKeySr25519* PolkadotKeyring::GetKeypair(uint32_t account_index) {
  return base::FindOrNull(secondary_keys_, account_index);
}

std::optional<std::string> PolkadotKeyring::AddNewHDAccount(
    uint32_t account_index) {
  if (secondary_keys_.contains(account_index)) {
    // Account already exists.
    return std::nullopt;
  }

  auto keypair =
      root_account_key_.DeriveHard(base::byte_span_from_ref(account_index));

  Ss58Address ss58_addr;
  ss58_addr.prefix = IsTestnet() ? kWestendPrefix : kPolkadotPrefix;
  base::span(ss58_addr.public_key)
      .copy_from_nonoverlapping(keypair.GetPublicKey());

  const auto addr = ss58_addr.Encode();
  if (!addr || !is_address_allowed_.Run(*addr)) {
    // We either failed to ss58-encode the public key or it was present in our
    // block-list. Either way, reject keypair creation.
    return std::nullopt;
  }

  secondary_keys_.emplace(account_index, std::move(keypair));
  return addr;
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
  auto* keypair = GetKeypair(account_index);
  if (!keypair) {
    return std::nullopt;
  }

  return PolkadotKeyring::EncodePrivateKeyForExport(
      *keypair, password, rand_salt_bytes_for_testing_,
      rand_nonce_bytes_for_testing_);
}

// static
std::optional<std::string> PolkadotKeyring::EncodePrivateKeyForExport(
    const HDKeySr25519& keypair,
    std::string_view password,
    const std::optional<std::array<uint8_t, kScryptSaltSize>>& salt_for_testing,
    const std::optional<std::array<uint8_t, kSecretboxNonceSize>>&
        nonce_for_testing) {
  auto pkcs8_key = keypair.GetExportKeyPkcs8();

  Ss58Address addr;
  addr.prefix = kSubstratePrefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(
          base::span<uint8_t const>(keypair.GetPublicKey()));
  auto address = addr.Encode();
  if (!address) {
    crypto::internal::SecureZeroBuffer(pkcs8_key);
    return std::nullopt;
  }

  std::array<uint8_t, kScryptSaltSize> salt = {};
  std::array<uint8_t, kSecretboxNonceSize> nonce = {};
  if (salt_for_testing.has_value()) {
    CHECK_IS_TEST();
    base::span(salt).copy_from_nonoverlapping(*salt_for_testing);
  } else {
    crypto::RandBytes(base::span(salt));
  }
  if (nonce_for_testing.has_value()) {
    CHECK_IS_TEST();
    base::span(nonce).copy_from_nonoverlapping(*nonce_for_testing);
  } else {
    crypto::RandBytes(base::span(nonce));
  }

  auto result = ::brave_wallet::EncodePrivateKeyForExport(
      pkcs8_key, *address, password, salt, nonce);
  crypto::internal::SecureZeroBuffer(pkcs8_key);
  return result;
}

}  // namespace brave_wallet
