/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <fuzzer/FuzzedDataProvider.h>

#include <string>

#include "brave/components/speedreader/rust/ffi/speedreader.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FuzzedDataProvider data_provider(data, size);

  speedreader::SpeedReader speedreader;
  auto rewriter = speedreader.MakeRewriter("https://example.com");
  while (data_provider.remaining_bytes()) {
    const auto& chunk = data_provider.ConsumeRandomLengthString(256);
    rewriter->Write(chunk.data(), chunk.size());
  }
  rewriter->End();
  return 0;
}
