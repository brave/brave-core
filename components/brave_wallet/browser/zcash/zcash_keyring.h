/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_KEYRING_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ZCashKeyring : public Secp256k1HDKeyring {
 public:
  ZCashKeyring(base::span<const uint8_t> seed, bool testnet);
  ~ZCashKeyring() override = default;
  ZCashKeyring(const ZCashKeyring&) = delete;
  ZCashKeyring& operator=(const ZCashKeyring&) = delete;

  mojom::ZCashAddressPtr GetAddress(const mojom::ZCashKeyId& key_id);

  std::optional<std::vector<uint8_t>> GetPubkey(
      const mojom::ZCashKeyId& key_id);

  std::optional<std::vector<uint8_t>> SignMessage(
      const mojom::ZCashKeyId& key_id,
      base::span<const uint8_t, 32> message);

  std::string EncodePrivateKeyForExport(const std::string& address) override;

 private:
  std::string GetAddressInternal(const HDKey& hd_key) const override;
  std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKey> DeriveKey(const mojom::ZCashKeyId& key_id);

  bool testnet_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_KEYRING_H_
