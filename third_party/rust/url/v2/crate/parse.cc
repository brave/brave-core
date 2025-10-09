/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/check.h"
#include "base/i18n/icu_util.h"
#include "brave/third_party/rust/url/v2/crate/parse.h"
#include "brave/third_party/rust/url/v2/crate/src/lib.rs.h"
#include "url/gurl.h"

namespace parse {

void InitializeICUForTesting() {
  CHECK(base::i18n::InitializeICU());
}

ParseResult ParseURL(rust::Str url_string) {
  GURL url((std::string(url_string)));
  ParseResult res;
  res.serialization = url.possibly_invalid_spec();
  res.has_host = url.has_host();
  res.host = std::string(url.host());
  res.has_path = url.has_path();
  res.path = std::string(url.path());
  res.has_fragment = url.has_ref();
  res.fragment = std::string(url.ref());
  res.has_scheme = url.has_scheme();
  res.scheme = std::string(url.scheme());
  res.has_query = url.has_query();
  res.query = std::string(url.query());
  res.has_port = url.has_port();
  // Max port value is 65535 and this has already been parsed and validated
  // by GURL
  res.port = base::saturated_cast<uint16_t>(url.IntPort());
  res.valid = url.is_valid();
  return res;
}

ParseResult Resolve(const ParseResult& base, rust::Str path) {
  return ParseURL(GURL((std::string(base.serialization))).Resolve((std::string(path))).possibly_invalid_spec());
}

}  // namespace parse
