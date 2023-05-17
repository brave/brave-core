/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/rust/string_strip_util.h"

#include "brave/components/brave_ads/rust/lib.rs.h"

namespace brave_ads::rust {

std::string StripNonAlphaCharacters(const std::string& value) {
  return static_cast<std::string>(strip_non_alpha_characters(value));
}

std::string StripNonAlphaNumericCharacters(const std::string& value) {
  return static_cast<std::string>(strip_non_alpha_numeric_characters(value));
}

}  // namespace brave_ads::rust
