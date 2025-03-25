/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check_is_test.h"

#include "src/base/check_is_test.cc"

namespace {
bool g_this_is_a_brave_test = false;
}

namespace base {
namespace test {

BASE_EXPORT void SetTestVendorIsBraveForTesting() {
  g_this_is_a_brave_test = true;
}

}  // namespace test

// static
TestVendor CurrentTestVendor::Get() {
  if (g_this_is_a_brave_test) {
    return TestVendor::kBrave;
  }
  if (g_this_is_a_test) {
    return TestVendor::kChromium;
  }
  return TestVendor::kNone;
}

}  // namespace base
