/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/base/url_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net {

TEST(URLUtilUnitTest, IsHTTPSOrLocalhostURL) {
  EXPECT_TRUE(IsHTTPSOrLocalhostURL(GURL("https://example.com")));
  EXPECT_TRUE(IsHTTPSOrLocalhostURL(GURL("http://localhost")));
  EXPECT_TRUE(IsHTTPSOrLocalhostURL(GURL("http://127.0.0.1/test")));
  EXPECT_FALSE(IsHTTPSOrLocalhostURL(GURL("http://test.com")));
  EXPECT_FALSE(IsHTTPSOrLocalhostURL(GURL("wss://example.com")));
}

}  // namespace net
