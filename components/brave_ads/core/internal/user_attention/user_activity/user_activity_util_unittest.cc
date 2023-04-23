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

TEST_F(BraveAdsUserActivityUtilTest, NoTabsOpened) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BraveAdsUserActivityUtilTest, TabsOpened) {
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
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // Assert
  EXPECT_EQ(2, number_of_tabs_opened);
}

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

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfUserActivityEvents(
      events, UserActivityEventType::kClickedLink);

  // Assert
  EXPECT_EQ(2, number_of_tabs_opened);
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsForMissingEvent) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int number_of_tabs_opened =
      GetNumberOfUserActivityEvents(events, UserActivityEventType::kClosedTab);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsFromEmptyHistory) {
  // Arrange

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int number_of_tabs_opened =
      GetNumberOfUserActivityEvents(events, UserActivityEventType::kClosedTab);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
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

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  EXPECT_EQ(6 * base::Time::kSecondsPerMinute, time);
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventForMissingEvent) {
  // Arrange
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClickedLink);

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  EXPECT_EQ(kUserActivityMissingValue, time);
}

TEST_F(BraveAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventFromEmptyHistory) {
  // Arrange

  // Act
  const UserActivityEventList events =
      UserActivityManager::GetInstance().GetHistoryForTimeWindow(
          base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  EXPECT_EQ(kUserActivityMissingValue, time);
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggers) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("05=.3;0C1305=1.0;0C13=0.5");

  // Assert
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

  EXPECT_EQ(expected_triggers, triggers);
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggersForInvalidTrigger) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers = ToUserActivityTriggers("INVALID");

  // Assert
  const UserActivityTriggerList expected_triggers;
  EXPECT_EQ(expected_triggers, triggers);
}

TEST_F(BraveAdsUserActivityUtilTest,
       ToUserActivityTriggersForMalformedTrigger) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers =
      ToUserActivityTriggers("05=.3;0C1305=;=0.5;C1305=1.0");

  // Assert
  UserActivityTriggerList expected_triggers;
  UserActivityTriggerInfo trigger;
  trigger.event_sequence = "05";
  trigger.score = 0.3;
  expected_triggers.push_back(trigger);

  EXPECT_EQ(expected_triggers, triggers);
}

TEST_F(BraveAdsUserActivityUtilTest, ToUserActivityTriggersForEmptyTrigger) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers = ToUserActivityTriggers({});

  // Assert
  const UserActivityTriggerList expected_triggers;
  EXPECT_EQ(expected_triggers, triggers);
}

}  // namespace brave_ads
