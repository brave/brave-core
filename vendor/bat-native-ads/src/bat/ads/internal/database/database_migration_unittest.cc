/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/database/database_version.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDatabaseMigrationTest : public UnitTestBase,
                                    public ::testing::WithParamInterface<int> {
 protected:
  BatAdsDatabaseMigrationTest() = default;

  ~BatAdsDatabaseMigrationTest() override = default;

  void SetUp() override {
    const std::string source_filename =
        base::StringPrintf("database_schema_%d.sqlite", GetSchemaVersion());

    ASSERT_TRUE(
        CopyFileFromTestPathToTempDir(source_filename, "database.sqlite"))
        << "Failed to copy " << source_filename;

    UnitTestBase::SetUpForTesting(/* integration_test */ false);
  }

  int GetSchemaVersion() { return GetParam() + 1; }

  AdEventInfo GetAdEvent() {
    AdEventInfo ad_event;

    ad_event.type = AdType::kAdNotification;
    ad_event.confirmation_type = ConfirmationType::kViewed;
    ad_event.uuid = "90ca9157-9c1a-484a-b52b-86e3dfea7d9c";
    ad_event.campaign_id = "b6c52182-dcbc-4932-aafa-34e6d2420383";
    ad_event.creative_set_id = "93af82a4-f929-4eb2-8af9-f5f2cafc3ed0";
    ad_event.creative_instance_id = "59b43768-40ad-4d3e-8a04-61431a75cd24";
    ad_event.advertiser_id = "46afa495-cf5b-405b-94de-25cff55f05e7";
    ad_event.timestamp = NowAsTimestamp();

    return ad_event;
  }
};

TEST_P(BatAdsDatabaseMigrationTest, MigrateFromSchema) {
  // Arrange
  const AdEventInfo ad_event = GetAdEvent();

  // Act
  LogAdEvent(ad_event, [=](const bool success) {
    EXPECT_TRUE(success) << "Failed to migrate database from schema "
                         << GetSchemaVersion() << " to schema "
                         << database::version();
  });

  // Assert
}

std::string TestParamToString(::testing::TestParamInfo<int> param_info) {
  return base::StringPrintf("%d_to_%d", param_info.param + 1,
                            database::version());
}

INSTANTIATE_TEST_SUITE_P(BatAdsDatabaseMigration,
                         BatAdsDatabaseMigrationTest,
                         testing::Range(0, database::version()),
                         TestParamToString);

}  // namespace ads
