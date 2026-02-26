/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_

#include <array>
#include <map>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"

namespace brave_wallet {

class PolkadotImportKeyring {
 public:
  explicit PolkadotImportKeyring(mojom::KeyringId keyring_id);
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

  bool IsTestnet() const;
  mojom::KeyringId keyring_id() const { return keyring_id_; }

 private:
  HDKeySr25519* GetAccountByIndex(uint32_t account);

  mojom::KeyringId keyring_id_;
  std::map<uint32_t, HDKeySr25519> accounts_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_IMPORT_KEYRING_H_
