/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_ads_database_table.h"

#include <memory>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeAdsDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsCreativeAdsDatabaseTableTest()
      : database_table_(std::make_unique<database::table::CreativeAds>()) {
  }

  ~BatAdsCreativeAdsDatabaseTableTest() override = default;

  std::unique_ptr<database::table::CreativeAds> database_table_;
};

TEST_F(BatAdsCreativeAdsDatabaseTableTest,
    TableName) {
  // Arrange

  // Act
  const std::string table_name = database_table_->GetTableName();

  // Assert
  const std::string expected_table_name = "creative_ads";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
