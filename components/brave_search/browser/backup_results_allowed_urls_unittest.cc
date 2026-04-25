/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/backup_results_allowed_urls.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_search {

TEST(BackupResultsAllowedURLsTest, ValidGoogleDomains) {
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://google.com")));
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://google.co.uk")));
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://google.com.au")));
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://google.fr")));
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://google.de")));
  EXPECT_TRUE(IsBackupResultURLAllowed(GURL("https://www.google.de")));
  EXPECT_TRUE(
      IsBackupResultURLAllowed(GURL("https://www.google.co.uk/search")));
}

TEST(BackupResultsAllowedURLsTest, InvalidDomains) {
  EXPECT_FALSE(
      IsBackupResultURLAllowed(GURL("http://google.com")));  // Not HTTPS
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("https://fake-google.com")));
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("https://google.invalid")));
  EXPECT_FALSE(
      IsBackupResultURLAllowed(GURL("https://google")));  // Missing TLD
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("https://googles.com")));
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("https://googles.com/search")));
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("about:blank")));
  EXPECT_FALSE(IsBackupResultURLAllowed(GURL("https://brave.com")));
}

}  // namespace brave_search
