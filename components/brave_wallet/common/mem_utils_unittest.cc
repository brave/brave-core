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
  SecureZeroData(&a, sizeof(a));
  EXPECT_EQ(a, 0);
  std::string b = "brave";
  SecureZeroData(&b, sizeof(b));
  EXPECT_TRUE(b.empty());
  std::vector<uint8_t> c = {0xde, 0xad, 0xbe, 0xef};
  SecureZeroData(&c, sizeof(c));
  for (const auto& byte : c) {
    EXPECT_EQ(byte, 0);
  }
}

}  // namespace brave_wallet
