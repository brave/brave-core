/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include "brave/components/brave_wallet/browser/internal/polkadot_chain_metadata.rs.h"

namespace brave_wallet {

namespace {

::rust::Str StringViewToRustStr(std::string_view sv) {
  return ::rust::Str(sv.data(), sv.size());
}

}  // namespace

PolkadotChainMetadata::PolkadotChainMetadata(const PolkadotChainMetadata& other)
    : chain_metadata_(other.chain_metadata_) {}

PolkadotChainMetadata::PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept =
    default;

PolkadotChainMetadata::~PolkadotChainMetadata() = default;

PolkadotChainMetadata& PolkadotChainMetadata::operator=(
    PolkadotChainMetadata&&) noexcept = default;

std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromMetadataHex(
    std::string_view metadata_hex) {
  auto result =
      parse_chain_metadata_fields_from_hex(StringViewToRustStr(metadata_hex));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto parsed = result->unwrap();
  auto metadata = FromFields(parsed->balances_pallet_index(),
                             parsed->transfer_allow_death_call_index(),
                             parsed->ss58_prefix(), parsed->spec_version());
  if (!metadata) {
    return std::nullopt;
  }

  return metadata;
}

// static
std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromFields(
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

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
  return chain_metadata_;
}

PolkadotChainMetadata::PolkadotChainMetadata(
    CxxPolkadotChainMetadata chain_metadata)
    : chain_metadata_(chain_metadata) {}

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

}  // namespace brave_wallet
