/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

namespace {

// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
constexpr uint32_t kPaymentKeyHashLength = 28;
constexpr uint32_t kStakeKeyHashLength = 28;
constexpr char kMainnetHrp[] = "addr";
constexpr char kTestnetHrp[] = "addr_test";

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

// https://cips.cardano.org/cip/CIP-0019#user-facing-encoding
std::string EncodeCardanoAddress(
    bool testnet,
    base::span<const uint8_t, kPaymentKeyHashLength> payment_part,
    base::span<const uint8_t, kStakeKeyHashLength> delegation_part) {
  // https://cips.cardano.org/cip/CIP-0019#shelley-addresses
  const uint8_t shelly_type = 0;  // PaymentKeyHash | StakeKeyHash
  const uint8_t network_tag = testnet ? 0 : 1;

  const uint8_t header = (shelly_type << 4) | network_tag;

  std::array<uint8_t, 1 + kPaymentKeyHashLength + kStakeKeyHashLength>
      address_bytes;
  auto span_writer = base::SpanWriter(base::span(address_bytes));
  span_writer.Write(header);
  span_writer.Write(payment_part);
  span_writer.Write(delegation_part);
  DCHECK_EQ(span_writer.remaining(), 0u);

  return bech32::Encode(address_bytes, testnet ? kTestnetHrp : kMainnetHrp,
                        bech32::Encoding::kBech32);
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

  // Using same recommended value of `0` for all generated stake addresses.
  // https://cips.cardano.org/cip/CIP-0011#address_index-value
  auto delegation_key_id =
      mojom::CardanoKeyId(mojom::CardanoKeyRole::kStaking, 0);
  auto delegation_hd_key = DeriveKey(account, delegation_key_id);
  if (!delegation_hd_key) {
    return nullptr;
  }

  return mojom::CardanoAddress::New(
      EncodeCardanoAddress(IsTestnet(),
                           Blake2bHash<kPaymentKeyHashLength>(
                               payment_hd_key->GetPublicKeyAsSpan()),
                           Blake2bHash<kStakeKeyHashLength>(
                               delegation_hd_key->GetPublicKeyAsSpan())),
      payment_key_id.Clone(), delegation_key_id.Clone());
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
