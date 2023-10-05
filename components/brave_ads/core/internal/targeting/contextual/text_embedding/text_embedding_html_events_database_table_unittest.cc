/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events_database_table.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsTextEmbeddingHtmlEventsDatabaseTableTest : public UnitTestBase {};

TEST(BraveAdsTextEmbeddingHtmlEventsDatabaseTableTest, GetTableName) {
  // Arrange
  const TextEmbeddingHtmlEvents database_table;

  // Act & Assert
  EXPECT_EQ("text_embedding_html_events", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
