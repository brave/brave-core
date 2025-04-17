/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_HD_KEYRING_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519_slip23.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

inline constexpr uint32_t kCardanoSignatureSize =
    kEd25519PublicKeySize + kEd25519SignatureSize;

// Keyring based on SLIP-0023 keys.
class CardanoHDKeyring {
 public:
  CardanoHDKeyring(base::span<const uint8_t> entropy,
                   mojom::KeyringId keyring_id);
  ~CardanoHDKeyring();
  CardanoHDKeyring(const CardanoHDKeyring&) = delete;
  CardanoHDKeyring& operator=(const CardanoHDKeyring&) = delete;

  mojom::CardanoAddressPtr GetAddress(
      uint32_t account,
      const mojom::CardanoKeyId& payment_key_id);

  std::optional<std::string> AddNewHDAccount(uint32_t index);

  std::optional<std::array<uint8_t, kCardanoSignatureSize>> SignMessage(
      uint32_t account,
      const mojom::CardanoKeyId& key_id,
      base::span<const uint8_t> message);

  mojom::KeyringId keyring_id() const { return keyring_id_; }
  bool IsTestnet() const;

 private:
  std::unique_ptr<HDKeyEd25519Slip23> DeriveAccount(uint32_t index) const;
  std::unique_ptr<HDKeyEd25519Slip23> DeriveKey(
      uint32_t account,
      const mojom::CardanoKeyId& key_id);

  std::unique_ptr<HDKeyEd25519Slip23> accounts_root_;

  mojom::KeyringId keyring_id_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_HD_KEYRING_H_
