/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser_manager/browser_manager.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity_event_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserManagerTest : public UnitTestBase {
 protected:
  BatAdsBrowserManagerTest() = default;

  ~BatAdsBrowserManagerTest() override = default;
};

TEST_F(BatAdsBrowserManagerTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = BrowserManager::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsBrowserManagerTest, BrowserWindowIsActive) {
  // Arrange
  BrowserManager::Get()->SetForegrounded(true);
  BrowserManager::Get()->SetActive(false);

  // Act
  BrowserManager::Get()->OnActive();

  // Assert
  EXPECT_TRUE(BrowserManager::Get()->IsActive());
}

TEST_F(BatAdsBrowserManagerTest, BrowserWindowIsActiveUserActivityEvent) {
  // Arrange
  BrowserManager::Get()->SetActive(false);

  // Act
  BrowserManager::Get()->OnActive();

  // Assert
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kBrowserWindowIsActive;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsBrowserManagerTest, BrowserWindowIsInactive) {
  // Arrange
  BrowserManager::Get()->SetActive(true);

  // Act
  BrowserManager::Get()->OnInactive();

  // Assert
  EXPECT_FALSE(BrowserManager::Get()->IsActive());
}

TEST_F(BatAdsBrowserManagerTest, BrowserWindowIsInactiveUserActivityEvent) {
  // Arrange
  BrowserManager::Get()->SetActive(true);

  // Act
  BrowserManager::Get()->OnInactive();

  // Assert
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kBrowserWindowIsInactive;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidBecomeActive) {
  // Arrange

  // Act
  BrowserManager::Get()->OnForegrounded();

  // Assert
  EXPECT_TRUE(BrowserManager::Get()->IsForegrounded());
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidBecomeActiveUserActivityEvent) {
  // Arrange
  BrowserManager::Get()->SetActive(false);

  // Act
  BrowserManager::Get()->OnActive();

  // Assert
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kBrowserWindowIsActive;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterBackground) {
  // Arrange
  BrowserManager::Get()->SetForegrounded(true);

  // Act
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  EXPECT_FALSE(BrowserManager::Get()->IsForegrounded());
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterBackgroundUserActivityEvent) {
  // Arrange
  BrowserManager::Get()->SetForegrounded(true);

  // Act
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEventList expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kBrowserDidEnterBackground;
  event.created_at = Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

}  // namespace ads
