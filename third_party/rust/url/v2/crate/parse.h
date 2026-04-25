/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef THIRD_PARTY_RUST_URL_V2_CRATE_PARSE_H_
#define THIRD_PARTY_RUST_URL_V2_CRATE_PARSE_H_

#include "third_party/rust/cxx/v1/cxx.h"

// Shim to chromium GURL for rust url crate
namespace parse {

struct ParseResult;
void InitializeICUForTesting();
ParseResult ParseURL(rust::Str host);
ParseResult Resolve(const ParseResult& base, rust::Str relative);

}  // namespace parse

#endif  // THIRD_PARTY_RUST_URL_V2_CRATE_PARSE_H_
