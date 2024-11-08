/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"

#include <memory>
#include <optional>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

ZCashKeyring::ZCashKeyring(base::span<const uint8_t> seed, bool testnet)
    : Secp256k1HDKeyring(
          seed,
          GetRootPath(testnet ? mojom::KeyringId::kZCashTestnet
                              : mojom::KeyringId::kZCashMainnet)),
      testnet_(testnet) {
#if BUILDFLAG(ENABLE_ORCHARD)
  if (!seed.empty() && IsZCashShieldedTransactionsEnabled()) {
    auto orchard_key = HDKeyZip32::GenerateFromSeed(seed);
    if (!orchard_key) {
      return;
    }
    orchard_key = orchard_key->DeriveHardenedChild(kZip32Purpose);
    if (!orchard_key) {
      return;
    }
    orchard_key_ = orchard_key->DeriveHardenedChild(
        testnet_ ? kTestnetCoinType
                 : static_cast<uint32_t>(mojom::CoinType::ZEC));
  }
#endif
}

ZCashKeyring::~ZCashKeyring() = default;

mojom::ZCashAddressPtr ZCashKeyring::GetTransparentAddress(
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

std::optional<std::vector<uint8_t>> ZCashKeyring::GetPubkeyHash(
    const mojom::ZCashKeyId& key_id) {
  auto hd_key_base = DeriveKey(key_id);
  if (!hd_key_base) {
    return std::nullopt;
  }

  return base::ToVector(Hash160(hd_key_base->GetPublicKeyBytes()));
}

#if BUILDFLAG(ENABLE_ORCHARD)
std::optional<std::string> ZCashKeyring::GetUnifiedAddress(
    const mojom::ZCashKeyId& transparent_key_id,
    const mojom::ZCashKeyId& orchard_key_id) {
  if (!orchard_key_) {
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(orchard_key_id.account);
  if (!esk) {
    return std::nullopt;
  }

  auto orchard_addr_bytes =
      orchard_key_id.change
          ? esk->GetDiversifiedAddress(orchard_key_id.index,
                                       OrchardAddressKind::Internal)
          : esk->GetDiversifiedAddress(orchard_key_id.index,
                                       OrchardAddressKind::External);
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

mojom::ZCashAddressPtr ZCashKeyring::GetShieldedAddress(
    const mojom::ZCashKeyId& key_id) {
  if (!orchard_key_) {
    return nullptr;
  }

  auto esk = orchard_key_->DeriveHardenedChild(key_id.account);
  if (!esk) {
    return nullptr;
  }

  auto addr_bytes = key_id.change
                        ? esk->GetDiversifiedAddress(
                              key_id.index, OrchardAddressKind::Internal)
                        : esk->GetDiversifiedAddress(
                              key_id.index, OrchardAddressKind::External);
  if (!addr_bytes) {
    return nullptr;
  }

  auto addr_str = GetOrchardUnifiedAddress(addr_bytes.value(), testnet_);
  if (!addr_str) {
    return nullptr;
  }
  return mojom::ZCashAddress::New(addr_str.value(), key_id.Clone());
}

std::optional<OrchardAddrRawPart> ZCashKeyring::GetOrchardRawBytes(
    const mojom::ZCashKeyId& key_id) {
  if (!orchard_key_) {
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(key_id.account);
  if (!esk) {
    return std::nullopt;
  }

  auto orchard_addr_bytes =
      key_id.change ? esk->GetDiversifiedAddress(key_id.index,
                                                 OrchardAddressKind::Internal)
                    : esk->GetDiversifiedAddress(key_id.index,
                                                 OrchardAddressKind::External);
  return orchard_addr_bytes;
}

std::optional<OrchardFullViewKey> ZCashKeyring::GetOrchardFullViewKey(
    const uint32_t& account_id) {
  if (!orchard_key_) {
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(account_id);
  if (!esk) {
    return std::nullopt;
  }

  return esk->GetFullViewKey();
}

std::optional<OrchardSpendingKey> ZCashKeyring::GetOrchardSpendingKey(
    const uint32_t& account_id) {
  if (!orchard_key_) {
    return std::nullopt;
  }

  auto esk = orchard_key_->DeriveHardenedChild(account_id);
  if (!esk) {
    return std::nullopt;
  }

  return esk->GetSpendingKey();
}

#endif

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
