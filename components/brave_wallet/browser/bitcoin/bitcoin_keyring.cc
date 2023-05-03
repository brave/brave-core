/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_keyring.h"

#include <memory>
#include <utility>

#include "base/check.h"

namespace brave_wallet {

BitcoinKeyring::BitcoinKeyring(bool testnet) : testnet_(testnet) {}

absl::optional<std::string> BitcoinKeyring::GetAddress(
    const mojom::BitcoinKeyId& key_id) {
  auto key = DeriveKey(key_id);
  if (!key) {
    return absl::nullopt;
  }

  HDKey* hd_key = static_cast<HDKey*>(key.get());

  return hd_key->GetSegwitAddress(testnet_);
}

absl::optional<std::vector<uint8_t>> BitcoinKeyring::GetPubkey(
    const mojom::BitcoinKeyId& key_id) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return absl::nullopt;
  }

  return hd_key_base->GetPublicKeyBytes();
}

absl::optional<std::vector<uint8_t>> BitcoinKeyring::SignMessage(
    const mojom::BitcoinKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return absl::nullopt;
  }

  auto* hd_key = static_cast<HDKey*>(hd_key_base.get());

  return hd_key->SignDer(message);
}

std::string BitcoinKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base) {
    return std::string();
  }
  HDKey* hd_key = static_cast<HDKey*>(hd_key_base);
  return hd_key->GetSegwitAddress(testnet_);
}

std::unique_ptr<HDKeyBase> BitcoinKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet - m/84'/0'/{index}'
  // Testnet - m/84'/1'/{index}'
  return root_->DeriveHardenedChild(index);
}

std::unique_ptr<HDKeyBase> BitcoinKeyring::DeriveKey(
    const mojom::BitcoinKeyId& key_id) {
  // TODO(apaymyshev): keep local cache of keys: key_id->key
  auto account_key = DeriveAccount(key_id.account);
  if (!account_key) {
    return nullptr;
  }

  // TODO(apaymyshev): think if |key_id.change| should be a boolean.
  DCHECK(key_id.change == 0 || key_id.change == 1);

  auto key = account_key->DeriveNormalChild(key_id.change);
  if (!key) {
    return nullptr;
  }

  // Mainnet - m/84'/0'/{address.account}'/{address.change}/{address.index}
  // Testnet - m/84'/1'/{address.account}'/{address.change}/{address.index}
  return key->DeriveNormalChild(key_id.index);
}

}  // namespace brave_wallet
