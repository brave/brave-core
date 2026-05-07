/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_title_bar_view.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

using FocusModeTitleBarViewTest = ::testing::Test;

TEST_F(FocusModeTitleBarViewTest, StripsHttpsAndTrivialSubdomain) {
  EXPECT_EQ(FocusModeTitleBarView::FormatDomain(
                GURL("https://www.example.com/some/path?q=1")),
            u"example.com");
}

TEST_F(FocusModeTitleBarViewTest, KeepsNonTrivialSubdomain) {
  EXPECT_EQ(
      FocusModeTitleBarView::FormatDomain(GURL("https://docs.example.com")),
      u"docs.example.com");
}

TEST_F(FocusModeTitleBarViewTest, KeepsHttpScheme) {
  EXPECT_EQ(
      FocusModeTitleBarView::FormatDomain(GURL("http://insecure.example.com/")),
      u"http://insecure.example.com");
}

TEST_F(FocusModeTitleBarViewTest, RewritesChromeSchemeToBrave) {
  EXPECT_EQ(FocusModeTitleBarView::FormatDomain(GURL("chrome://settings")),
            u"brave://settings");
}

TEST_F(FocusModeTitleBarViewTest, TrimsAfterHostAndRewritesChromeScheme) {
  EXPECT_EQ(
      FocusModeTitleBarView::FormatDomain(GURL("chrome://settings/passwords")),
      u"brave://settings");
}

TEST_F(FocusModeTitleBarViewTest, DoesNotRewriteChromeUntrustedScheme) {
  const std::u16string formatted =
      FocusModeTitleBarView::FormatDomain(GURL("chrome-untrusted://print"));
  EXPECT_NE(formatted.find(u"chrome-untrusted"), std::u16string::npos);
  EXPECT_EQ(formatted.find(u"brave"), std::u16string::npos);
}

TEST_F(FocusModeTitleBarViewTest, InvalidUrlReturnsEmpty) {
  ASSERT_FALSE(GURL().is_valid());
  EXPECT_EQ(FocusModeTitleBarView::FormatDomain(GURL()), u"");
}

}  // namespace
