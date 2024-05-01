/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"

#include <memory>
#include <optional>

#include "base/check.h"

namespace brave_wallet {

ZCashKeyring::ZCashKeyring(base::span<const uint8_t> seed, bool testnet)
    : Secp256k1HDKeyring(
          seed,
          GetRootPath(testnet ? mojom::KeyringId::kZCashTestnet
                              : mojom::KeyringId::kZCashMainnet)),
      testnet_(testnet) {}

mojom::ZCashAddressPtr ZCashKeyring::GetAddress(
    const mojom::ZCashKeyId& key_id) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return nullptr;
  }

  return mojom::ZCashAddress::New(hd_key->GetZCashTransparentAddress(testnet_),
                                  key_id.Clone());
}

std::optional<std::vector<uint8_t>> ZCashKeyring::GetPubkey(
    const mojom::ZCashKeyId& key_id) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->GetPublicKeyBytes();
}

std::string ZCashKeyring::GetAddressInternal(const HDKey& hd_key) const {
  return hd_key.GetZCashTransparentAddress(testnet_);
}

std::unique_ptr<HDKey> ZCashKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet - m/44'/133'/{index}'
  // Testnet - m/44'/1'/{index}'
  return root_->DeriveHardenedChild(index);
}

std::unique_ptr<HDKey> ZCashKeyring::DeriveKey(
    const mojom::ZCashKeyId& key_id) {
  auto account_key = DeriveAccount(key_id.account);
  if (!account_key) {
    return nullptr;
  }

  DCHECK(key_id.change == 0 || key_id.change == 1);

  auto key = account_key->DeriveNormalChild(key_id.change);
  if (!key) {
    return nullptr;
  }

  // Mainnet - m/44'/133'/{address.account}'/{address.change}/{address.index}
  // Testnet - m/44'/1'/{address.account}'/{address.change}/{address.index}
  return key->DeriveNormalChild(key_id.index);
}

std::optional<std::vector<uint8_t>> ZCashKeyring::SignMessage(
    const mojom::ZCashKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->SignDer(message);
}

std::string ZCashKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  NOTIMPLEMENTED();
  return "";
}

}  // namespace brave_wallet
