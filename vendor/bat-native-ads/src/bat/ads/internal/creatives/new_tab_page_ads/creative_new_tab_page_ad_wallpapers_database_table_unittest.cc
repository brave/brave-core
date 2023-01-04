/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::database::table {

TEST(BatAdsCreativeNewTabPageAdWallpapersDatabaseTableTest, TableName) {
  // Arrange
  const CreativeNewTabPageAdWallpapers database_table;

  // Act
  const std::string table_name = database_table.GetTableName();

  // Assert
  const std::string expected_table_name = "creative_new_tab_page_ad_wallpapers";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads::database::table
