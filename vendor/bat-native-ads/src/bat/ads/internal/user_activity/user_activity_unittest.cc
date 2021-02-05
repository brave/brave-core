/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsUserActivityTest : public UnitTestBase {
 protected:
  BatAdsUserActivityTest() = default;

  ~BatAdsUserActivityTest() override = default;
};

TEST_F(BatAdsUserActivityTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = UserActivity::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsUserActivityTest, RecordOpenedNewOrFocusedOnExistingTabEvent) {
  // Arrange

  const UserActivityEventType event_type =
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();

  EXPECT_EQ(1UL, history.size());
  EXPECT_NE(history.end(), history.find(event_type));
}

TEST_F(BatAdsUserActivityTest, RecordClosedTabEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();
  EXPECT_EQ(1UL, history.size());
  EXPECT_NE(history.end(), history.find(event_type));
}

TEST_F(BatAdsUserActivityTest, RecordPlayedMediaEvent) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kPlayedMedia;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();
  EXPECT_EQ(1UL, history.size());
  EXPECT_NE(history.end(), history.find(event_type));
}

TEST_F(BatAdsUserActivityTest, RecordBrowserWindowDidBecomeActiveEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserWindowDidBecomeActive;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();
  EXPECT_EQ(1UL, history.size());
  EXPECT_NE(history.end(), history.find(event_type));
}

TEST_F(BatAdsUserActivityTest, RecordBrowserWindowDidEnterBackgroundEvent) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kBrowserWindowDidEnterBackground;

  // Act
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();
  EXPECT_EQ(1UL, history.size());
  EXPECT_NE(history.end(), history.find(event_type));
}

TEST_F(BatAdsUserActivityTest, RecordTheSameEventMultipleTimes) {
  // Arrange
  const UserActivityEventType event_type =
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab;

  // Act
  UserActivity::Get()->RecordEvent(event_type);
  UserActivity::Get()->RecordEvent(event_type);

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();

  const UserActivityEventHistory user_activity_event_history =
      history.find(event_type)->second;

  EXPECT_EQ(2UL, user_activity_event_history.size());
}

TEST_F(BatAdsUserActivityTest, GetHistory) {
  // Arrange
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kClosedTab);
  UserActivity::Get()->RecordEvent(UserActivityEventType::kPlayedMedia);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowDidBecomeActive);
  UserActivity::Get()->RecordEvent(
      UserActivityEventType::kBrowserWindowDidEnterBackground);

  // Act
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();

  // Assert
  EXPECT_EQ(5UL, history.size());
}

TEST_F(BatAdsUserActivityTest, MaximumUserActivityEventHistoryEntries) {
  // Arrange
  const UserActivityEventType event_type = UserActivityEventType::kClosedTab;

  // Act
  for (int i = 0; i < 101; i++) {
    UserActivity::Get()->RecordEvent(event_type);
  }

  // Assert
  const UserActivityEventHistoryMap history =
      UserActivity::Get()->get_history();

  const UserActivityEventHistory user_activity_event_history =
      history.find(event_type)->second;

  EXPECT_EQ(100UL, user_activity_event_history.size());
}

}  // namespace ads
