/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::database::table {

class BatAdsTextEmbeddingHtmlEventsDatabaseTableTest : public UnitTestBase {};

TEST(BatAdsTextEmbeddingHtmlEventsDatabaseTableTest, TableName) {
  // Arrange
  const TextEmbeddingHtmlEvents database_table;

  // Act
  const std::string table_name = database_table.GetTableName();

  // Assert
  const std::string expected_table_name = "text_embedding_html_events";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace brave_ads::database::table
