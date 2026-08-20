/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/to_string.h"

#include "base/containers/span.h"
#include "base/test/test_future.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ToString, Uint256) {
  using uint256_t = unsigned _BitInt(256);

  uint256_t val{1};
  // `ToString` compiles and uses fallback representation for uint256_t.
  EXPECT_THAT(base::ToString(val),
              testing::StartsWith("[32-byte object at 0x"));

  // TestFuture::SetValue is also able to compile.
  base::test::TestFuture<uint256_t> future;
  future.SetValue(uint256_t{7});
  EXPECT_EQ(future.Take(), uint256_t{7});
}
