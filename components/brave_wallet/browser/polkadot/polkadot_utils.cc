/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

std::optional<std::array<uint8_t, kPolkadotSubstrateAccountIdSize>>
ParsePolkadotAccount(const std::string& input, uint16_t ss58_prefix) {
  auto ss58_address = Ss58Address::Decode(input);
  if (ss58_address) {
    if (ss58_address->prefix != ss58_prefix) {
      return std::nullopt;
    }
    return ss58_address->public_key;
  }

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  if (base::HexStringToSpan(input, pubkey) ||
      PrefixedHexStringToFixed(input, pubkey)) {
    return pubkey;
  }

  return std::nullopt;
}
}  // namespace brave_wallet
