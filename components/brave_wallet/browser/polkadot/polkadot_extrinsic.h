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

// PolkadotChainMetadata stores a hash map that maps a chain id from our side to
// the runtime metadata of the specified chain's ChainSpec. This value is
// currently obtained from a "system_chain" RPC call documented here:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#154-system_chain
//
// In the future, this will most likely be replaced by the hex string that
// describes the capabilities of the connected remote described here:
// https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#1119-state_getmetadata
// https://spec.polkadot.network/sect-metadata
//
// To this end, we're able to support the relay chain and multiple independent
// parachains which will all contain their own pallet indices for the common
// pallets we need like Balance.
struct PolkadotChainMetadata {
 public:
  PolkadotChainMetadata();
  ~PolkadotChainMetadata();

  // Associate the chain metadata for chain_name with the chain_id we use on our
  // end.
  void AddChainMetadata(std::string_view chain_id, std::string_view chain_name);

  // Returns a boolean that lets us know if the associated chain_id has a
  // backing ChainMetadata associated with it.
  bool HasChainMetadata(std::string_view chain_id) const;

  const ChainMetadata& chain_metadata() const { return *chain_metadata_; }

 private:
  ::rust::Box<ChainMetadata> chain_metadata_;
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
  std::string Encode(std::string_view chain_id,
                     const PolkadotChainMetadata& chain_metadata) const;

  // Decode recreates the unsigned transfer extrinsic from the provided
  // hex string. This method is the dual of Encode().
  static std::optional<PolkadotUnsignedTransfer> Decode(
      std::string_view chain_id,
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
