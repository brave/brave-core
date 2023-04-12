/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::database::table {

TEST(BatAdsCreativeAdsDatabaseTableTest, TableName) {
  // Arrange
  const CreativeAds database_table;

  // Act

  // Assert
  EXPECT_EQ("creative_ads", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
