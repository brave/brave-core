/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

namespace brave_wallet {

PolkadotChainMetadata::PolkadotChainMetadata() = default;

PolkadotChainMetadata::PolkadotChainMetadata(
    const PolkadotChainMetadata& other) = default;

PolkadotChainMetadata::PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept =
    default;

PolkadotChainMetadata::~PolkadotChainMetadata() = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    const PolkadotChainMetadata&) = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    PolkadotChainMetadata&&) noexcept = default;

bool PolkadotChainMetadata::operator==(
    const PolkadotChainMetadata& other) const = default;

std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromBytes(
    base::span<const uint8_t> metadata_bytes) {
  auto result = parse_chain_metadata_from_scale(
      ::rust::Slice<const uint8_t>(metadata_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto parsed = result->unwrap();
  return PolkadotChainMetadata(*parsed);
}

// static
PolkadotChainMetadata PolkadotChainMetadata::FromFields(
    uint8_t system_pallet_index,
    uint8_t balances_pallet_index,
    uint8_t transaction_payment_pallet_index,
    uint8_t transfer_allow_death_call_index,
    uint8_t transfer_keep_alive_call_index,
    uint8_t transfer_all_call_index,
    uint16_t ss58_prefix,
    uint32_t spec_version,
    bool asset_tx_payment,
    bool has_assets_pallet,
    uint8_t assets_pallet_index,
    uint8_t assets_transfer_all_call_index,
    uint8_t assets_transfer_keep_alive_call_index) {
  PolkadotChainMetadata metadata;
  metadata->system_pallet_index = system_pallet_index;
  metadata->balances_pallet_index = balances_pallet_index;
  metadata->transaction_payment_pallet_index = transaction_payment_pallet_index;
  metadata->transfer_allow_death_call_index = transfer_allow_death_call_index;
  metadata->transfer_keep_alive_call_index = transfer_keep_alive_call_index;
  metadata->transfer_all_call_index = transfer_all_call_index;
  metadata->assets_pallet_index = assets_pallet_index;
  metadata->assets_transfer_all_call_index = assets_transfer_all_call_index;
  metadata->assets_transfer_keep_alive_call_index =
      assets_transfer_keep_alive_call_index;
  metadata->has_assets_pallet = has_assets_pallet;
  metadata->ss58_prefix = ss58_prefix;
  metadata->spec_version = spec_version;
  metadata->asset_tx_payment = asset_tx_payment;

  return metadata;
}

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
  return chain_metadata_;
}

CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() {
  return chain_metadata_;
}

CxxPolkadotChainMetadata* PolkadotChainMetadata::operator->() {
  return &chain_metadata_;
}

const CxxPolkadotChainMetadata* PolkadotChainMetadata::operator->() const {
  return &chain_metadata_;
}

PolkadotChainMetadata::PolkadotChainMetadata(
    CxxPolkadotChainMetadata chain_metadata)
    : chain_metadata_(chain_metadata) {}

}  // namespace brave_wallet
