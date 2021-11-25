/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_new_tab_page_ad_wallpapers_database_table.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeNewTabPageAdWallpapersDatabaseTableTest
    : public UnitTestBase {
 protected:
  BatAdsCreativeNewTabPageAdWallpapersDatabaseTableTest() = default;

  ~BatAdsCreativeNewTabPageAdWallpapersDatabaseTableTest() override = default;
};

TEST_F(BatAdsCreativeNewTabPageAdWallpapersDatabaseTableTest, TableName) {
  // Arrange

  // Act
  database::table::CreativeNewTabPageAdWallpapers database_table;
  const std::string& table_name = database_table.GetTableName();

  // Assert
  const std::string& expected_table_name =
      "creative_new_tab_page_ad_wallpapers";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
