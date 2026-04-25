/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"

#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

TEST(BraveAdsDayPartsDatabaseTableTest, InsertEmptyDayparts) {
  // Arrange
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Dayparts database_table;

  // Act
  database_table.Insert(mojom_db_transaction, /*dayparts=*/{});

  // Assert
  EXPECT_THAT(mojom_db_transaction->actions, ::testing::IsEmpty());
}

TEST(BraveAdsDayPartsDatabaseTableTest, InsertDayparts) {
  // Arrange
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Dayparts database_table;

  // Act
  database_table.Insert(mojom_db_transaction,
                        /*dayparts=*/{{"foo", {CreativeDaypartInfo{}}}});

  // Assert
  EXPECT_THAT(mojom_db_transaction->actions, ::testing::SizeIs(1));
}

}  // namespace brave_ads::database::table
