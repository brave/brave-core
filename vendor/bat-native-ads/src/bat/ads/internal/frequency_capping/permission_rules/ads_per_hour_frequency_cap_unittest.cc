/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsPerHourFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsAdsPerHourFrequencyCapTest() = default;

  ~BatAdsAdsPerHourFrequencyCapTest() override = default;
};

TEST_F(BatAdsAdsPerHourFrequencyCapTest, AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest, AlwaysAllowAdOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kViewed,
                 ads_per_hour);

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest, AlwaysAllowAdOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  const int64_t ads_per_hour = 5;
  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kViewed,
                 ads_per_hour);

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kViewed,
                 ads_per_hour - 1);

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest, AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kViewed,
                 ads_per_hour);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsAdsPerHourFrequencyCapTest, DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  const int64_t ads_per_hour = 5;

  AdsClientHelper::Get()->SetInt64Pref(prefs::kAdsPerHour, ads_per_hour);

  RecordAdEvents(AdType::kAdNotification, ConfirmationType::kViewed,
                 ads_per_hour);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  AdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
