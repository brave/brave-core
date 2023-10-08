/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

TEST(BraveAdsCreativeNewTabPageAdWallpapersDatabaseTableTest, GetTableName) {
  // Arrange
  const CreativeNewTabPageAdWallpapers database_table;

  // Act & Assert
  EXPECT_EQ("creative_new_tab_page_ad_wallpapers",
            database_table.GetTableName());
}

}  // namespace brave_ads::database::table
