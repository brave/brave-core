/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_H_

#include <stdint.h>

#include <optional>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"

namespace brave_wallet {

inline constexpr size_t kMaxSignedExtensions = 64;

// Runtime metadata is sourced from the hex string returned by
// state_getMetadata:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#1119-state_getmetadata
// https://spec.polkadot.network/sect-metadata
// We parse minimal fields needed for transfer extrinsic handling.
class PolkadotChainMetadata {
 public:
  PolkadotChainMetadata();
  PolkadotChainMetadata(const PolkadotChainMetadata&);
  PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept;
  ~PolkadotChainMetadata();

  PolkadotChainMetadata& operator=(const PolkadotChainMetadata&);
  PolkadotChainMetadata& operator=(PolkadotChainMetadata&&) noexcept;
  bool operator==(const PolkadotChainMetadata& other) const;

  // Fallibly parse runtime metadata bytes and extract the minimal
  // fields needed for transfer extrinsics.
  static std::optional<PolkadotChainMetadata> FromBytes(
      base::span<const uint8_t> metadata_bytes);

  // Build metadata from explicit fields. Always succeeds.
  static PolkadotChainMetadata FromFields(
      uint8_t system_pallet_index,
      uint8_t balances_pallet_index,
      uint8_t transaction_payment_pallet_index,
      uint8_t transfer_allow_death_call_index,
      uint8_t transfer_keep_alive_call_index,
      uint8_t transfer_all_call_index,
      uint16_t ss58_prefix,
      uint32_t spec_version,
      const std::array<uint8_t, kMaxSignedExtensions>& signed_extensions,
      bool has_assets_pallet,
      uint8_t assets_pallet_index,
      uint8_t assets_transfer_all_call_index,
      uint8_t assets_transfer_keep_alive_call_index);

  // Obtain a reference to the underlying opaque type so that it can be passed
  // to Rust routines.
  CxxPolkadotChainMetadata& operator*();
  const CxxPolkadotChainMetadata& operator*() const;

  // We want to expose the native data members of the underlying
  // CxxPolkadotChainMetadata struct, but the cxx crate marks generated structs
  // as final so we can't simply inherit from them publicly. To make accessing
  // the data members more ergonomic, we overload the arrow operator to return a
  // pointer to the generated struct.
  CxxPolkadotChainMetadata* operator->();
  const CxxPolkadotChainMetadata* operator->() const;

 private:
  explicit PolkadotChainMetadata(CxxPolkadotChainMetadata chain_metadata);

  CxxPolkadotChainMetadata chain_metadata_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_H_
