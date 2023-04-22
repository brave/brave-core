/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::notification_ads {

class BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest
    : public UnitTestBase {
 protected:
  MinimumWaitTimePermissionRule permission_rule_;
};

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, 5);

  RecordAdEvent(AdType::kNotificationAd, ConfirmationType::kServed);

  // Act
  AdvanceClockBy(base::Minutes(12));

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       DoNotAllowAdIfExceedsCap) {
  // Arrange
  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, 5);

  RecordAdEvent(AdType::kNotificationAd, ConfirmationType::kServed);

  // Act
  AdvanceClockBy(base::Minutes(12) - base::Milliseconds(1));

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads::notification_ads
