/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/string_utils.h"

namespace confirmations {

std::vector<uint8_t> DecodeHexString(
    const std::string& string) {
  std::vector<uint8_t> bytes;

  if (string.empty()) {
    return bytes;
  }

  for (size_t i = 0; i < string.length(); i += 2) {
      const std::string hex = string.substr(i, 2);
      const uint8_t decimal = std::strtol(hex.c_str(), 0, 16);
      bytes.push_back(decimal);
  }

  return bytes;
}

}  // namespace confirmations
