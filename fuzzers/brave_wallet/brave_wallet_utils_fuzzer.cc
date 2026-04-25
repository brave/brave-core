/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <fuzzer/FuzzedDataProvider.h>

#include <string>

#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

struct Environment {
  Environment() { logging::SetMinLogLevel(logging::LOG_FATAL); }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static Environment env;

  FuzzedDataProvider data_provider(data, size);

  const size_t array_size =
      data_provider.ConsumeIntegralInRange<size_t>(0, 128);
  const size_t offset = data_provider.ConsumeIntegralInRange<size_t>(0, 1024);
  const std::string input = data_provider.ConsumeRemainingBytesAsString();

  std::string output;
  brave_wallet::EncodeString(input, &output);
  brave_wallet::EncodeStringArray(std::vector<std::string>(array_size, input),
                                  &output);

  brave_wallet::DecodeString(offset, input, &output);
  return 0;
}
