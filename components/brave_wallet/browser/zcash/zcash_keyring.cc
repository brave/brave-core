/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

ZCashKeyring::ZCashKeyring(bool testnet) : testnet_(testnet) {}

ZCashKeyring::~ZCashKeyring() = default;

mojom::ZCashAddressPtr ZCashKeyring::GetTransparentAddress(
    const mojom::ZCashKeyId& key_id) {
  auto key = DeriveKey(key_id);
  if (!key) {
    return nullptr;
  }

  HDKey* hd_key = static_cast<HDKey*>(key.get());

  return mojom::ZCashAddress::New(hd_key->GetZCashTransparentAddress(testnet_),
                                  key_id.Clone());
}

mojom::ZCashAddressPtr ZCashKeyring::GetShieldedAddress(
    const mojom::ZCashKeyId& key_id) {
  if (!orchard_key_) {
    NOTREACHED();
    return nullptr;
  }

  auto esk = orchard_key_->DeriveHardenedChild(key_id.account);
  if (!esk) {
    return nullptr;
  }

  auto addr_bytes =
      key_id.change
          ? esk->GetDiversifiedAddress(key_id.index, OrchardKind::Internal)
          : esk->GetDiversifiedAddress(key_id.index, OrchardKind::External);
  if (!addr_bytes) {
    return nullptr;
  }

  auto addr_str = GetOrchardUnifiedAddress(addr_bytes.value(), testnet_);
  if (!addr_str) {
    return nullptr;
  }
  return mojom::ZCashAddress::New(addr_str.value(), key_id.Clone());
}

std::optional<std::string> ZCashKeyring::GetUnifiedAddress(
    const mojom::ZCashKeyId& transparent_key_id,
    const mojom::ZCashKeyId& zcash_key_id) {
  if (!orchard_key_) {
    NOTREACHED();
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(zcash_key_id.account);
  if (!esk) {
    return std::nullopt;
  }

  auto orchard_addr_bytes =
      zcash_key_id.change ? esk->GetDiversifiedAddress(zcash_key_id.index,
                                                       OrchardKind::Internal)
                          : esk->GetDiversifiedAddress(zcash_key_id.index,
                                                       OrchardKind::External);
  if (!orchard_addr_bytes) {
    return std::nullopt;
  }

  auto transparent_pubkey_hash = GetPubkeyHash(transparent_key_id);
  if (!transparent_pubkey_hash) {
    return std::nullopt;
  }

  return GetMergedUnifiedAddress(
      std::vector<ParsedAddress>{
          ParsedAddress(ZCashAddrType::kP2PKH, transparent_pubkey_hash.value()),
          ParsedAddress(ZCashAddrType::kOrchard,
                        std::vector<uint8_t>(orchard_addr_bytes->begin(),
                                             orchard_addr_bytes->end()))},
      testnet_);
}

std::optional<std::vector<uint8_t>> ZCashKeyring::GetPubkey(
    const mojom::ZCashKeyId& key_id) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return std::nullopt;
  }

  return hd_key_base->GetPublicKeyBytes();
}

std::optional<std::vector<uint8_t>> ZCashKeyring::GetPubkeyHash(
    const mojom::ZCashKeyId& key_id) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return std::nullopt;
  }

  return Hash160(hd_key_base->GetPublicKeyBytes());
}

std::optional<std::array<uint8_t, kOrchardRawBytesSize>>
ZCashKeyring::GetOrchardRawBytes(const mojom::ZCashKeyId& key_id) {
  if (!orchard_key_) {
    NOTREACHED();
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(key_id.account);
  if (!esk) {
    return std::nullopt;
  }

  auto orchard_addr_bytes =
      key_id.change
          ? esk->GetDiversifiedAddress(key_id.index, OrchardKind::Internal)
          : esk->GetDiversifiedAddress(key_id.index, OrchardKind::External);
  return orchard_addr_bytes;
}

std::string ZCashKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base) {
    return std::string();
  }
  HDKey* hd_key = static_cast<HDKey*>(hd_key_base);
  return hd_key->GetZCashTransparentAddress(testnet_);
}

std::unique_ptr<HDKeyBase> ZCashKeyring::DeriveAccount(uint32_t index) const {
  // Mainnet - m/44'/133'/{index}'
  // Testnet - m/44'/1'/{index}'
  return root_->DeriveHardenedChild(index);
}

std::unique_ptr<HDKeyBase> ZCashKeyring::DeriveKey(
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
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return std::nullopt;
  }

  auto* hd_key = static_cast<HDKey*>(hd_key_base.get());

  return hd_key->SignDer(message);
}

void ZCashKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                      const std::string& hd_path) {
  HDKeyring::ConstructRootHDKey(seed, hd_path);

  if (!seed.empty() && IsZCashShieldedEnabled()) {
    auto orchard_key = HDKeyZip32::GenerateFromSeed(seed);
    if (!orchard_key) {
      NOTREACHED();
      return;
    }
    orchard_key = orchard_key->DeriveHardenedChild(kZip32Purpose);
    if (!orchard_key) {
      NOTREACHED();
      return;
    }
    orchard_key_ = orchard_key->DeriveHardenedChild(
        testnet_ ? kTestnetCoinType
                 : static_cast<uint32_t>(mojom::CoinType::ZEC));
    DCHECK(orchard_key_);
  }
}

}  // namespace brave_wallet
