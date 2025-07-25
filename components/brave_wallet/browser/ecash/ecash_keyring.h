/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ECASH_ECASH_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ECASH_ECASH_KEYRING_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ECashKeyring : public Secp256k1HDKeyring {
 public:
  ECashKeyring(base::span<const uint8_t> seed, mojom::KeyringId keyring_id);
  ~ECashKeyring() override = default;
  ECashKeyring(const ECashKeyring&) = delete;
  ECashKeyring& operator=(const ECashKeyring&) = delete;

  mojom::ECashAddressPtr GetAddress(const mojom::ECashKeyId& key_id);
  std::optional<std::vector<uint8_t>> GetPubkey(
      const mojom::ECashKeyId& key_id);
  std::optional<std::vector<uint8_t>> SignMessage(
      const mojom::ECashKeyId& key_id,
      base::span<const uint8_t, 32> message);

  mojom::KeyringId keyring_id() const { return keyring_id_; }
  bool IsTestnet() const;

 private:
  std::string GetAddressInternal(const HDKey& hd_key) const override;
  std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKey> DeriveKey(const mojom::ECashKeyId& key_id);

  mojom::KeyringId keyring_id_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ECASH_ECASH_KEYRING_H_
