/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"

#include <cstdint>

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::notification_ads {

class BatAdsNotificationAdsPerHourPermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest, AlwaysAllowAdOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const int64_t ads_per_hour = 5;

  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 ads_per_hour);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest, AlwaysAllowAdOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  const int64_t ads_per_hour = 5;
  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 ads_per_hour);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 ads_per_hour - 1);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 ads_per_hour);

  AdvanceClockBy(base::Hours(1));

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNotificationAdsPerHourPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::GetInstance()->SetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 ads_per_hour);

  AdvanceClockBy(base::Hours(1) - base::Seconds(1));

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads::notification_ads
