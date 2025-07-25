/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ecash/ecash_keyring.h"

#include <array>
#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/cashaddr.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

namespace {

std::unique_ptr<HDKey> ConstructAccountsRootKey(base::span<const uint8_t> seed,
                                                bool testnet) {
  auto result = HDKey::GenerateFromSeed(seed);
  if (!result) {
    return nullptr;
  }

  if (testnet) {
    // Testnet: m/44'/1'
    return result->DeriveChildFromPath({DerivationIndex::Hardened(44),  //
                                        DerivationIndex::Hardened(1)});
  } else {
    // Mainnet: m/44'/899'
    // https://github.com/satoshilabs/slips/blob/master/slip-0044.md
    return result->DeriveChildFromPath({DerivationIndex::Hardened(44),  //
                                        DerivationIndex::Hardened(899)});
  }
}
}  // namespace

ECashKeyring::ECashKeyring(base::span<const uint8_t> seed,
                           mojom::KeyringId keyring_id)
    : keyring_id_(keyring_id) {
  CHECK(IsECashKeyring(keyring_id));
  accounts_root_ = ConstructAccountsRootKey(seed, IsTestnet());
}

mojom::ECashAddressPtr ECashKeyring::GetAddress(
    const mojom::ECashKeyId& key_id) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return nullptr;
  }

  return mojom::ECashAddress::New(GetAddressInternal(*hd_key), key_id.Clone());
}

std::optional<std::vector<uint8_t>> ECashKeyring::GetPubkey(
    const mojom::ECashKeyId& key_id) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->GetPublicKeyBytes();
}

std::string ECashKeyring::GetAddressInternal(const HDKey& hd_key) const {
  auto chain_type =
      IsTestnet() ? cashaddr::ChainType::TEST : cashaddr::ChainType::MAIN;
  auto hash = Hash160(hd_key.GetPublicKeyBytes());
  return cashaddr::EncodeCashAddress(
      cashaddr::PrefixFromChainType(chain_type).value(),
      cashaddr::AddressContent{
          cashaddr::AddressType::PUBKEY,
          {hash.begin(), hash.end()},
          chain_type,
      });
}

std::optional<std::vector<uint8_t>> ECashKeyring::SignMessage(
    const mojom::ECashKeyId& key_id,
    base::span<const uint8_t, 32> message) {
  auto hd_key = DeriveKey(key_id);
  if (!hd_key) {
    return std::nullopt;
  }

  return hd_key->SignDer(message);
}

std::unique_ptr<HDKey> ECashKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet - m/44'/899'/{index}'
  // Testnet - m/44'/1'/{index}'
  return accounts_root_->DeriveChild(DerivationIndex::Hardened(index));
}

std::unique_ptr<HDKey> ECashKeyring::DeriveKey(
    const mojom::ECashKeyId& key_id) {
  auto account_key = DeriveAccount(key_id.account);
  if (!account_key) {
    return nullptr;
  }

  DCHECK(key_id.change == 0 || key_id.change == 1);

  // Mainnet - m/44'/899'/{address.account}'/{address.change}/{address.index}
  // Testnet - m/44'/1'/{address.account}'/{address.change}/{address.index}
  return account_key->DeriveChildFromPath(
      std::array{DerivationIndex::Normal(key_id.change),
                 DerivationIndex::Normal(key_id.index)});
}

bool ECashKeyring::IsTestnet() const {
  return IsECashTestnetKeyring(keyring_id_);
}

}  // namespace brave_wallet
