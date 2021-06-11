/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCreativeAdNotificationsDatabaseTableIntegrationTest
    : public UnitTestBase {
 protected:
  BatAdsCreativeAdNotificationsDatabaseTableIntegrationTest() = default;

  ~BatAdsCreativeAdNotificationsDatabaseTableIntegrationTest() override =
      default;

  void SetUp() override {
    UnitTestBase::SetUpForTesting(/* integration_test */ true);
  }
};

TEST_F(BatAdsCreativeAdNotificationsDatabaseTableIntegrationTest,
       GetCreativeAdNotificationsFromCatalogEndpoint) {
  // Arrange
  const URLEndpoints endpoints = {
      {"/v8/catalog", {{net::HTTP_OK, "/catalog.json"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  InitializeAds();

  // Act

  // Assert
  const std::vector<std::string> segments = {"Technology & Computing"};

  database::table::CreativeAdNotifications creative_ad_notifications;
  creative_ad_notifications.GetForSegments(
      segments,
      [](const Result result, const SegmentList& segments,
         const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_EQ(Result::SUCCESS, result);
        EXPECT_EQ(2UL, creative_ad_notifications.size());
      });
}

}  // namespace ads
