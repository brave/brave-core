/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/brave_wallet_utils.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/third_party/ethash/src/include/ethash/keccak.h"

namespace brave_wallet {

std::string ToHex(const std::string& data) {
  if (data.empty()) {
    return "0x0";
  }
  return base::StringPrintf(
      "0x%s",
      base::ToLowerASCII(base::HexEncode(data.data(), data.size())).c_str());
}

std::string KeccakHash(const std::string& input) {
  auto hash = ethash_keccak256(
      reinterpret_cast<uint8_t*>(const_cast<char*>(input.data())),
      input.size());
  std::string result(hash.str, sizeof(hash.str) / sizeof(hash.str[0]));
  return ToHex(result);
}

std::string GetFunctionHash(const std::string& input) {
  auto result = KeccakHash(input);
  return result.substr(0, std::min(static_cast<size_t>(10), input.length()));
}

}  // namespace brave_wallet
