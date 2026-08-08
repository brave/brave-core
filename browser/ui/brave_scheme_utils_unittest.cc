// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_scheme_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(BraveSchemeUtilsTest, ReplaceChromeToBraveSchemeGURL) {
  EXPECT_EQ(GURL("brave://settings/"), brave_utils::ReplaceChromeToBraveScheme(
                                           GURL("chrome://settings/")));

  // Everything but the scheme is preserved.
  EXPECT_EQ(GURL("brave://settings/clearBrowserData?foo=bar#ref"),
            brave_utils::ReplaceChromeToBraveScheme(
                GURL("chrome://settings/clearBrowserData?foo=bar#ref")));

  // "chrome-extension" must not be mistaken for a "chrome" scheme match.
  for (const char* spec : {"https://search.brave.com/", "brave://settings/",
                           "chrome-extension://abcdefghijklmnop/page.html"}) {
    const GURL url(spec);
    EXPECT_EQ(url, brave_utils::ReplaceChromeToBraveScheme(url)) << spec;
  }
}

TEST(BraveSchemeUtilsTest, ReplaceChromeToBraveScheme) {
  std::u16string url_string = u"chrome://settings";
  EXPECT_TRUE(brave_utils::ReplaceChromeToBraveScheme(url_string));
  EXPECT_EQ(url_string, u"brave://settings");

  url_string = u"chrome://flags";
  EXPECT_TRUE(brave_utils::ReplaceChromeToBraveScheme(url_string));
  EXPECT_EQ(url_string, u"brave://flags");

  url_string = u"https://search.brave.com";
  EXPECT_FALSE(brave_utils::ReplaceChromeToBraveScheme(url_string));
  EXPECT_EQ(url_string, u"https://search.brave.com");
}
