/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ad_event_history.h"

#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

constexpr char kID1[] = "26330bea-9b8c-4cd3-b04a-1c74cbdf701e";
constexpr char kID2[] = "5b2f108c-e176-4a3e-8e7c-fe67fb3db518";

}  // namespace

class BatAdsAdEventHistoryTest : public UnitTestBase {
 protected:
  void RecordAdEvent(const std::string& id,
                     const AdType& ad_type,
                     const ConfirmationType& confirmation_type) {
    ad_event_history_.RecordForId(id, ad_type.ToString(),
                                  confirmation_type.ToString(), Now());
  }

  std::vector<base::Time> GetAdEventHistory(
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) {
    return ad_event_history_.Get(ad_type.ToString(),
                                 confirmation_type.ToString());
  }

  AdEventHistory ad_event_history_;
};

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForNewType) {
  // Arrange
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  const std::vector<base::Time> expected_history = {Now()};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForExistingType) {
  // Arrange
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  const std::vector<base::Time> expected_history = {Now(), Now()};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForMultipleIds) {
  // Arrange
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  RecordAdEvent(kID2, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  const std::vector<base::Time> expected_history = {Now(), Now()};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, RecordAdEventForMultipleTypes) {
  // Arrange
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);
  RecordAdEvent(kID1, AdType::kNewTabPageAd, ConfirmationType::kClicked);

  // Act
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  const std::vector<base::Time> expected_history = {Now()};
  EXPECT_EQ(expected_history, history);
}

TEST_F(BatAdsAdEventHistoryTest, PurgeHistoryOlderThan) {
  // Arrange
  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  AdvanceClockBy(base::Days(1) + base::Milliseconds(1));

  RecordAdEvent(kID1, AdType::kNotificationAd, ConfirmationType::kViewed);

  // Act
  const std::vector<base::Time> history =
      GetAdEventHistory(AdType::kNotificationAd, ConfirmationType::kViewed);

  // Assert
  const std::vector<base::Time> expected_history = {Now()};
  EXPECT_EQ(expected_history, history);
}

}  // namespace brave_ads
