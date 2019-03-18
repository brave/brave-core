/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"


#include "chrome/test/base/chrome_render_view_host_test_harness.h"

namespace {

typedef testing::Test BraveShieldsExceptionsTest;
using brave::IsWhitelistedReferrer;
using brave::IsWhitelistedCookieException;

TEST_F(BraveShieldsExceptionsTest, IsWhitelistedReferrer) {
  // *.fbcdn.net not allowed on some other URL
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://test.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // *.fbcdn.net allowed on Facebook
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // Facebook doesn't allow just anything
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
        GURL("https://test.com")));
  // Allowed for reddit.com
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://www.redditmedia.com/97")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://cdn.embedly.com/157")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://imgur.com/179")));
  // Not allowed for reddit.com
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.reddit.com"),
        GURL("https://test.com")));
  // Not allowed imgur on another domain
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.test.com"),
        GURL("https://imgur.com/173")));
  // Fonts allowed anywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://use.typekit.net/193")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://cloud.typography.com/199")));
  // geetest allowed everywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://binance.com"),
      GURL("https://api.geetest.com/ajax.php?")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("http://binance.com"),
      GURL("https://api.geetest.com/")));
  // not allowed with a different scheme
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("http://binance.com"),
      GURL("http://api.geetest.com/")));
  // Google Accounts only allows a specific hostname
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://accounts.google.com"),
      GURL("https://content.googleapis.com/cryptauth/v1/authzen/awaittx")));
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://accounts.google.com"),
      GURL("https://ajax.googleapis.com/ajax/libs/d3js/5.7.0/d3.min.js")));
}

TEST_F(BraveShieldsExceptionsTest, IsWhitelistedCookieException) {
  // Cookie exceptions for Google auth domains
  EXPECT_TRUE(IsWhitelistedCookieException(GURL("https://www.airbnb.com/"),
      GURL("https://accounts.google.com/o/oauth2/iframe"), true));
  EXPECT_FALSE(IsWhitelistedCookieException(GURL("https://www.mozilla.org/"),
      GURL("https://www.googletagmanager.com/gtm.js"), true));
  EXPECT_FALSE(IsWhitelistedCookieException(GURL("https://www.airbnb.com/"),
      GURL("https://accounts.google.com/o/oauth2/iframe"), false));
}

}  // namespace
