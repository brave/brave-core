/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_
#define BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_

#include "base/rust_buildflags.h"       // IWYU pragma: private

// JSON_ALLOW_64BIT_NUMBERS is used by the parse to determine if 64-bit numbers
// should be allowed as binary data.
#define JSON_ALLOW_VERT_TAB \
  JSON_ALLOW_64BIT_NUMBERS = 1 << 7, JSON_ALLOW_VERT_TAB
#include "src/base/json/json_reader.h"  // IWYU pragma: export
#undef JSON_ALLOW_VERT_TAB

namespace base {

#if BUILDFLAG(BUILD_RUST_JSON_READER)
BASE_EXPORT JSONReader::Result DecodeJSONInRust(std::string_view json,
                                                int options);
#endif  // BUILDFLAG(BUILD_RUST_JSON_READER)

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_
