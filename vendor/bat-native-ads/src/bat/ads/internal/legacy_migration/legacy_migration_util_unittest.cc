/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/legacy_migration_util.h"

#include <cstdint>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsLegacyMigrationUtilTest, MigrateTimestamp) {
  // Arrange
  const uint64_t timestamp = 13250441166;

  // Act
  const uint64_t migrated_timestamp = MigrateTimestampToDoubleT(timestamp);

  // Assert
  EXPECT_EQ(1605967566UL, migrated_timestamp);
}

TEST(BatAdsLegacyMigrationUtilTest, DoNotMigrateAlreadyMigratedTimestamp) {
  // Arrange
  const uint64_t timestamp = 1605967566;

  // Act
  const uint64_t migrated_timestamp = MigrateTimestampToDoubleT(timestamp);

  // Assert
  EXPECT_EQ(1605967566UL, migrated_timestamp);
}

}  // namespace ads
