// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <base/strings/string_number_conversions.cc>

namespace base {

std::string HexEncodeLower(base::span<const uint8_t> bytes) {
  // Each input byte creates two output hex characters.
  std::string ret;
  ret.reserve(bytes.size() * 2);

  for (uint8_t byte : bytes) {
    AppendHexEncodedByte(byte, ret, /*uppercase=*/false);
  }
  return ret;
}

std::string HexEncodeLower(std::string_view chars) {
  return HexEncodeLower(base::as_byte_span(chars));
}

}  // namespace base
