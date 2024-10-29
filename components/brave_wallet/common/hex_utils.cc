/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/hex_utils.h"

#include <limits>
#include <optional>

#include "base/containers/adapters.h"
#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace brave_wallet {

std::string ToHex(const std::string& data) {
  if (data.empty()) {
    return "0x0";
  }
  return "0x" + base::ToLowerASCII(base::HexEncode(data.data(), data.size()));
}

std::string ToHex(base::span<const uint8_t> data) {
  if (data.empty()) {
    return "0x0";
  }
  return "0x" + base::ToLowerASCII(base::HexEncode(data));
}

// Returns a hex string representation of a binary buffer. The returned hex
// string will be in lower case, without the 0x prefix.
std::string HexEncodeLower(const void* bytes, size_t size) {
  return base::ToLowerASCII(base::HexEncode(bytes, size));
}

std::string HexEncodeLower(base::span<const uint8_t> bytes) {
  return base::ToLowerASCII(base::HexEncode(bytes));
}

// Determines if the passed in hex string is valid
bool IsValidHexString(const std::string& hex_input) {
  if (hex_input.length() < 2) {
    return false;
  }
  if (!hex_input.starts_with("0x")) {
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
      out->clear();
      return false;
    }
    *out += hex_inputs[i].substr(2);
  }

  return true;
}

bool HexValueToUint256(const std::string& hex_input, uint256_t* out) {
  if (!out) {
    return false;
  }
  if (!IsValidHexString(hex_input)) {
    return false;
  }
  *out = 0;
  uint256_t last_val = 0;  // Used to check overflows
  for (char c : hex_input.substr(2)) {
    (*out) <<= 4;
    (*out) += static_cast<uint256_t>(base::HexDigitToInt(c));
    if (last_val > *out) {
      return false;
    }
    last_val = *out;
  }
  return true;
}

bool HexValueToInt256(const std::string& hex_input, int256_t* out) {
  if (!out) {
    return false;
  }
  uint256_t val;
  if (!HexValueToUint256(hex_input, &val)) {
    return false;
  }
  // This is the same as ~val + 1
  // To convert a positive number into a negative number, using the two’s
  // complement representation, invert all of the bits of the number + 1
  *out = static_cast<int256_t>(val);
  return true;
}

std::string Uint256ValueToHex(uint256_t input) {
  std::string result;
  result.reserve(34);
  result.append("0x");

  auto input_span = base::byte_span_from_ref(input);
  bool skipping_zeros = true;
  for (auto& byte : base::Reversed(input_span)) {
    if (skipping_zeros && !byte) {
      continue;
    }
    if (skipping_zeros && byte) {
      skipping_zeros = false;
      if (byte <= 0xf) {
        std::string one_char_byte;
        base::AppendHexEncodedByte(byte, one_char_byte, false);
        result += one_char_byte[1];
        continue;
      }
    }
    base::AppendHexEncodedByte(byte, result, false);
  }

  if (result.size() == 2) {
    return result += '0';  // 0x0 case
  }

  return result;
}

bool PrefixedHexStringToBytes(const std::string& input,
                              std::vector<uint8_t>* bytes) {
  CHECK(bytes);
  bytes->clear();
  if (!IsValidHexString(input)) {
    return false;
  }
  if (input.size() == 2) {
    // Valid hex string of size 2 must be "0x"
    DCHECK_EQ(input, "0x");
    return true;
  }
  std::string hex_substr = input.substr(2);
  if (hex_substr.length() % 2 == 1) {
    hex_substr = "0" + hex_substr;
  }
  if (!base::HexStringToBytes(hex_substr, bytes)) {
    return false;
  }
  return true;
}

std::optional<std::vector<uint8_t>> PrefixedHexStringToBytes(
    const std::string& input) {
  std::vector<uint8_t> result;
  if (!PrefixedHexStringToBytes(input, &result)) {
    return std::nullopt;
  }

  return result;
}

}  // namespace brave_wallet
