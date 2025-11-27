/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_

#include <stdint.h>

#include <array>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Using the name of the chain spec provided here:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#154-system_chain
//
// we obtain the runtime metadata associated with the chain.
//
// In the future, the chain spec name will most likely be replaced by the hex
// string that describes the capabilities of the connected remote described
// here:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#1119-state_getmetadata
// https://spec.polkadot.network/sect-metadata
//
// To this end, we're able to support the relay chain and multiple independent
// parachains which will all contain their own pallet indices for the common
// pallets we need like Balance.
class PolkadotChainMetadata {
 public:
  PolkadotChainMetadata() = delete;
  PolkadotChainMetadata(PolkadotChainMetadata&&);
  ~PolkadotChainMetadata();

  // Fallibly retrieve the metadata associated with the provided chain spec.
  static std::optional<PolkadotChainMetadata> FromChainName(
      std::string_view chain_name);

  // Obtain a reference to the underlying opaque type so that it can be passed
  // to Rust routines.
  const CxxPolkadotChainMetadata& operator*() const;

 private:
  explicit PolkadotChainMetadata(
      ::rust::Box<CxxPolkadotChainMetadata> chain_metadata);

  rust::Box<CxxPolkadotChainMetadata> chain_metadata_;
};

// An unsigned extrinsic that represents the "transfer_allow_death" call from
// the Balances pallet. Note, the hosted Westend nodes uses the same runtime
// metadata as the Kusama chains which have the Balances pallet living at
// index 4.
// For more information on the structure of extrinsics and their nature, see the
// following documentation: https://spec.polkadot.network/id-extrinsics
class PolkadotUnsignedTransfer {
 public:
  PolkadotUnsignedTransfer(
      base::span<uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      uint128_t send_amount);

  // Encode is the analog of the polkadot-js api's toHex() implementation which
  // creates a hex string encoded with the SCALE-encoded length of the string
  // along with the extrinsic version, the pallet index, call index, and the
  // account address type.
  std::string Encode(const PolkadotChainMetadata& chain_metadata) const;

  // Decode recreates the unsigned transfer extrinsic from the provided
  // hex string. This method is the dual of Encode().
  static std::optional<PolkadotUnsignedTransfer> Decode(
      const PolkadotChainMetadata& chain_metadata,
      std::string_view input);

  // Get the send amount associated with this extrinsic.
  uint128_t send_amount() const { return send_amount_; }

  // Get a view of the public key of the intended recipient for this
  // transaction.
  base::span<const uint8_t> recipient() const { return recipient_; }

 private:
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_ = {};
  uint128_t send_amount_ = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_
