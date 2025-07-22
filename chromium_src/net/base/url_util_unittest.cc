/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/net/base/url_util_unittest.cc"

namespace net {
namespace {

TEST(UrlUtilTest, IsOnionOrigin) {
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.onion/"))));
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.onion:20000/"))));
  EXPECT_TRUE(IsOnion(url::Origin::Create(GURL("https://foo.bar.onion/"))));
  EXPECT_FALSE(IsOnion(url::Origin::Create(GURL("https://foo.onion.com/"))));
  EXPECT_FALSE(IsOnion(url::Origin::Create(GURL("https://foo.com/b.onion"))));
}

TEST(UrlUtilTest, IsOnionUrlSchemes) {
  EXPECT_TRUE(IsOnion(GURL("https://foo.onion/")));
  EXPECT_TRUE(IsOnion(GURL("Http://foo.bar.onion/")));
  EXPECT_TRUE(IsOnion(GURL("wSs://foo.onion/bar")));
  EXPECT_TRUE(IsOnion(GURL("WS://foo.onion/bar")));
  EXPECT_FALSE(IsOnion(GURL("file:///foo.onion/bar")));
  EXPECT_FALSE(IsOnion(GURL("ftp://foo.onion/")));
  EXPECT_FALSE(IsOnion(GURL("data:,foo.onion")));
  EXPECT_FALSE(IsOnion(GURL("chrome://onion/")));
}

}  // namespace
}  // namespace net
