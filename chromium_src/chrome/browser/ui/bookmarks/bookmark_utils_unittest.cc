// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chrome {

TEST(BraveBookmarkUtilsTest, ChromeUISchemeIsDisplayedAsBrave) {
  EXPECT_EQ(u"brave://settings",
            FormatBookmarkURLForDisplay(GURL("chrome://settings/")));

  // Only the scheme changes; path and query are preserved.
  EXPECT_EQ(u"brave://settings/clearBrowserData?foo=bar",
            FormatBookmarkURLForDisplay(
                GURL("chrome://settings/clearBrowserData?foo=bar")));
}

TEST(BraveBookmarkUtilsTest, BraveUISchemeIsDisplayedUnchanged) {
  EXPECT_EQ(u"brave://settings",
            FormatBookmarkURLForDisplay(GURL("brave://settings/")));
}

TEST(BraveBookmarkUtilsTest, OtherSchemesAreNotRebranded) {
  EXPECT_EQ(u"https://example.com",
            FormatBookmarkURLForDisplay(GURL("https://example.com/")));

  // "chrome-extension" must not be mistaken for a "chrome" scheme match.
  EXPECT_EQ(u"chrome-extension://abcdefghijklmnop/page.html",
            FormatBookmarkURLForDisplay(
                GURL("chrome-extension://abcdefghijklmnop/page.html")));
}

}  // namespace chrome
