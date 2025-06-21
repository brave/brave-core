/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_WRITER_H_
#define BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_WRITER_H_

// Adding a value to allow us to serialise 64-bit from blobs.
#define OPTIONS_PRETTY_PRINT \
  OPTIONS_SERIALISE_64BIT_NUMBERS = 1 << 3, OPTIONS_PRETTY_PRINT
#define pretty_print_ \
  pretty_print_;      \
  bool serialise_64bit_numbers_

#include "src/base/json/json_writer.h"  // IWYU pragma: export

#undef OPTIONS_PRETTY_PRINT
#undef pretty_print_

#endif  // BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_WRITER_H_
