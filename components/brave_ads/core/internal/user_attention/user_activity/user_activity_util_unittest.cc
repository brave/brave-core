/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_trigger_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserActivityUtilTest : public UnitTestBase {};

TEST_F(BraveAdsUserActivityUtilTest, GetNumberOfUserActivityEvents) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);

  AdvanceClockBy(base::Minutes(30));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);

  AdvanceClockBy(base::Minutes(5));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_EQ(2U, GetNumberOfUserActivityEvents(
                    events, UserActivityEventType::kClickedLink));
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsForMissingEvent) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_EQ(0U, GetNumberOfUserActivityEvents(
                    events, UserActivityEventType::kClosedTab));
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsFromEmptyHistory) {
  // Arrange
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_EQ(0U, GetNumberOfUserActivityEvents(
                    events, UserActivityEventType::kClosedTab));
}

TEST_F(BraveAdsUserActivityUtilTest, GetTimeSinceLastUserActivityEvent) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClockBy(base::Minutes(30));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClockBy(base::Minutes(5));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClockBy(base::Minutes(5));

  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);
  AdvanceClockBy(base::Minutes(1));

  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_EQ(base::Minutes(6),
            GetTimeSinceLastUserActivityEvent(
                events, UserActivityEventType::kTabStartedPlayingMedia));
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventForMissingEvent) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);

  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_TRUE(GetTimeSinceLastUserActivityEvent(
                  events, UserActivityEventType::kTabStartedPlayingMedia)
                  .is_zero());
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventFromEmptyHistory) {
  // Arrange
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));

  // Act & Assert
  EXPECT_TRUE(GetTimeSinceLastUserActivityEvent(
                  events, UserActivityEventType::kTabStartedPlayingMedia)
                  .is_zero());
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggers) {
  // Act & Assert
  UserActivityTriggerList expected_triggers;
  UserActivityTriggerInfo trigger;
  trigger.event_sequence = "05";
  trigger.score = 0.3;
  expected_triggers.push_back(trigger);
  trigger.event_sequence = "0C1305";
  trigger.score = 1.0;
  expected_triggers.push_back(trigger);
  trigger.event_sequence = "0C13";
  trigger.score = 0.5;
  expected_triggers.push_back(trigger);
  EXPECT_EQ(
      expected_triggers,
      ToUserActivityTriggers(/*param_value=*/"05=.3;0C1305=1.0;0C13=0.5"));
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggersForInvalidTrigger) {
  // Act
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(/*param_value=*/"INVALID");

  // Assert
  EXPECT_TRUE(triggers.empty());
}

TEST_F(BraveAdsUserActivityUtilTest,
       ToUserActivityTriggersForMalformedTrigger) {
  // Act & Assert
  UserActivityTriggerList expected_triggers;
  UserActivityTriggerInfo trigger;
  trigger.event_sequence = "05";
  trigger.score = 0.3;
  expected_triggers.push_back(trigger);
  EXPECT_EQ(
      expected_triggers,
      ToUserActivityTriggers(/*param_value=*/"05=.3;0C1305=;=0.5;C1305=1.0"));
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggersForEmptyTrigger) {
  // Act
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers(/*param_value=*/{});

  // Assert
  EXPECT_TRUE(triggers.empty());
}

}  // namespace brave_ads
