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
#include "brave/components/brave_wallet/common/buildflags.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/internal/hd_key_zip32.h"
#endif

namespace brave_wallet {

class ZCashKeyring : public Secp256k1HDKeyring {
 public:
  ZCashKeyring(base::span<const uint8_t> seed, mojom::KeyringId keyring_id);
  ~ZCashKeyring() override;

  ZCashKeyring(const ZCashKeyring&) = delete;
  ZCashKeyring& operator=(const ZCashKeyring&) = delete;

  mojom::ZCashAddressPtr GetTransparentAddress(const mojom::ZCashKeyId& key_id);
  std::optional<std::vector<uint8_t>> GetPubkey(
      const mojom::ZCashKeyId& key_id);
  std::optional<std::vector<uint8_t>> GetPubkeyHash(
      const mojom::ZCashKeyId& key_id);

// TODO(cypt4): move Orchard to the separate keyring
#if BUILDFLAG(ENABLE_ORCHARD)
  std::unique_ptr<HDKeyZip32> DeriveOrchardAccount(uint32_t index) const;
  std::optional<std::string> GetUnifiedAddress(
      const mojom::ZCashKeyId& transparent_key_id,
      const mojom::ZCashKeyId& orchard_key_id);
  mojom::ZCashAddressPtr GetShieldedAddress(const mojom::ZCashKeyId& key_id);
  std::optional<OrchardAddrRawPart> GetOrchardRawBytes(
      const mojom::ZCashKeyId& key_id);
  std::optional<OrchardFullViewKey> GetOrchardFullViewKey(
      const uint32_t& account_id);
  std::optional<OrchardSpendingKey> GetOrchardSpendingKey(
      const uint32_t& account_id);
#endif

  std::optional<std::vector<uint8_t>> SignMessage(
      const mojom::ZCashKeyId& key_id,
      base::span<const uint8_t, 32> message);

  mojom::KeyringId keyring_id() const { return keyring_id_; }
  bool IsTestnet() const;

 private:
  std::string GetAddressInternal(const HDKey& hd_key) const override;
  std::unique_ptr<HDKey> DeriveAccount(uint32_t index) const override;
  std::unique_ptr<HDKey> DeriveKey(const mojom::ZCashKeyId& key_id);

#if BUILDFLAG(ENABLE_ORCHARD)
  std::unique_ptr<HDKeyZip32> orchard_accounts_root_;
#endif

  mojom::KeyringId keyring_id_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_KEYRING_H_
