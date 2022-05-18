/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/ads_per_hour_permission_rule.h"

#include <cstdint>

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsPerHourPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsAdsPerHourPermissionRuleTest() = default;

  ~BatAdsAdsPerHourPermissionRuleTest() override = default;
};

TEST_F(BatAdsAdsPerHourPermissionRuleTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourPermissionRuleTest, AlwaysAllowAdOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kServed,
                 ads_per_hour);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourPermissionRuleTest, AlwaysAllowAdOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  const int64_t ads_per_hour = 5;
  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kServed,
                 ads_per_hour);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourPermissionRuleTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kServed,
                 ads_per_hour - 1);

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kServed,
                 ads_per_hour);

  FastForwardClockBy(base::Hours(1));

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kServed,
                 ads_per_hour);

  FastForwardClockBy(base::Minutes(59));

  // Act
  AdsPerHourPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
