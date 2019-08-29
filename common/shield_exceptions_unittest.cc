/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"

#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"

namespace {

typedef testing::Test BraveShieldsExceptionsTest;
using brave::IsWhitelistedFingerprintingException;

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

  // Tests for sandbox URLs
  EXPECT_TRUE(IsWhitelistedFingerprintingException(
      GURL("https://sandbox.uphold.com"),
      GURL("https://netverify.com/iframe")));
  EXPECT_TRUE(IsWhitelistedFingerprintingException(
      GURL("https://sandbox.uphold.com/"),
      GURL("https://netverify.com")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("http://sandbox.uphold.com/"),
      GURL("https://uphold.netverify.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://sandbox.uphold.com/"),
      GURL("http://netverify.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://netverify.com/iframe"),
      GURL("https://sandbox.uphold.com/")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://sandbox.uphold.com/"),
      GURL("https://uphold.netverify.com/iframe")));
  EXPECT_FALSE(IsWhitelistedFingerprintingException(
      GURL("https://www.sandbox.uphold.com/"),
      GURL("https://netverify.com/iframe")));
}

}  // namespace
