/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/base/json/json_reader.cc"

namespace base {

#if BUILDFLAG(BUILD_RUST_JSON_READER)
JSONReader::Result DecodeJSONInRust(std::string_view json, int options) {
  SCOPED_UMA_HISTOGRAM_TIMER_MICROS(kSecurityJsonParsingTime);
  return DecodeJSONInRust(json, options, internal::kAbsoluteMaxDepth);
}
#endif  // BUILDFLAG(BUILD_RUST_JSON_READER)

}  // namespace base
