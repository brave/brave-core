/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/string_utils.h"

#include <limits>

#include "base/strings/string_util.h"

namespace brave_wallet {

// Determines if the passed in base-10 string is valid
bool IsValidBase10String(const std::string& input) {
  if (input.empty())
    return false;
  std::string check_input = input;
  if (input.size() > 0 && input[0] == '-')
    check_input = input.substr(1);
  for (const auto& c : check_input) {
    if (!base::IsAsciiDigit(c))
      return false;
  }
  return true;
}

bool Base10ValueToUint256(const std::string& input, uint256_t* out) {
  if (!out)
    return false;
  if (!IsValidBase10String(input))
    return false;
  *out = 0;
  uint256_t last_val = 0;  // Used to check overflows
  for (char c : input) {
    (*out) *= 10;
    // We can use this because we know the input string is 0-9 digits only
    (*out) += static_cast<uint256_t>(base::HexDigitToInt(c));
    if (last_val > *out) {
      return false;
    }
    last_val = *out;
  }
  return true;
}

bool Base10ValueToInt256(const std::string& input, int256_t* out) {
  if (!out)
    return false;
  if (!IsValidBase10String(input))
    return false;
  *out = 0;
  int256_t last_val = 0;  // Used to check overflows
  std::string check_input = input;
  bool negative = false;
  if (input.size() > 0 && input[0] == '-') {
    check_input = input.substr(1);
    negative = true;
  }

  for (char c : check_input) {
    (*out) *= 10;
    // We can use this because we know the input string is 0-9 digits only
    if (negative) {
      (*out) -= static_cast<int256_t>(base::HexDigitToInt(c));
    } else {
      (*out) += static_cast<int256_t>(base::HexDigitToInt(c));
    }
    if (!negative && (last_val > *out || *out > kMax256BitInt)) {
      return false;
    } else if (negative && (last_val < *out || *out < kMin256BitInt)) {
      return false;
    }
    last_val = *out;
  }
  return true;
}

}  // namespace brave_wallet
