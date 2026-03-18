/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include "brave/components/brave_wallet/browser/internal/polkadot_chain_metadata.rs.h"

namespace brave_wallet {

PolkadotChainMetadata::PolkadotChainMetadata(const PolkadotChainMetadata& other)
    : chain_metadata_(other.chain_metadata_) {}

PolkadotChainMetadata::PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept =
    default;

PolkadotChainMetadata::~PolkadotChainMetadata() = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    const PolkadotChainMetadata&) = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    PolkadotChainMetadata&&) noexcept = default;

std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromBytes(
    base::span<const uint8_t> metadata_bytes) {
  auto result = parse_chain_metadata_from_scale(
      ::rust::Slice<const uint8_t>(metadata_bytes));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto parsed = result->unwrap();
  return FromFields(parsed->balances_pallet_index(),
                    parsed->transfer_allow_death_call_index(),
                    parsed->ss58_prefix(), parsed->spec_version());
}

// static
PolkadotChainMetadata PolkadotChainMetadata::FromFields(
    uint8_t balances_pallet_index,
    uint8_t transfer_allow_death_call_index,
    uint16_t ss58_prefix,
    uint32_t spec_version) {
  CxxPolkadotChainMetadata metadata;
  metadata.balances_pallet_index = balances_pallet_index;
  metadata.transfer_allow_death_call_index = transfer_allow_death_call_index;
  metadata.ss58_prefix = ss58_prefix;
  metadata.spec_version = spec_version;
  return PolkadotChainMetadata(metadata);
}

// spec_version is unknown when constructing from chain name alone; callers
// must update it from state_getRuntimeVersion before use.
constexpr uint32_t kUnknownSpecVersion = 0;

// static
std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromChainName(
    std::string_view chain_name) {
  // https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
  if (chain_name == "Westend") {
    return FromFields(/*balances_pallet_index=*/4,
                      /*transfer_allow_death_call_index=*/0,
                      /*ss58_prefix=*/42, kUnknownSpecVersion);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
  if (chain_name == "Westend Asset Hub") {
    return FromFields(/*balances_pallet_index=*/10,
                      /*transfer_allow_death_call_index=*/0,
                      /*ss58_prefix=*/42, kUnknownSpecVersion);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
  if (chain_name == "Polkadot") {
    return FromFields(/*balances_pallet_index=*/5,
                      /*transfer_allow_death_call_index=*/0,
                      /*ss58_prefix=*/0, kUnknownSpecVersion);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
  if (chain_name == "Polkadot Asset Hub") {
    return FromFields(/*balances_pallet_index=*/10,
                      /*transfer_allow_death_call_index=*/0,
                      /*ss58_prefix=*/0, kUnknownSpecVersion);
  }

  return std::nullopt;
}

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
  return chain_metadata_;
}

uint8_t PolkadotChainMetadata::GetBalancesPalletIndex() const {
  return chain_metadata_.balances_pallet_index;
}

uint8_t PolkadotChainMetadata::GetTransferAllowDeathCallIndex() const {
  return chain_metadata_.transfer_allow_death_call_index;
}

uint16_t PolkadotChainMetadata::GetSs58Prefix() const {
  return chain_metadata_.ss58_prefix;
}

uint32_t PolkadotChainMetadata::GetSpecVersion() const {
  return chain_metadata_.spec_version;
}

PolkadotChainMetadata::PolkadotChainMetadata(
    CxxPolkadotChainMetadata chain_metadata)
    : chain_metadata_(chain_metadata) {}

}  // namespace brave_wallet
