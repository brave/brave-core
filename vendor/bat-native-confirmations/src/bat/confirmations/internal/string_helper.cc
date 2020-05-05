/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/string_helper.h"

#include "base/logging.h"

namespace helper {

std::vector<uint8_t> String::decode_hex(const std::string& hexadecimal) {
  std::vector<uint8_t> bytes;

  if (hexadecimal.empty()) {
    return bytes;
  }

  for (size_t i = 0; i < hexadecimal.length(); i += 2) {
      std::string hexidecimal_byte = hexadecimal.substr(i, 2);
      uint8_t decimal = std::strtol(hexidecimal_byte.c_str(), 0, 16);
      bytes.push_back(decimal);
  }

  return bytes;
}

}  // namespace helper
