// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <fuzzer/FuzzedDataProvider.h>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/commander/fuzzy_finder.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  FuzzedDataProvider provider(data, size);
  std::vector<gfx::Range> ranges;
  std::u16string needle =
      base::UTF8ToUTF16(provider.ConsumeRandomLengthString());
  std::u16string haystack =
      base::UTF8ToUTF16(provider.ConsumeRandomLengthString());

  commander::FuzzyFinder(needle).Find(haystack, &ranges);
  return 0;
}
