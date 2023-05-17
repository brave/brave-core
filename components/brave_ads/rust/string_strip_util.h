/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_RUST_STRING_STRIP_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_RUST_STRING_STRIP_UTIL_H_

#include <string>

namespace brave_ads::rust {

std::string StripNonAlphaCharacters(const std::string& value);
std::string StripNonAlphaNumericCharacters(const std::string& value);

}  // namespace brave_ads::rust

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_RUST_STRING_STRIP_UTIL_H_
