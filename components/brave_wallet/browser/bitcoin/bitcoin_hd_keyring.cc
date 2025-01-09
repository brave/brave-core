/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"

namespace brave_wallet {

namespace {

std::unique_ptr<HDKey> ConstructAccountsRootKey(base::span<const uint8_t> seed,
                                                bool testnet) {
  auto result = HDKey::GenerateFromSeed(seed);
  if (!result) {
    return nullptr;
  }

  if (testnet) {
    // Testnet: m/84'/1'
    return result->DeriveChildFromPath({DerivationIndex::Hardened(84),  //
                                        DerivationIndex::Hardened(1)});
  } else {
    // Mainnet: m/84'/0'
    return result->DeriveChildFromPath({DerivationIndex::Hardened(84),  //
                                        DerivationIndex::Hardened(0)});
  }
}

}  // namespace

BitcoinHDKeyring::BitcoinHDKeyring(base::span<const uint8_t> seed, bool testnet)
    : testnet_(testnet) {
  accounts_root_ = ConstructAccountsRootKey(seed, testnet);
}

mojom::BitcoinAddressPtr BitcoinHDKeyring::GetAddress(
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

std::optional<std::vector<uint8_t>> BitcoinHDKeyring::GetPubkey(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->GetPublicKeyBytes();
}

std::optional<std::vector<uint8_t>> BitcoinHDKeyring::SignMessage(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->SignDer(message);
}

std::string BitcoinHDKeyring::ImportAccount(
    base::span<const uint8_t> private_key) {
  NOTIMPLEMENTED();
  return "";
}

bool BitcoinHDKeyring::RemoveImportedAccount(const std::string& address) {
  NOTIMPLEMENTED();
  return false;
}

std::string BitcoinHDKeyring::GetDiscoveryAddress(size_t index) const {
  NOTIMPLEMENTED();
  return "";
}

std::vector<std::string> BitcoinHDKeyring::GetImportedAccountsForTesting()
    const {
  NOTIMPLEMENTED();
  return {};
}

std::string BitcoinHDKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  NOTIMPLEMENTED();
  return "";
}

std::string BitcoinHDKeyring::GetAddressInternal(const HDKey& hd_key) const {
  return PubkeyToSegwitAddress(hd_key.GetPublicKeyBytes(), testnet_);
}

std::unique_ptr<HDKey> BitcoinHDKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet - m/84'/0'/{index}'
  // Testnet - m/84'/1'/{index}'
  return accounts_root_->DeriveChild(DerivationIndex::Hardened(index));
}

std::unique_ptr<HDKey> BitcoinHDKeyring::DeriveKey(
    uint32_t account,
    const mojom::BitcoinKeyId& key_id) {
  // TODO(apaymyshev): keep local cache of keys: key_id->key
  auto account_key = DeriveAccount(account);
  if (!account_key) {
    return nullptr;
  }

  // TODO(apaymyshev): think if |key_id.change| should be a boolean.
  DCHECK(key_id.change == 0 || key_id.change == 1);

  // Mainnet - m/84'/0'/{account}'/{key_id.change}/{key_id.index}
  // Testnet - m/84'/1'/{account}'/{key_id.change}/{key_id.index}
  return account_key->DeriveChildFromPath(
      std::array{DerivationIndex::Normal(key_id.change),
                 DerivationIndex::Normal(key_id.index)});
}

}  // namespace brave_wallet
