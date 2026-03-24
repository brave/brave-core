/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_

#include <array>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/browser/scrypt_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"

namespace brave_wallet {

// This class is used to import a private key into a Polkadot keyring.
// It contains a flat_map of account indices to HDKeySr25519 objects.
// Accounts are added using the AddAccount method when wallet is created.
class PolkadotImportKeyring {
 public:
  PolkadotImportKeyring(
      mojom::KeyringId keyring_id,
      base::RepeatingCallback<bool(const std::string&)> is_address_allowed);
  ~PolkadotImportKeyring();

  PolkadotImportKeyring(const PolkadotImportKeyring&) = delete;
  PolkadotImportKeyring& operator=(const PolkadotImportKeyring&) = delete;

  bool AddAccount(uint32_t account,
                  base::span<const uint8_t, kSr25519Pkcs8Size> pkcs8_key);
  bool RemoveAccount(uint32_t account);

  // Returns SS58 address with prefix determined by keyring_id (mainnet vs
  // testnet).
  std::optional<std::string> GetAccountAddress(uint32_t account_index);
  std::optional<std::string> GetAddress(uint32_t account_index,
                                        uint16_t prefix);
  std::optional<std::array<uint8_t, kSr25519PublicKeySize>> GetPublicKey(
      uint32_t account_index);
  std::optional<std::array<uint8_t, kSr25519SignatureSize>> SignMessage(
      base::span<const uint8_t> message,
      uint32_t account_index);

  std::optional<std::string> EncodePrivateKeyForExport(
      uint32_t account_index,
      std::string_view password);

  // Sets random bytes for testing for private key export.
  void SetRandBytesForTesting(
      const std::array<uint8_t, kScryptSaltSize>& salt_bytes,
      const std::array<uint8_t, kSecretboxNonceSize>& nonce_bytes);

  bool IsTestnet() const;
  mojom::KeyringId keyring_id() const { return keyring_id_; }

 private:
  HDKeySr25519* GetAccountByIndex(uint32_t account);

  mojom::KeyringId keyring_id_;
  base::flat_map<uint32_t, HDKeySr25519> accounts_;

  base::RepeatingCallback<bool(const std::string&)> is_address_allowed_;

  std::optional<std::array<uint8_t, kScryptSaltSize>>
      rand_salt_bytes_for_testing_;
  std::optional<std::array<uint8_t, kSecretboxNonceSize>>
      rand_nonce_bytes_for_testing_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_
