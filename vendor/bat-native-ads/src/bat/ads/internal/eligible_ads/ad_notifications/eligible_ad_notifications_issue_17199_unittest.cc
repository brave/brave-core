/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

#include <memory>
#include <string>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEligibleAdNotificationsIssue17199Test : public UnitTestBase {
 protected:
  BatAdsEligibleAdNotificationsIssue17199Test() = default;

  ~BatAdsEligibleAdNotificationsIssue17199Test() override = default;

  void SetUp() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempDir("database_issue_17199.sqlite",
                                              "database.sqlite"));

    ASSERT_TRUE(CopyFileFromTestPathToTempDir("client_issue_17199.json",
                                              "client.json"));

    UnitTestBase::SetUpForTesting(/* integration_test */ false);
  }

  void RecordUserActivityEvents() {
    UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
    UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  }
};

TEST_F(BatAdsEligibleAdNotificationsIssue17199Test, GetEligibleAds) {
  // Arrange
  AdvanceClock(TimeFromDateString("4 July 2021"));

  RecordUserActivityEvents();

  // Act
  ad_targeting::geographic::SubdivisionTargeting subdivision_targeting;
  resource::AntiTargeting anti_targeting_resource;
  ad_notifications::EligibleAds eligible_ads(&subdivision_targeting,
                                             &anti_targeting_resource);

  eligible_ads.GetForSegments(
      {"technology & computing-computing"},
      [](const bool success,
         const CreativeAdNotificationList& creative_ad_notifications) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(creative_ad_notifications.empty());
      });

  // Assert
}

}  // namespace ads
