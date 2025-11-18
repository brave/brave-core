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
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

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
  std::string Encode(std::string_view chain_id) const;

  // Decode recreates the unsigned transfer extrinsic from the provided
  // hex string. This method is the dual of Encode().
  static std::optional<PolkadotUnsignedTransfer> Decode(std::string_view input);

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
