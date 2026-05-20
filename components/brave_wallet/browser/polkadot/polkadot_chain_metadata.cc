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
