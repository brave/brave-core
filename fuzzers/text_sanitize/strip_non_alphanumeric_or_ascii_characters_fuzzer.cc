/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <fuzzer/FuzzedDataProvider.h>

#include "base/strings/string_util.h"
#include "brave/components/text_sanitize/rs/src/lib.rs.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FuzzedDataProvider data_provider(data, size);
  std::string data_string = data_provider.ConsumeRemainingBytesAsString();

  if (base::IsStringUTF8AllowingNoncharacters(data_string)) {
    text_sanitize::strip_non_alphanumeric_or_ascii_characters(data_string);
  }

  return 0;
}
