/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_scoring.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_trigger_info.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityScoringTest : public UnitTestBase {};

TEST_F(BatAdsUserActivityScoringTest, GetUserActivityScore) {
  // Arrange
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("06=.3;0D1406=1.0;0D14=0.5");

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.8, score);
}

TEST_F(BatAdsUserActivityScoringTest, GetUserActivityScoreForTimeWindow) {
  // Arrange
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("06=.3;0D1406=1.0;0D14=0.5");

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  AdvanceClockBy(base::Hours(2));
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.5, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForInvalidEventSequence) {
  // Arrange
  const UserActivityTriggerList triggers = ToUserActivityTriggers("INVALID");

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(0.0, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForMalformedEventSequence) {
  // Arrange
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("06=1;0D1406=1.0;=0.5");

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(2.0, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForMixedCaseEventSequence) {
  // Arrange
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("06=.3;0d1406=1.0;0D14=0.5");

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(1.8, score);
}

TEST_F(BatAdsUserActivityScoringTest,
       GetUserActivityScoreForEmptyEventSequence) {
  // Arrange
  const UserActivityTriggerList triggers = ToUserActivityTriggers({});

  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedReloadButton);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kTypedUrl);
  UserActivityManager::GetInstance()->RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance()->GetHistoryForTimeWindow(
          base::Hours(1));

  // Act
  const double score = GetUserActivityScore(triggers, events);

  // Assert
  EXPECT_EQ(0.0, score);
}

}  // namespace ads
