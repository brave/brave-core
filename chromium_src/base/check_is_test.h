/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_CHECK_IS_TEST_H_
#define BRAVE_CHROMIUM_SRC_BASE_CHECK_IS_TEST_H_

#include "base/gtest_prod_util.h"

#include "src/base/check_is_test.h"  // IWYU pragma: export

namespace variations {
class PublicKeyWrapper;
}

namespace base {

enum class TestVendor { kNone, kChromium, kBrave };

// Returns currently running test vendor. Private interface to allow-list usage.
class BASE_EXPORT CurrentTestVendor {
 private:
  FRIEND_TEST_ALL_PREFIXES(CurrentTestVendorTest, IsBrave);
  friend variations::PublicKeyWrapper;
  static TestVendor Get();
};

}  // namespace base

#endif  // BRAVE_CHROMIUM_SRC_BASE_CHECK_IS_TEST_H_
