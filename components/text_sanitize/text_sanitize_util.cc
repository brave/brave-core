/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/text_sanitize/text_sanitize_util.h"

#include "brave/components/text_sanitize/rs/src/lib.rs.h"

namespace text_sanitize {

std::string StripNonAlphanumericOrAsciiCharacters(const std::string& text) {
  return static_cast<std::string>(
      strip_non_alphanumeric_or_ascii_characters(::rust::String::lossy(text)));
}

}  // namespace text_sanitize
