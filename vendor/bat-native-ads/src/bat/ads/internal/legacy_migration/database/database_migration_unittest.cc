/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_constants.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/legacy_migration/database/database_constants.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDatabaseMigrationTest : public UnitTestBase,
                                    public ::testing::WithParamInterface<int> {
 protected:
  void SetUpMocks() override {
    const std::string database_filename =
        base::StringPrintf("database_schema_%d.sqlite", GetSchemaVersion());
    CopyFileFromTestPathToTempPath(database_filename, kDatabaseFilename);
  }

  static int GetSchemaVersion() { return GetParam() + 1; }
};

TEST_P(BatAdsDatabaseMigrationTest, MigrateFromSchema) {
  // Arrange
  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());

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

std::string TestParamToString(::testing::TestParamInfo<int> param_info) {
  return base::StringPrintf("%d_to_%d", param_info.param + 1,
                            database::kVersion);
}

INSTANTIATE_TEST_SUITE_P(BatAdsDatabaseMigration,
                         BatAdsDatabaseMigrationTest,
                         testing::Range(0, database::kVersion),
                         TestParamToString);

}  // namespace ads
