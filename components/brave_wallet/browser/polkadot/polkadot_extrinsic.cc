/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {

// "Balances" pallet lives at index 4:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/kusama-json.json#L921
// https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
inline constexpr uint8_t kPolkadotTestnetBalancesPallet = 4;
inline constexpr uint8_t kPolkadotTestnetTransferAllowDeathCallIndex = 0;

// "Balances" pallet lives at index 5:
// https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
inline constexpr uint8_t kPolkadotMainnetBalancesPallet = 5;
inline constexpr uint8_t kPolkadotMainnetTransferAllowDeathCallIndex = 0;

}  // namespace

PolkadotUnsignedTransfer::PolkadotUnsignedTransfer(
    base::span<uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    uint128_t send_amount)
    : send_amount_(send_amount) {
  base::span(recipient_).copy_from_nonoverlapping(recipient);
}

std::string PolkadotUnsignedTransfer::Encode(std::string_view chain_id) const {
  CHECK(IsPolkadotNetwork(chain_id));

  uint8_t balances_pallet_idx = 0;
  uint8_t transfer_allow_death_call_index = 0;

  if (chain_id == mojom::kPolkadotTestnet) {
    balances_pallet_idx = kPolkadotTestnetBalancesPallet;
    transfer_allow_death_call_index =
        kPolkadotTestnetTransferAllowDeathCallIndex;
  } else {
    balances_pallet_idx = kPolkadotMainnetBalancesPallet;
    transfer_allow_death_call_index =
        kPolkadotMainnetTransferAllowDeathCallIndex;
  }

  auto buf = brave_wallet::encode_unsigned_transfer_allow_death(
      static_cast<uint64_t>(send_amount_ >> 64),
      static_cast<uint64_t>(send_amount_ & 0xffffffffffffffff), recipient_,
      balances_pallet_idx, transfer_allow_death_call_index);

  return base::HexEncode(std::vector<uint8_t>(buf.begin(), buf.end()));
}

// static
std::optional<PolkadotUnsignedTransfer> PolkadotUnsignedTransfer::Decode(
    std::string_view input) {
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  uint64_t send_amount_high = 0;
  uint64_t send_amount_low = 0;

  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(input, &bytes)) {
    return std::nullopt;
  }

  if (!brave_wallet::decode_unsigned_transfer_allow_death(
          rust::Slice<const uint8_t>(bytes.data(), bytes.size()),
          rust::Slice<uint8_t>(pubkey.data(), pubkey.size()), send_amount_high,
          send_amount_low)) {
    return std::nullopt;
  }

  uint128_t send_amount =
      (uint128_t{send_amount_high} << 64) | uint128_t{send_amount_low};

  return {PolkadotUnsignedTransfer(pubkey, send_amount)};
}

}  // namespace brave_wallet
