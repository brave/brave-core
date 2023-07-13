/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDatabaseMigrationTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<int> {
 protected:
  void SetUpMocks() override {
    const std::string database_filename =
        base::StringPrintf("database_schema_%d.sqlite", GetSchemaVersion());
    CopyFileFromTestPathToTempPath(database_filename, kDatabaseFilename);
  }

  static int GetSchemaVersion() { return GetParam() + 1; }
};

TEST_P(BraveAdsDatabaseMigrationTest, MigrateFromSchema) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ true);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());

  // Act
  LogAdEvent(ad_event,
             base::BindOnce(
                 [](const int schema_version, const bool success) {
                   EXPECT_TRUE(success)
                       << "Failed to migrate database from schema version "
                       << schema_version << " to schema version "
                       << database::kVersion;
                 },
                 GetSchemaVersion()));

  // Assert
}

std::string TestParamToString(::testing::TestParamInfo<int> test_param) {
  return base::StringPrintf("%d_to_%d", test_param.param + 1,
                            database::kVersion);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsDatabaseMigrationTest,
                         testing::Range(0, database::kVersion),
                         TestParamToString);

}  // namespace brave_ads
