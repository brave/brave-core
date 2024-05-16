/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/mem_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(MemUtilsUnitTest, SecureZeroData) {
  int a = 123;
  SecureZeroData(base::byte_span_from_ref(a));
  EXPECT_EQ(a, 0);
  std::vector<uint8_t> c = {0xde, 0xad, 0xbe, 0xef};
  SecureZeroData(base::as_writable_byte_span(c));
  for (const auto& byte : c) {
    EXPECT_EQ(byte, 0);
  }
}

}  // namespace brave_wallet
