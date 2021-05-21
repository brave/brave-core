/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_manager.h"

#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/user_activity/user_activity_event_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTabManagerTest : public UnitTestBase {
 protected:
  BatAdsTabManagerTest() = default;

  ~BatAdsTabManagerTest() override = default;
};

TEST_F(BatAdsTabManagerTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = TabManager::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsTabManagerTest, IsTabVisible) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsVisible(1));
}

TEST_F(BatAdsTabManagerTest, IsTabOccluded) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", false, false);

  // Assert
  EXPECT_FALSE(TabManager::Get()->IsVisible(1));
}

TEST_F(BatAdsTabManagerTest, UpdatedTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, OpenedNewTabUserActivityEvent) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, FocusedOnExistingTabUserActivityEvent) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", false, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kFocusedOnExistingTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, UpdatedIncognitoTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, true);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);
  EXPECT_EQ(absl::nullopt, tab);
}

TEST_F(BatAdsTabManagerTest, DoNotRecordEventWhenUpdatingIncognitoTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, true);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  const UserActivityEvents expected_events = {};

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, UpdatedOccludedTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", false, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab.value());
}

TEST_F(BatAdsTabManagerTest, DoNotRecordEventWhenUpdatingOccludedTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", false, false);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  const UserActivityEvents expected_events = {};

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, UpdatedExistingTabWithSameUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab.value());
}

TEST_F(BatAdsTabManagerTest,
       DoNotRecordEventWhenUpdatingExistingTabWithSameUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, UpdatedExistingTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com/about", true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com/about";
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab.value());
}

TEST_F(BatAdsTabManagerTest, RecordEventWhenUpdatingExistingTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com/about", true, false);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kTabUpdated;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, ClosedTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnClosed(1);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);
  EXPECT_EQ(absl::nullopt, tab);
}

TEST_F(BatAdsTabManagerTest, RecordEventWhenClosingTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnClosed(1);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kClosedTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, PlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));
}

TEST_F(BatAdsTabManagerTest, RecordEventWhenPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kPlayedMedia;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnMediaPlaying(1);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));
}

TEST_F(BatAdsTabManagerTest, DoNotRecordEventWhenAlreadyPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnMediaPlaying(1);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kPlayedMedia;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, StoppedPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);
  TabManager::Get()->OnMediaPlaying(1);

  // Act
  TabManager::Get()->OnMediaStopped(1);

  // Assert
  EXPECT_FALSE(TabManager::Get()->IsPlayingMedia(1));
}

TEST_F(BatAdsTabManagerTest, RecordEventWhenStoppedPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);
  TabManager::Get()->OnMediaPlaying(1);

  // Act
  TabManager::Get()->OnMediaStopped(1);

  // Assert
  const UserActivityEvents events =
      UserActivity::Get()->GetHistoryForTimeWindow(
          base::TimeDelta::FromHours(1));

  UserActivityEvents expected_events;
  UserActivityEventInfo event;
  event.type = UserActivityEventType::kOpenedNewTab;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kPlayedMedia;
  event.time = base::Time::Now();
  expected_events.push_back(event);
  event.type = UserActivityEventType::kStoppedPlayingMedia;
  event.time = base::Time::Now();
  expected_events.push_back(event);

  EXPECT_EQ(expected_events, events);
}

TEST_F(BatAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnUpdated(2, "https://brave.com", true, false);

  // Act
  absl::optional<TabInfo> tab = TabManager::Get()->GetVisible();

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 2;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, GetLastVisibleTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnUpdated(2, "https://brave.com", true, false);

  // Act
  absl::optional<TabInfo> tab = TabManager::Get()->GetLastVisible();

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://foobar.com";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, GetTabForId) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, GetTabWithInvalidId) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(2);

  // Assert
  EXPECT_EQ(absl::nullopt, tab);
}

}  // namespace ads
