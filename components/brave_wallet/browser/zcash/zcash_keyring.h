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

#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_zip32.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ZCashKeyring : public HDKeyring {
 public:
  explicit ZCashKeyring(bool testnet);
  ~ZCashKeyring() override;
  ZCashKeyring(const ZCashKeyring&) = delete;
  ZCashKeyring& operator=(const ZCashKeyring&) = delete;

  mojom::ZCashAddressPtr GetTransparentAddress(const mojom::ZCashKeyId& key_id);
  mojom::ZCashAddressPtr GetShieldedAddress(const mojom::ZCashKeyId& key_id);
  std::optional<std::string> GetUnifiedAddress(
      const mojom::ZCashKeyId& transparent_key_id,
      const mojom::ZCashKeyId& zcash_key_id);

  std::optional<std::vector<uint8_t>> GetPubkey(
      const mojom::ZCashKeyId& key_id);

  std::optional<std::vector<uint8_t>> GetPubkeyHash(
      const mojom::ZCashKeyId& key_id);

  std::optional<std::vector<uint8_t>> GetOrchardRawBytes(
      const mojom::ZCashKeyId& key_id);

  std::optional<std::vector<uint8_t>> SignMessage(
      const mojom::ZCashKeyId& key_id,
      base::span<const uint8_t, 32> message);

  void ConstructRootHDKey(const std::vector<uint8_t>& seed,
                          const std::string& hd_path) override;

 private:
  std::string GetAddressInternal(HDKeyBase* hd_key_base) const override;
  std::unique_ptr<HDKeyBase> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKeyBase> DeriveKey(const mojom::ZCashKeyId& key_id);

  std::unique_ptr<HDKeyZip32> orchard_key_;

  bool testnet_ = false;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_KEYRING_H_
