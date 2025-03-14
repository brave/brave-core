/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_HD_KEYRING_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_base_keyring.h"
#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BitcoinHDKeyring : public BitcoinBaseKeyring, public Secp256k1HDKeyring {
 public:
  BitcoinHDKeyring(base::span<const uint8_t> seed, mojom::KeyringId keyring_id);
  ~BitcoinHDKeyring() override = default;
  BitcoinHDKeyring(const BitcoinHDKeyring&) = delete;
  BitcoinHDKeyring& operator=(const BitcoinHDKeyring&) = delete;

  // BitcoinBaseKeyring:
  mojom::BitcoinAddressPtr GetAddress(
      uint32_t account,
      const mojom::BitcoinKeyId& key_id) override;
  std::optional<std::vector<uint8_t>> GetPubkey(
      uint32_t account,
      const mojom::BitcoinKeyId& key_id) override;
  std::optional<std::vector<uint8_t>> SignMessage(
      uint32_t account,
      const mojom::BitcoinKeyId& key_id,
      base::span<const uint8_t, 32> message) override;

 private:
  std::string GetAddressInternal(const HDKey& hd_key) const override;
  std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKey> DeriveKey(uint32_t account,
                                   const mojom::BitcoinKeyId& key_id);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_HD_KEYRING_H_
