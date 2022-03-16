/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base64_util.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"

namespace ads {

std::vector<uint8_t> Base64ToBytes(const std::string& value_base64) {
  std::string value_as_string;
  base::Base64Decode(value_base64, &value_as_string);

  const std::string hex_encoded =
      base::HexEncode(value_as_string.data(), value_as_string.size());

  std::vector<uint8_t> bytes;
  base::HexStringToBytes(hex_encoded, &bytes);

  return bytes;
}

}  // namespace ads
