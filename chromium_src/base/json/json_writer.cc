/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/json/json_writer.h"

#define BRAVE_JSON_WRITER_CTOR \
  serialise_64bit_numbers_((options & OPTIONS_SERIALISE_64BIT_NUMBERS) != 0),
#define BRAVE_JSON_WRITER_BUILD_JSON_STRING           \
  if (serialise_64bit_numbers_) {                     \
    json_string_->append(as_string_view(span(node))); \
    return true;                                      \
  }

#include "src/base/json/json_writer.cc"

#undef BRAVE_JSON_WRITER_CTOR
#undef BRAVE_JSON_WRITER_BUILD_JSON_STRING
