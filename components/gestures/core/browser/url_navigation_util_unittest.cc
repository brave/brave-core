// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/url_navigation_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace gestures {

TEST(UrlNavigationUtilTest, ShouldAllowHttpUrl) {
  EXPECT_TRUE(IsGestureAllowedToNavigateToUrl(GURL("http://foo.com/")));
}

TEST(UrlNavigationUtilTest, ShouldAllowHttpsUrl) {
  EXPECT_TRUE(IsGestureAllowedToNavigateToUrl(GURL("https://foo.com/")));
}

TEST(UrlNavigationUtilTest, ShouldAllowBraveSchemeUrl) {
  EXPECT_TRUE(IsGestureAllowedToNavigateToUrl(GURL("brave://settings/")));
}

TEST(UrlNavigationUtilTest, ShouldAllowAboutBlankUrl) {
  EXPECT_TRUE(IsGestureAllowedToNavigateToUrl(GURL("about:blank")));
}

TEST(UrlNavigationUtilTest, ShouldRejectJavascriptUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("javascript:alert(1)")));
}

TEST(UrlNavigationUtilTest, ShouldRejectFileUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("file:///foo.txt")));
}

TEST(UrlNavigationUtilTest, ShouldRejectDataUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("data:text/plain,hello")));
}

TEST(UrlNavigationUtilTest, ShouldRejectFtpUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("ftp://foo.com/")));
}

TEST(UrlNavigationUtilTest, ShouldRejectChromeSchemeUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("chrome://settings")));
}

TEST(UrlNavigationUtilTest, ShouldRejectAboutUrlsOtherThanAboutBlank) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("about:settings")));
}

TEST(UrlNavigationUtilTest, ShouldRejectBlobUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("blob:https://foo.com/x")));
}

TEST(UrlNavigationUtilTest, ShouldRejectInvalidUrl) {
  EXPECT_FALSE(IsGestureAllowedToNavigateToUrl(GURL("not a url")));
}

TEST(UrlNavigationUtilTest, ShouldAllowPageSuppliedHttpUrl) {
  EXPECT_TRUE(IsPageSuppliedUrlAllowedToOpen(GURL("http://foo.com/")));
}

TEST(UrlNavigationUtilTest, ShouldAllowPageSuppliedHttpsUrl) {
  EXPECT_TRUE(IsPageSuppliedUrlAllowedToOpen(GURL("https://foo.com/")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedBraveSchemeUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("brave://settings/")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedBraveSchemeSubpageUrl) {
  EXPECT_FALSE(
      IsPageSuppliedUrlAllowedToOpen(GURL("brave://settings/extensions")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedBraveWalletUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("brave://wallet/")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedAboutBlankUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("about:blank")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedJavascriptUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("javascript:alert(1)")));
}

TEST(UrlNavigationUtilTest, ShouldRejectPageSuppliedFileUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("file:///foo.txt")));
}

TEST(UrlNavigationUtilTest, ShouldRejectInvalidPageSuppliedUrl) {
  EXPECT_FALSE(IsPageSuppliedUrlAllowedToOpen(GURL("not a url")));
}

}  // namespace gestures
