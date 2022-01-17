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
  for (const auto& c : input) {
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
  for (char c : input) {
    (*out) *= 10;
    // We can use this because we know the input string is 0-9 digits only
    (*out) += static_cast<uint256_t>(base::HexDigitToInt(c));
  }
  return true;
}

bool Base10ValueToInt256(const std::string& input, int256_t* out) {
  if (!out)
    return false;
  uint256_t val;
  if (!Base10ValueToUint256(input, &val))
    return false;
  if (val > std::numeric_limits<int256_t>::max())
    return false;
  *out = static_cast<int256_t>(val);
  return true;
}

}  // namespace brave_wallet
