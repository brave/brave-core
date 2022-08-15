/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

#include <vector>

#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeNotificationAdsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  BatAdsCreativeNotificationAdsDatabaseTableIntegrationTest() = default;

  ~BatAdsCreativeNotificationAdsDatabaseTableIntegrationTest() override =
      default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* is_integration_test */ true);
  }

  void SetUpMocks() override {
    const URLResponseMap url_responses = {
        {"/v9/catalog", {{net::HTTP_OK, "/catalog.json"}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }
};

TEST_F(BatAdsCreativeNotificationAdsDatabaseTableIntegrationTest,
       GetCreativeNotificationAdsFromCatalogResponse) {
  // Arrange

  // Act

  // Assert
  const std::vector<std::string> segments = {"technology & computing"};

  database::table::CreativeNotificationAds creative_ads;
  creative_ads.GetForSegments(
      segments, [](const bool success, const SegmentList& segments,
                   const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(success);
        EXPECT_EQ(2UL, creative_ads.size());
      });
}

}  // namespace ads
