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
    const PolkadotChainMetadata& other) const {
  return chain_metadata_ == other.chain_metadata_;
}

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

// spec_version is unknown when constructing from chain name alone; callers
// must update it from state_getRuntimeVersion before use.
constexpr uint32_t kUnknownSpecVersion = 0;

// static
std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromChainName(
    std::string_view chain_name) {
  PolkadotChainMetadata metadata;
  metadata->system_pallet_index = 0;
  metadata->transfer_allow_death_call_index = 0;
  metadata->transfer_keep_alive_call_index = 3;
  metadata->transfer_all_call_index = 4;
  metadata->spec_version = kUnknownSpecVersion;

  // https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
  if (chain_name == "Westend") {
    metadata->balances_pallet_index = 4;
    metadata->transaction_payment_pallet_index = 0x1a;
    metadata->ss58_prefix = 42;
    metadata->asset_tx_payment = false;
    return metadata;
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
  if (chain_name == "Westend Asset Hub") {
    metadata->balances_pallet_index = 10;
    metadata->transaction_payment_pallet_index = 0x0b;
    metadata->ss58_prefix = 42;
    metadata->asset_tx_payment = true;
    return metadata;
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
  if (chain_name == "Polkadot") {
    metadata->balances_pallet_index = 5;
    metadata->transaction_payment_pallet_index = 0x20;
    metadata->ss58_prefix = 0;
    metadata->asset_tx_payment = false;
    return metadata;
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
  if (chain_name == "Polkadot Asset Hub") {
    metadata->balances_pallet_index = 10;
    metadata->transaction_payment_pallet_index = 0x0b;
    metadata->ss58_prefix = 0;
    metadata->asset_tx_payment = true;
    return metadata;
  }

  return std::nullopt;
}

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
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
