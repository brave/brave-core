/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/string_util.h"

#include <iomanip>
#include <sstream>

namespace ads {

std::string BytesToHexString(
    const std::vector<uint8_t>& bytes) {
  std::ostringstream hex_string;

  for (size_t i = 0; i < bytes.size(); i++) {
    hex_string << std::setfill('0') << std::setw(sizeof(uint8_t) * 2)
       << std::hex << static_cast<int>(bytes[i]);
  }

  return hex_string.str();
}

}  // namespace ads
