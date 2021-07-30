/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/brave_search_fallback_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_search {

TEST(BraveSearchFallbackHost, GetBackupResultURL) {
  GURL base_url("https://www.google.com/search/");
  ASSERT_EQ(
      BraveSearchFallbackHost::GetBackupResultURL(base_url, "test", "en", "ca",
                                                  "32,32", true),
      GURL("https://www.google.com/search/?q=test&hl=en&gl=ca&safe=active"));
}

TEST(BraveSearchFallbackHost, GetBackupResultURLNoLang) {
  GURL base_url("https://www.google.com/search/");
  ASSERT_EQ(BraveSearchFallbackHost::GetBackupResultURL(base_url, "test", "",
                                                        "ca", "32,32", true),
            GURL("https://www.google.com/search/?q=test&gl=ca&safe=active"));
}

TEST(BraveSearchFallbackHost, GetBackupResultURLNoCountry) {
  GURL base_url("https://www.google.com/search/");
  ASSERT_EQ(BraveSearchFallbackHost::GetBackupResultURL(base_url, "test", "en",
                                                        "", "32,32", true),
            GURL("https://www.google.com/search/?q=test&hl=en&safe=active"));
}

TEST(BraveSearchFallbackHost, GetBackupResultURLNoFilter) {
  GURL base_url("https://www.google.com/search/");
  ASSERT_EQ(BraveSearchFallbackHost::GetBackupResultURL(base_url, "test", "en",
                                                        "ca", "32,32", false),
            GURL("https://www.google.com/search/?q=test&hl=en&gl=ca"));
}

TEST(BraveSearchFallbackHost, GetBackupResultURLMinimal) {
  GURL base_url("https://www.google.com/search/");
  ASSERT_EQ(BraveSearchFallbackHost::GetBackupResultURL(base_url, "test", "",
                                                        "", "", false),
            GURL("https://www.google.com/search/?q=test"));
}

}  // namespace brave_search
