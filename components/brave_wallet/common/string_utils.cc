/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/string_utils.h"

#include <limits>
#include <string>
#include <string_view>

#include "base/numerics/safe_math.h"
#include "base/strings/string_util.h"

namespace brave_wallet {

// Determines if the passed in base-10 string is valid
bool IsValidBase10String(std::string_view input) {
  if (input.empty()) {
    return false;
  }
  auto check_input =
      !input.empty() && input.front() == '-' ? input.substr(1) : input;
  for (const auto& c : check_input) {
    if (!base::IsAsciiDigit(c)) {
      return false;
    }
  }
  return true;
}

std::optional<uint256_t> Base10ValueToUint256(std::string_view input) {
  if (!IsValidBase10String(input)) {
    return std::nullopt;
  }

  base::CheckedNumeric<uint256_t> out = 0;
  for (char c : input) {
    out *= 10u;
    out += static_cast<uint8_t>(base::HexDigitToInt(c));
  }

  if (!out.IsValid()) {
    return std::nullopt;
  }
  return out.ValueOrDie();
}

std::optional<int256_t> Base10ValueToInt256(std::string_view input) {
  if (!IsValidBase10String(input)) {
    return std::nullopt;
  }

  base::CheckedNumeric<int256_t> out = 0;
  bool negative = (!input.empty() && input.front() == '-');
  auto check_input = negative ? input.substr(1) : input;

  for (char c : check_input) {
    out *= 10;
    if (negative) {
      out -= base::HexDigitToInt(c);
    } else {
      out += base::HexDigitToInt(c);
    }
  }

  if (!out.IsValid()) {
    return std::nullopt;
  }
  return out.ValueOrDie();
}

std::string Uint256ValueToBase10(uint256_t input) {
  if (input == 0) {
    return "0";
  }

  std::string result;
  result.reserve(78);  // 78 is the max length of a uint256_t in base 10

  while (input > 0) {
    result += '0' + static_cast<uint8_t>(input % 10);
    input /= 10;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

}  // namespace brave_wallet
