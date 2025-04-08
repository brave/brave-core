/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

namespace {

std::unique_ptr<HDKeyEd25519Slip23> ConstructAccountsRootKey(
    base::span<const uint8_t> entropy) {
  auto result = HDKeyEd25519Slip23::GenerateMasterKeyFromBip39Entropy(entropy);
  if (!result) {
    return nullptr;
  }

  // https://cips.cardano.org/cip/CIP-1852#specification
  // m/1852'/1815'
  return result->DeriveChildFromPath({DerivationIndex::Hardened(1852),  //
                                      DerivationIndex::Hardened(1815)});
}

mojom::CardanoKeyId CardanoDefaultDelegationKeyId() {
  // Using same recommended value of `0` for all generated stake addresses.
  // https://cips.cardano.org/cip/CIP-0011#address_index-value
  return mojom::CardanoKeyId(mojom::CardanoKeyRole::kStaking, 0);
}

}  // namespace

CardanoHDKeyring::CardanoHDKeyring(base::span<const uint8_t> entropy,
                                   mojom::KeyringId keyring_id)
    : accounts_root_(ConstructAccountsRootKey(entropy)),
      keyring_id_(keyring_id) {
  CHECK(IsCardanoHDKeyring(keyring_id_));
}

CardanoHDKeyring::~CardanoHDKeyring() = default;

mojom::CardanoAddressPtr CardanoHDKeyring::GetAddress(
    uint32_t account,
    const mojom::CardanoKeyId& payment_key_id) {
  auto payment_hd_key = DeriveKey(account, payment_key_id);
  if (!payment_hd_key) {
    return nullptr;
  }

  auto delegation_hd_key = DeriveKey(account, CardanoDefaultDelegationKeyId());
  if (!delegation_hd_key) {
    return nullptr;
  }

  return mojom::CardanoAddress::New(
      CardanoAddress::FromParts(IsTestnet(),
                                Blake2bHash<kPaymentKeyHashLength>(
                                    payment_hd_key->GetPublicKeyAsSpan()),
                                Blake2bHash<kStakeKeyHashLength>(
                                    delegation_hd_key->GetPublicKeyAsSpan()))
          .ToString(),
      payment_key_id.Clone());
}

std::optional<std::string> CardanoHDKeyring::AddNewHDAccount(uint32_t index) {
  if (!accounts_root_) {
    return std::nullopt;
  }

  auto new_account = DeriveAccount(index);
  if (!new_account) {
    return std::nullopt;
  }
  return "";
}

std::optional<std::array<uint8_t, kCardanoSignatureSize>>
CardanoHDKeyring::SignMessage(uint32_t account,
                              const mojom::CardanoKeyId& key_id,
                              base::span<const uint8_t> message) {
  auto hd_key = DeriveKey(account, key_id);
  if (!hd_key) {
    return std::nullopt;
  }
  auto signature = hd_key->Sign(message);
  if (!signature) {
    return std::nullopt;
  }

  std::array<uint8_t, kCardanoSignatureSize> result;
  base::span(result).first<kEd25519PublicKeySize>().copy_from_nonoverlapping(
      hd_key->GetPublicKeyAsSpan());
  base::span(result).last<kEd25519SignatureSize>().copy_from_nonoverlapping(
      *signature);

  return result;
}

bool CardanoHDKeyring::IsTestnet() const {
  return IsCardanoTestnetKeyring(keyring_id_);
}

std::unique_ptr<HDKeyEd25519Slip23> CardanoHDKeyring::DeriveAccount(
    uint32_t index) const {
  return accounts_root_->DeriveChild(DerivationIndex::Hardened(index));
}

std::unique_ptr<HDKeyEd25519Slip23> CardanoHDKeyring::DeriveKey(
    uint32_t account,
    const mojom::CardanoKeyId& key_id) {
  auto account_key = DeriveAccount(account);
  if (!account_key) {
    return nullptr;
  }

  return account_key->DeriveChildFromPath(
      std::array{DerivationIndex::Normal(static_cast<uint32_t>(key_id.role)),
                 DerivationIndex::Normal(key_id.index)});
}

}  // namespace brave_wallet
