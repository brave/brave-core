/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_keyring.h"

#include <memory>
#include <utility>

namespace brave_wallet {

BitcoinKeyring::BitcoinKeyring(bool testnet) : testnet_(testnet) {}

std::string BitcoinKeyring::GetAddress(const mojom::BitcoinKeyId& key_id) {
  auto key = DeriveKey(key_id);
  if (!key) {
    return "";
  }

  HDKey* hd_key = static_cast<HDKey*>(key.get());

  return hd_key->GetSegwitAddress(testnet_);
}

std::vector<uint8_t> BitcoinKeyring::GetBitcoinPubkey(
    const mojom::BitcoinKeyId& key_id) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return {};
  }

  return hd_key_base->GetPublicKeyBytes();
}

std::vector<uint8_t> BitcoinKeyring::SignBitcoinMessage(
    const mojom::BitcoinKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return {};
  }

  auto* hd_key = static_cast<HDKey*>(hd_key_base.get());

  return hd_key->SignBitcoin(message);
}

std::string BitcoinKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base) {
    return std::string();
  }
  HDKey* hd_key = static_cast<HDKey*>(hd_key_base);
  return hd_key->GetSegwitAddress(testnet_);
}

std::unique_ptr<HDKeyBase> BitcoinKeyring::DeriveAccount(uint32_t index) const {
  // m/84'/0'/{index}'
  return root_->DeriveHardenedChild(index);
}

std::unique_ptr<HDKeyBase> BitcoinKeyring::DeriveKey(
    const mojom::BitcoinKeyId& key_id) {
  // TODO(apaymyshev): keep local cache of keys: key_id->key
  auto account_key = DeriveAccount(key_id.account);
  if (!account_key) {
    return nullptr;
  }

  auto key = account_key->DeriveNormalChild(key_id.change);
  if (!key) {
    return nullptr;
  }

  // m/84'/0'/{address.account}'/{address.change}/{address.index}
  return key->DeriveNormalChild(key_id.index);
}

}  // namespace brave_wallet
