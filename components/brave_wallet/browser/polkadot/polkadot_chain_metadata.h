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

// Runtime metadata is sourced from the hex string returned by
// state_getMetadata:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#1119-state_getmetadata
// https://spec.polkadot.network/sect-metadata
// We parse minimal fields needed for transfer extrinsic handling.
class PolkadotChainMetadata {
 public:
  PolkadotChainMetadata() = delete;
  PolkadotChainMetadata(const PolkadotChainMetadata&);
  PolkadotChainMetadata(PolkadotChainMetadata&&) noexcept;
  ~PolkadotChainMetadata();

  PolkadotChainMetadata& operator=(const PolkadotChainMetadata&);
  PolkadotChainMetadata& operator=(PolkadotChainMetadata&&) noexcept;

  // Fallibly parse runtime metadata bytes and extract the minimal
  // fields needed for transfer extrinsics.
  static std::optional<PolkadotChainMetadata> FromBytes(
      base::span<const uint8_t> metadata_bytes);

  // Build metadata from explicit fields. Always succeeds.
  static PolkadotChainMetadata FromFields(
      uint8_t balances_pallet_index,
      uint8_t transfer_allow_death_call_index,
      uint16_t ss58_prefix,
      uint32_t spec_version);

  // Build metadata from a known relay/parachain name returned by system_chain.
  // Returns std::nullopt for unknown names. The returned metadata has an
  // unknown spec_version (set to 0); callers must populate spec_version from
  // state_getRuntimeVersion before using it for version-sensitive operations.
  static std::optional<PolkadotChainMetadata> FromChainName(
      std::string_view chain_name);

  // Obtain a reference to the underlying opaque type so that it can be passed
  // to Rust routines.
  const CxxPolkadotChainMetadata& operator*() const;

  uint8_t GetBalancesPalletIndex() const;
  uint8_t GetTransferAllowDeathCallIndex() const;

  // Returns the ss58 prefix used by the chain when encoding/working with ss58
  // addresses.
  uint16_t GetSs58Prefix() const;
  uint32_t GetSpecVersion() const;

 private:
  explicit PolkadotChainMetadata(CxxPolkadotChainMetadata chain_metadata);

  CxxPolkadotChainMetadata chain_metadata_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_CHAIN_METADATA_H_
