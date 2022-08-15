/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_constants.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/legacy_migration/database/database_constants.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_unittest_util.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDatabaseMigrationTest : public UnitTestBase,
                                    public ::testing::WithParamInterface<int> {
 protected:
  BatAdsDatabaseMigrationTest() = default;

  ~BatAdsDatabaseMigrationTest() override = default;

  void SetUpMocks() override {
    const std::string database_filename =
        base::StringPrintf("database_schema_%d.sqlite", GetSchemaVersion());
    CopyFileFromTestPathToTempPath(database_filename, kDatabaseFilename);
  }

  int GetSchemaVersion() { return GetParam() + 1; }
};

TEST_P(BatAdsDatabaseMigrationTest, MigrateFromSchema) {
  // Arrange
  const CreativeAdInfo creative_ad = BuildCreativeAd();
  const AdEventInfo ad_event = BuildAdEvent(
      creative_ad, AdType::kNotificationAd, ConfirmationType::kViewed, Now());
  const TextEmbeddingEventInfo& text_embedding_event =
      BuildTextEmbeddingEvent();

  // Act
  LogAdEvent(ad_event, [=](const bool success) {
    EXPECT_TRUE(success) << "Failed to migrate database from schema version "
                         << GetSchemaVersion() << " to schema version "
                         << database::kVersion;
  });
  LogTextEmbeddingHtmlEvent(
      text_embedding_event.embedding, text_embedding_event.hashed_key,
      [=](const bool success) {
        EXPECT_TRUE(success)
            << "Failed to migrate database from schema version "
            << GetSchemaVersion() << " to schema version "
            << database::kVersion;
        if (success) {
          GetTextEmbeddingEventsFromDatabase(
              [=](const bool success, const TextEmbeddingHtmlEventList&
                                          text_embedding_html_events) {
                EXPECT_TRUE(success)
                    << "Failed to migrate database from schema version "
                    << GetSchemaVersion() << " to schema version "
                    << database::kVersion;
                // Assert
                ASSERT_EQ(1ul, text_embedding_html_events.size());
                ASSERT_EQ(text_embedding_event.hashed_key,
                          text_embedding_html_events[0].hashed_key);
              });
        }
      });
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
