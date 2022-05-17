/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/browsing/user_activity_scoring.h"

#include "base/time/time.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/covariates/covariates_constants.h"
#include "bat/ads/internal/user_activity/browsing/user_activity.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_trigger_info.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_trigger_info_aliases.h"
#include "bat/ads/internal/user_activity/browsing/user_activity_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityUtilTest : public UnitTestBase {
 protected:
  BatAdsUserActivityUtilTest() = default;

  ~BatAdsUserActivityUtilTest() override = default;
};

TEST_F(BatAdsUserActivityUtilTest, NoTabsOpened) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityUtilTest, TabsOpened) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  AdvanceClock(base::Minutes(30));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfTabsOpened(events);

  // Assert
  EXPECT_EQ(2, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityUtilTest, GetNumberOfUserActivityEvents) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  AdvanceClock(base::Minutes(30));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);

  AdvanceClock(base::Minutes(5));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened = GetNumberOfUserActivityEvents(
      events, UserActivityEventType::kClickedLink);

  // Assert
  EXPECT_EQ(2, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsForMissingEvent) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened =
      GetNumberOfUserActivityEvents(events, UserActivityEventType::kClosedTab);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityUtilTest,
       GetNumberOfUserActivityEventsFromEmptyHistory) {
  // Arrange

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int number_of_tabs_opened =
      GetNumberOfUserActivityEvents(events, UserActivityEventType::kClosedTab);

  // Assert
  EXPECT_EQ(0, number_of_tabs_opened);
}

TEST_F(BatAdsUserActivityUtilTest, GetTimeSinceLastUserActivityEvent) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClock(base::Minutes(30));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClock(base::Minutes(5));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kTabStartedPlayingMedia);
  AdvanceClock(base::Minutes(5));

  UserActivity::Get()->RecordEvent(UserActivityEventType::kOpenedNewTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);
  AdvanceClock(base::Minutes(1));

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  const int64_t expected_time = 6 * base::Time::kSecondsPerMinute;
  EXPECT_EQ(expected_time, time);
}

TEST_F(BatAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventForMissingEvent) {
  // Arrange
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClickedLink);

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  EXPECT_EQ(kUserActivityMissingValue, time);
}

TEST_F(BatAdsUserActivityUtilTest,
       GetTimeSinceLastUserActivityEventFromEmptyHistory) {
  // Arrange

  // Act
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));
  const int64_t time = GetTimeSinceLastUserActivityEvent(
      events, UserActivityEventType::kTabStartedPlayingMedia);

  // Assert
  EXPECT_EQ(kUserActivityMissingValue, time);
}

TEST_F(BatAdsUserActivityUtilTest, ToUserActivityTriggers) {
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

TEST_F(BatAdsUserActivityUtilTest, ToUserActivityTriggersForInvalidTrigger) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers = ToUserActivityTriggers("INVALID");

  // Assert
  const UserActivityTriggerList expected_triggers = {};
  EXPECT_EQ(expected_triggers, triggers);
}

TEST_F(BatAdsUserActivityUtilTest, ToUserActivityTriggersForMalformedTrigger) {
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

TEST_F(BatAdsUserActivityUtilTest, ToUserActivityTriggersForEmptyTrigger) {
  // Arrange

  // Act
  const UserActivityTriggerList triggers = ToUserActivityTriggers("");

  // Assert
  const UserActivityTriggerList expected_triggers = {};
  EXPECT_EQ(expected_triggers, triggers);
}

}  // namespace ads
