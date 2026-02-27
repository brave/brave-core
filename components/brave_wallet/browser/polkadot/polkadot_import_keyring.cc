/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_import_keyring.h"

#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/rand_util.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

PolkadotImportKeyring::PolkadotImportKeyring(mojom::KeyringId keyring_id)
    : keyring_id_(keyring_id) {
  CHECK(IsPolkadotImportKeyring(keyring_id_));
}

PolkadotImportKeyring::~PolkadotImportKeyring() = default;

bool PolkadotImportKeyring::AddAccount(
    uint32_t account,
    base::span<const uint8_t, kSr25519Pkcs8Size> pkcs8_key) {
  if (accounts_.contains(account)) {
    return false;
  }
  auto key = HDKeySr25519::CreateFromPkcs8(pkcs8_key);
  if (!key) {
    return false;
  }
  accounts_.emplace(account, std::move(*key));
  return true;
}

bool PolkadotImportKeyring::RemoveAccount(uint32_t account) {
  return accounts_.erase(account) > 0;
}

std::optional<std::string> PolkadotImportKeyring::GetAccountAddress(
    uint32_t account_index) {
  const uint16_t prefix = IsTestnet() ? kWestendPrefix : kPolkadotPrefix;
  return GetAddress(account_index, prefix);
}

std::optional<std::string> PolkadotImportKeyring::GetAddress(
    uint32_t account_index,
    uint16_t prefix) {
  auto* key = GetAccountByIndex(account_index);
  if (!key) {
    return std::nullopt;
  }

  Ss58Address addr;
  addr.prefix = prefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(base::span<uint8_t const>(key->GetPublicKey()));
  return addr.Encode();
}

std::optional<std::array<uint8_t, kSr25519PublicKeySize>>
PolkadotImportKeyring::GetPublicKey(uint32_t account_index) {
  auto* key = GetAccountByIndex(account_index);
  if (!key) {
    return std::nullopt;
  }
  return key->GetPublicKey();
}

std::optional<std::array<uint8_t, kSr25519SignatureSize>>
PolkadotImportKeyring::SignMessage(base::span<const uint8_t> message,
                                   uint32_t account_index) {
  auto* key = GetAccountByIndex(account_index);
  if (!key) {
    return std::nullopt;
  }
  return key->SignMessage(message);
}

std::optional<std::string> PolkadotImportKeyring::EncodePrivateKeyForExport(
    uint32_t account_index,
    std::string_view password) {
  auto* key = GetAccountByIndex(account_index);
  if (!key) {
    return std::nullopt;
  }

  auto pkcs8_key = key->GetExportKeyPkcs8();
  auto address = GetAccountAddress(account_index);
  if (!address.has_value()) {
    return std::nullopt;
  }

  std::array<uint8_t, kScryptSaltSize> salt;
  std::array<uint8_t, kSecretboxNonceSize> nonce;
  if (rand_salt_bytes_for_testing_.has_value()) {
    base::span(salt).copy_from_nonoverlapping(*rand_salt_bytes_for_testing_);
  } else {
    base::RandBytes(base::span(salt));
  }
  if (rand_nonce_bytes_for_testing_.has_value()) {
    base::span(nonce).copy_from_nonoverlapping(*rand_nonce_bytes_for_testing_);
  } else {
    base::RandBytes(base::span(nonce));
  }
  return ::brave_wallet::EncodePrivateKeyForExport(pkcs8_key, *address,
                                                   password, salt, nonce);
}

void PolkadotImportKeyring::SetRandBytesForTesting(  // IN-TEST
    const std::array<uint8_t, kScryptSaltSize>& salt_bytes,
    const std::array<uint8_t, kSecretboxNonceSize>& nonce_bytes) {
  CHECK_IS_TEST();
  rand_salt_bytes_for_testing_ = salt_bytes;
  rand_nonce_bytes_for_testing_ = nonce_bytes;
}

bool PolkadotImportKeyring::IsTestnet() const {
  return keyring_id_ == mojom::KeyringId::kPolkadotImportTestnet;
}

HDKeySr25519* PolkadotImportKeyring::GetAccountByIndex(uint32_t account) {
  auto it = accounts_.find(account);
  if (it == accounts_.end()) {
    return nullptr;
  }
  return &it->second;
}

}  // namespace brave_wallet
