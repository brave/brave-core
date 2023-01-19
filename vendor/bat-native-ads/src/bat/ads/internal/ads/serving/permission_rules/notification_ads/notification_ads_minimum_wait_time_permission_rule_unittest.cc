/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/notification_ads/notification_ads_minimum_wait_time_permission_rule.h"

#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::notification_ads {

class BatAdsNotificationAdsMinimumWaitTimePermissionRuleTest
    : public UnitTestBase {};

TEST_F(BatAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, 5);

  RecordAdEvent(AdType::kNotificationAd, ConfirmationType::kServed);

  AdvanceClockBy(base::Minutes(12));

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsMinimumWaitTimePermissionRuleTest,
       DoNotAllowAdIfExceedsCap) {
  // Arrange
  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, 5);

  RecordAdEvent(AdType::kNotificationAd, ConfirmationType::kServed);

  AdvanceClockBy(base::Minutes(12) - base::Seconds(1));

  // Act
  MinimumWaitTimePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads::notification_ads
