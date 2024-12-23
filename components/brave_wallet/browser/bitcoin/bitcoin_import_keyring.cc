/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_import_keyring.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"

namespace brave_wallet {

BitcoinImportKeyring::BitcoinImportKeyring(bool testnet) : testnet_(testnet) {}

BitcoinImportKeyring::~BitcoinImportKeyring() = default;

bool BitcoinImportKeyring::AddAccount(uint32_t account,
                                      const std::string& payload) {
  if (accounts_.contains(account)) {
    return false;
  }
  auto parsed_key = HDKey::GenerateFromExtendedKey(payload);
  if (!parsed_key) {
    return false;
  }

  if (testnet_ && parsed_key->version != ExtendedKeyVersion::kVprv) {
    return false;
  }
  if (!testnet_ && parsed_key->version != ExtendedKeyVersion::kZprv) {
    return false;
  }

  accounts_[account] = std::move(parsed_key->hdkey);
  return true;
}

bool BitcoinImportKeyring::RemoveAccount(uint32_t account) {
  return accounts_.erase(account) > 0;
}

mojom::BitcoinAddressPtr BitcoinImportKeyring::GetAddress(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return nullptr;
  }

  return mojom::BitcoinAddress::New(
      PubkeyToSegwitAddress(hd_key->GetPublicKeyBytes(), testnet_),
      key_id.Clone());
}

std::optional<std::vector<uint8_t>> BitcoinImportKeyring::GetPubkey(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->GetPublicKeyBytes();
}

std::optional<std::vector<uint8_t>> BitcoinImportKeyring::SignMessage(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->SignDer(message);
}

HDKey* BitcoinImportKeyring::GetAccountByIndex(uint32_t account) {
  if (!accounts_.contains(account)) {
    return nullptr;
  }
  return accounts_[account].get();
}

std::unique_ptr<HDKey> BitcoinImportKeyring::DeriveKey(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id) {
  // TODO(apaymyshev): keep local cache of keys: key_id->key
  auto* account_key = GetAccountByIndex(account);
  if (!account_key) {
    return nullptr;
  }

  DCHECK(key_id.change == 0 || key_id.change == 1);

  // Mainnet - m/84'/0'/{account}'/{key_id.change}/{key_id.index}
  // Testnet - m/84'/1'/{account}'/{key_id.change}/{key_id.index}
  return account_key->DeriveChildFromPath(
      std::array{DerivationIndex::Normal(key_id.change),
                 DerivationIndex::Normal(key_id.index)});
}

}  // namespace brave_wallet
