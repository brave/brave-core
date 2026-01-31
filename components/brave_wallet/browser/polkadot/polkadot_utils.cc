/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

std::optional<PolkadotAddress> ParsePolkadotAccount(const std::string& input,
                                                    uint16_t ss58_prefix) {
  auto ss58_address = Ss58Address::Decode(input);
  if (ss58_address) {
    if (ss58_address->prefix != ss58_prefix) {
      return std::nullopt;
    }
    return PolkadotAddress{ss58_address->public_key, ss58_prefix};
  }

  // Note: Avoid using PrefixedHexStringToFixed here because it accepts hex
  // strings of the form: 0x123 which is undesireable when being used as a
  // recipient address of funds.
  std::string_view str = input;
  str = base::RemovePrefix(str, "0x").value_or({});
  if (str.empty()) {
    return std::nullopt;
  }

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  if (base::HexStringToSpan(str, pubkey)) {
    return PolkadotAddress{pubkey, std::nullopt};
  }

  return std::nullopt;
}

std::optional<std::string> PolkadotAddress::ToString() const {
  if (ss58_prefix.has_value()) {
    Ss58Address addr;
    addr.prefix = *ss58_prefix;
    addr.public_key = pubkey;
    return addr.Encode();
  }

  return "0x" + base::HexEncodeLower(pubkey);
}

mojom::uint128Ptr Uint128ToMojom(uint128_t x) {
  return mojom::uint128::New(x >> 64, x & 0xffffffffffffffff);
}

uint128_t MojomToUint128(const mojom::uint128Ptr& x) {
  return (uint128_t{x->high} << 64) | uint128_t{x->low};
}

}  // namespace brave_wallet
