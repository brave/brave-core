/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

TEST(BraveAdsSegmentsDatabaseTableTest, InsertEmptySegments) {
  // Arrange
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Segments database_table;

  // Act
  database_table.Insert(mojom_db_transaction, /*segments=*/{});

  // Assert
  EXPECT_THAT(mojom_db_transaction->actions, ::testing::IsEmpty());
}

TEST(BraveAdsSegmentsDatabaseTableTest, InsertSegments) {
  // Arrange
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Segments database_table;

  // Act
  database_table.Insert(mojom_db_transaction,
                        /*segments=*/{{"foo", {"bar"}}});

  // Assert
  EXPECT_THAT(mojom_db_transaction->actions, ::testing::SizeIs(1));
}

}  // namespace brave_ads::database::table
