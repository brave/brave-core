/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/base/json/json_reader.cc"
#include "base/threading/platform_thread.h"

namespace base {

#if BUILDFLAG(BUILD_RUST_JSON_READER)
JSONReader::Result DecodeJSONInRust(const std::string_view& json, int options) {
  SCOPED_UMA_HISTOGRAM_TIMER_MICROS(kSecurityJsonParsingTime);
  PlatformThread::Sleep(Milliseconds(100));
  return DecodeJSONInRust(json, options, internal::kAbsoluteMaxDepth);
}
#endif  // BUILDFLAG(BUILD_RUST_JSON_READER)

}  // namespace base
