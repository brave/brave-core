/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <fuzzer/FuzzedDataProvider.h>

#include <iostream>
#include <string>

#include "brave/components/adblock_rust_ffi/src/wrapper.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FuzzedDataProvider data_provider(data, size);

  const auto& input = data_provider.ConsumeRemainingBytesAsString();
  if (::getenv("LPM_DUMP_NATIVE_INPUT")) {
    std::cout << input << std::endl;
  }

  adblock::Engine engine;
  engine.useResources(input);
  return 0;
}
