// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_
#define BRAVE_CHROMIUM_SRC_BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_

#include <base/strings/string_number_conversions.h>  // IWYU pragma: export

namespace base {

// Behaves like the above, but returns the hex string in lower case.
BASE_EXPORT std::string HexEncodeLower(base::span<const uint8_t> bytes);
BASE_EXPORT std::string HexEncodeLower(std::string_view chars);

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_
