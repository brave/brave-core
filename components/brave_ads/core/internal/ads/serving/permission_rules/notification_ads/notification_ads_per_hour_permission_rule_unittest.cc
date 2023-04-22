/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ads_per_hour_permission_rule.h"

#include <cstdint>

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::notification_ads {

class BraveAdsNotificationAdsPerHourPermissionRuleTest : public UnitTestBase {
 protected:
  AdsPerHourPermissionRule permission_rule_;
};

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       AlwaysAllowAdOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const int ads_per_hour = kDefaultAdsPerHour.Get();

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                                 ads_per_hour);

  // Act
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ ads_per_hour);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest, AlwaysAllowAdOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  const int ads_per_hour = kDefaultAdsPerHour.Get();

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                                 ads_per_hour);

  // Act
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ ads_per_hour);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int ads_per_hour = kDefaultAdsPerHour.Get();

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                                 ads_per_hour);

  // Act
  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ ads_per_hour - 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int ads_per_hour = kDefaultAdsPerHour.Get();

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                                 ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ ads_per_hour);

  // Act
  AdvanceClockBy(base::Hours(1));

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsNotificationAdsPerHourPermissionRuleTest,
       DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  const int ads_per_hour = kDefaultAdsPerHour.Get();

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour,
                                 ads_per_hour);

  RecordAdEvents(AdType::kNotificationAd, ConfirmationType::kServed,
                 /*count*/ ads_per_hour);

  // Act
  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads::notification_ads
