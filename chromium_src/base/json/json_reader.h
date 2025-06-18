/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_
#define BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_

// Extra value to allow us to handle 64bit integers as blobs.
#define JSON_ALLOW_VERT_TAB \
  JSON_ALLOW_64BIT_NUMBERS = 1 << 6, JSON_ALLOW_VERT_TAB

#include "src/base/json/json_reader.h"  // IWYU pragma: export

#undef JSON_ALLOW_VERT_TAB

#endif  // BRAVE_CHROMIUM_SRC_BASE_JSON_JSON_READER_H_
