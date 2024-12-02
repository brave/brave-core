/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/json/json_writer.h"

#define BRAVE_JSON_WRITER_BUILD_STRING_JSON           \
  if (serialise_64bit_numbers_) {                     \
    json_string_->append(as_string_view(span(node))); \
    return true;                                      \
  }

#define BRAVE_JSON_WRITER_SERIALISE_64BIT_NUMBERS_ARG \
  serialise_64bit_numbers_((options & OPTIONS_SERIALISE_64BIT_NUMBERS) != 0),
#include "src/base/json/json_writer.cc"
#undef BRAVE_JSON_WRITER_SERIALISE_64BIT_NUMBERS_ARG
#undef BRAVE_JSON_WRITER_BUILD_STRING_JSON
