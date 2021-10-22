/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hex_utils.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace brave_wallet {

std::string ToHex(const std::string& data) {
  if (data.empty()) {
    return "0x0";
  }
  return "0x" + base::ToLowerASCII(base::HexEncode(data.data(), data.size()));
}

std::string ToHex(const std::vector<uint8_t>& data) {
  if (data.empty())
    return "0x0";
  return "0x" + base::ToLowerASCII(base::HexEncode(data));
}

// Determines if the passed in hex string is valid
bool IsValidHexString(const std::string& hex_input) {
  if (hex_input.length() < 3) {
    return false;
  }
  if (!base::StartsWith(hex_input, "0x")) {
    return false;
  }
  for (const auto& c : hex_input.substr(2)) {
    if (!base::IsHexDigit(c)) {
      return false;
    }
  }
  return true;
}

// Pads a hex encoded parameter to 32-bytes
// i.e. 64 hex characters.
bool PadHexEncodedParameter(const std::string& hex_input, std::string* out) {
  if (!out) {
    return false;
  }
  if (!IsValidHexString(hex_input)) {
    return false;
  }
  if (hex_input.length() >= 64 + 2) {
    *out = hex_input;
    return true;
  }
  std::string hex_substr = hex_input.substr(2);
  size_t padding_len = 64 - hex_substr.length();
  std::string padding(padding_len, '0');

  *out = "0x" + padding + hex_substr;
  return true;
}

// Takes 2 inputs prefixed by 0x and combines them into an output with a single
// 0x. For example 0x1 and 0x2 would return 0x12
bool ConcatHexStrings(const std::string& hex_input1,
                      const std::string& hex_input2,
                      std::string* out) {
  if (!out) {
    return false;
  }
  if (!IsValidHexString(hex_input1) || !IsValidHexString(hex_input2)) {
    return false;
  }
  *out = hex_input1 + hex_input2.substr(2);
  return true;
}

bool ConcatHexStrings(const std::vector<std::string>& hex_inputs,
                      std::string* out) {
  if (!out) {
    return false;
  }
  if (hex_inputs.empty()) {
    return false;
  }
  if (!IsValidHexString(hex_inputs[0])) {
    return false;
  }

  *out = hex_inputs[0];
  for (size_t i = 1; i < hex_inputs.size(); i++) {
    if (!IsValidHexString(hex_inputs[i])) {
      return false;
    }
    *out += hex_inputs[i].substr(2);
  }

  return true;
}

}  // namespace brave_wallet
