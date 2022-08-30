/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposits_database_table.h"

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace database {
namespace table {

TEST(BatAdsDepositsDatabaseTableTest, TableName) {
  // Arrange
  Deposits database_table;

  // Act
  const std::string& table_name = database_table.GetTableName();

  // Assert
  const std::string expected_table_name = "deposits";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace table
}  // namespace database
}  // namespace ads
