/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_scoring.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityScoringTest : public UnitTestBase {
 protected:
  BatAdsUserActivityScoringTest() = default;

  ~BatAdsUserActivityScoringTest() override = default;
};

TEST_F(BatAdsUserActivityScoringTest, GetUserActivityScore) {
  // Arrange
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("06=.3;0D1406=1.0;0D14=0.5");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.8, score);
}

TEST_F(BatAdsUserActivityScoringTest, GetUserActivityScoreForTimeWindow) {
  // Arrange
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("06=.3;0D1406=1.0;0D14=0.5");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  AdvanceClock(base::TimeDelta::FromHours(2));
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.5, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForInvalidEventSequence) {
  // Arrange
  const UserActivityTriggers triggers = ToUserActivityTriggers("INVALID");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(0.0, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForMalformedEventSequence) {
  // Arrange
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("06=1;0D1406=1.0;=0.5");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(2.0, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForMixedCaseEventSequence) {
  // Arrange
  const UserActivityTriggers triggers =
      ToUserActivityTriggers("06=.3;0d1406=1.0;0D14=0.5");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.8, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForEmptyEventSequence) {
  // Arrange
  const UserActivityTriggers triggers = ToUserActivityTriggers("");

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedReloadButton);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kTypedUrl);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(0.0, score);
}

}  // namespace ads
