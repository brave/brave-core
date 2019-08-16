/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"

#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

namespace {

typedef testing::Test BraveShieldsExceptionsTest;
using content_settings::IsWhitelistedCookieException;
using brave::IsWhitelistedFingerprintingException;

TEST_F(BraveShieldsExceptionsTest, IsWhitelistedCookieException) {
  // Cookie exceptions for Google auth domains
  EXPECT_TRUE(IsWhitelistedCookieException(GURL("https://www.airbnb.com/"),
      GURL("https://accounts.google.com/o/oauth2/iframe"), true));
  EXPECT_FALSE(IsWhitelistedCookieException(GURL("https://www.mozilla.org/"),
      GURL("https://www.googletagmanager.com/gtm.js"), true));
  EXPECT_FALSE(IsWhitelistedCookieException(GURL("https://www.airbnb.com/"),
      GURL("https://accounts.google.com/o/oauth2/iframe"), false));
}

TEST_F(BraveShieldsExceptionsTest, IsWhitelistedFingerprintingException) {
  EXPECT_TRUE(IsWhitelistedFingerprintingException(
      GURL("https://uphold.com"),
      GURL("https://uphold.netverify.com/iframe")));
  EXPECT_TRUE(IsWhitelistedFingerprintingException(
      GURL("https://uphold.com/"),
      GURL("https://uphold.netverify.com")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("http://uphold.com/"),
      GURL("https://uphold.netverify.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://uphold.com/"),
      GURL("http://uphold.netverify.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://uphold.netverify.com/iframe"),
      GURL("https://uphold.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://uphold.com/"),
      GURL("https://netverify.com/iframe")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://www.uphold.com/"),
      GURL("https://uphold.netverify.com/iframe")));
}

}  // namespace
