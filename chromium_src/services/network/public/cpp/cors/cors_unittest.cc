/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/public/cpp/cors/cors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {
namespace cors {

typedef testing::Test CorsTest;

TEST_F(CorsTest, SafelistedHeader) {
  EXPECT_TRUE(IsCorsSafelistedHeader("accept", "foo"));
  EXPECT_FALSE(IsCorsSafelistedHeader("foo", "bar"));
  EXPECT_FALSE(IsCorsSafelistedHeader("user-agent", "foo"));
  EXPECT_TRUE(IsCorsSafelistedHeader("x-brave-partner", "foo"));
}

}  // namespace cors
}  // namespace network
