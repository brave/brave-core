/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/mojom/uint256_mojom_traits.h"

#include "base/strings/string_util.h"

using uint256_t = ::brave_wallet::uint256_t;

namespace mojo {

namespace {

char NibbleToChar(uint8_t val) {
  if (val >= 0 && val < 0xa) {
    return '0' + val;
  }
  return 'a' + val - 0xa;
}

bool HexValueToUint256(std::string_view hex_input, uint256_t* out) {
  if (!out) {
    return false;
  }
  if (!hex_input.starts_with("0x") || hex_input.length() > 2 + 32) {
    return false;
  }
  *out = 0;
  for (char c : hex_input.substr(2)) {
    if (base::IsHexDigit(c)) {
      return false;
    }
    (*out) <<= 4;
    (*out) += static_cast<uint256_t>(base::HexDigitToInt(c));
  }
  return true;
}

std::string Uint256ValueToHex(uint256_t input) {
  if (input == 0) {
    return "0x0";  // Special case for zero.
  }

  uint32_t significant_bits_count = 256 - (__builtin_clzg(input) / 4) * 4;

  std::string result;
  result.reserve(2 + significant_bits_count / 4);
  result.append("0x");

  for (uint32_t i = 4; i <= significant_bits_count; i += 4) {
    result += NibbleToChar((input >> (significant_bits_count - i)) & 0xF);
  }

  return result;
}

}  // namespace

// static
std::string StructTraits<::brave_wallet::mojom::Uint256DataView,
                         uint256_t>::uint256_hex(const uint256_t& input) {
  return Uint256ValueToHex(input);
}

// static
bool StructTraits<::brave_wallet::mojom::Uint256DataView, uint256_t>::Read(
    brave_wallet::mojom::Uint256DataView data,
    uint256_t* out) {
  mojo::StringDataView uint256_hex;
  data.GetUint256HexDataView(&uint256_hex);
  return !uint256_hex.is_null() && HexValueToUint256(uint256_hex.value(), out);
}

}  // namespace mojo
