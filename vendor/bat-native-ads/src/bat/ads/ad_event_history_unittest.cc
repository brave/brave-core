/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_event_history.h"

#include <cstdint>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdEventHistoryTest : public UnitTestBase {
 protected:
  BatAdsAdEventHistoryTest() = default;

  ~BatAdsAdEventHistoryTest() override = default;

  uint64_t Now() {
    const uint64_t timestamp =
        static_cast<uint64_t>(base::Time::Now().ToDoubleT());
    return timestamp;
  }

  void RecordAdEvent(const AdType& ad_type,
                     const ConfirmationType& confirmation_type) {
    const std::string ad_type_as_string = std::string(ad_type);

    const std::string confirmation_type_as_string =
        std::string(confirmation_type);

    const uint64_t timestamp = Now();

    ad_event_history_.Record(ad_type_as_string, confirmation_type_as_string,
        timestamp);
  }

  std::vector<uint64_t> GetAdEvent(const AdType& ad_type,
                                   const ConfirmationType& confirmation_type) {
    const std::string ad_type_as_string = std::string(ad_type);

    const std::string confirmation_type_as_string =
        std::string(confirmation_type);

    return ad_event_history_.Get(ad_type_as_string,
                                 confirmation_type_as_string);
  }

  AdEventHistory ad_event_history_;
};

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForNewType) {
  // Arrange
  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Act
  const std::vector<uint64_t> history =
      GetAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  const uint64_t timestamp = Now();
  const std::vector<uint64_t> expected_history = {timestamp};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForExistingType) {
  // Arrange
  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);
  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Act
  const std::vector<uint64_t> history =
      GetAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  const uint64_t timestamp = Now();
  const std::vector<uint64_t> expected_history = {timestamp, timestamp};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForMultipleTypes) {
  // Arrange
  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);
  RecordAdEvent(AdType::kNewTabPageAd, ConfirmationType::kClicked);

  // Act
  const std::vector<uint64_t> history =
      GetAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  const uint64_t timestamp = Now();
  const std::vector<uint64_t> expected_history = {timestamp};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, PurgeHistoryOlderThan) {
  // Arrange
  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  FastForwardClockBy(base::TimeDelta::FromDays(1));

  RecordAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Act
  const std::vector<uint64_t> history =
      GetAdEvent(AdType::kAdNotification, ConfirmationType::kViewed);

  // Assert
  const uint64_t timestamp = Now();
  const std::vector<uint64_t> expected_history = {timestamp};
  EXPECT_EQ(expected_history, history);
}

}  // namespace ads
