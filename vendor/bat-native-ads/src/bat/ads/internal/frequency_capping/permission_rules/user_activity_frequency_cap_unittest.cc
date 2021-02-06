/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsUserActivityFrequencyCapTest() = default;

  ~BatAdsUserActivityFrequencyCapTest() override = default;
};

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfActivityWasReportedForTwoTypes) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfActivityWasReportedForTwoOfTheSameType) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfActivityWasReportedForMoreThanTwoTypes) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfActivityWasReportedForMoreThanTwoOfTheSameType) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       AllowAdIfDuplicateActivityWasReportedForMoreThanTwoTypes) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       DoNotAllowAdIfActivityWasReportedForLessThanTwoTypes) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       DoNotAllowAdIfNoActivityWasReported) {
  // Arrange

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsUserActivityFrequencyCapTest,
       DoNotAllowAdIfActivityWasReportedInThePreviousHour) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  UserActivityFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
