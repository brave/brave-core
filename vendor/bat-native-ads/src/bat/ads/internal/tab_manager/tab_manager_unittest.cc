/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_manager.h"

#include "bat/ads/internal/tab_manager/tab_info.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTabManagerTest : public UnitTestBase {
 protected:
  BatAdsTabManagerTest() = default;

  ~BatAdsTabManagerTest() override = default;

  int UserActivityCount() {
    const UserActivityEventHistoryMap history =
        UserActivity::Get()->get_history();

    int count = 0;
    for (const auto& item : history) {
      const UserActivityEventHistory user_activity_event_history = item.second;
      count += user_activity_event_history.size();
    }

    return count;
  }

  int UserActivityCountForEventType(const UserActivityEventType event_type) {
    const UserActivityEventHistoryMap history =
        UserActivity::Get()->get_history();

    const UserActivityEventHistory user_activity_event_history =
        history.find(event_type)->second;

    return user_activity_event_history.size();
  }
};

TEST_F(BatAdsTabManagerTest, HasInstance) {
  // Arrange

  // Act

  // Assert
  const bool has_instance = TabManager::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsTabManagerTest, BrowserWindowDidBecomeActive) {
  // Arrange

  // Act
  TabManager::Get()->OnForegrounded();

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsForegrounded());
}

TEST_F(BatAdsTabManagerTest, BrowserWindowDidBecomeActiveUserActivityEvent) {
  // Arrange

  // Act
  TabManager::Get()->OnForegrounded();

  // Assert
  const int count = UserActivityCountForEventType(
      UserActivityEventType::kBrowserWindowDidBecomeActive);

  EXPECT_EQ(1, count);
}

TEST_F(BatAdsTabManagerTest, BrowserWindowDidEnterBackground) {
  // Arrange

  // Act
  TabManager::Get()->OnBackgrounded();

  // Assert
  EXPECT_FALSE(TabManager::Get()->IsForegrounded());
}

TEST_F(BatAdsTabManagerTest, BrowserWindowDidEnterBackgroundUserActivityEvent) {
  // Arrange

  // Act
  TabManager::Get()->OnBackgrounded();

  // Assert
  const int count = UserActivityCountForEventType(
      UserActivityEventType::kBrowserWindowDidEnterBackground);

  EXPECT_EQ(1, count);
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
  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, UpdatedTabUserActivityEvent) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Assert
  const int count = UserActivityCountForEventType(
      UserActivityEventType::kOpenedNewOrFocusedOnExistingTab);

  EXPECT_EQ(1, count);
}

TEST_F(BatAdsTabManagerTest, UpdatedIncognitoTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, true);

  // Assert
  EXPECT_EQ(0, UserActivityCount());

  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);
  EXPECT_EQ(base::nullopt, tab);
}

TEST_F(BatAdsTabManagerTest, UpdatedOccludedTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com", false, false);

  // Assert
  EXPECT_EQ(0, UserActivityCount());

  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);
  EXPECT_EQ(base::nullopt, tab);
}

TEST_F(BatAdsTabManagerTest, UpdatedExistingTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnUpdated(1, "https://brave.com/about", true, false);

  // Assert
  EXPECT_EQ(1, UserActivityCount());

  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = "https://brave.com/about";
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, ClosedTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnClosed(1);

  // Assert
  EXPECT_EQ(2, UserActivityCount());

  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);
  EXPECT_EQ(base::nullopt, tab);
}

TEST_F(BatAdsTabManagerTest, ClosedTabUserActivityEvent) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://brave.com", true, false);

  // Act
  TabManager::Get()->OnClosed(1);

  // Assert
  const int count =
      UserActivityCountForEventType(UserActivityEventType::kClosedTab);

  EXPECT_EQ(1, count);
}

TEST_F(BatAdsTabManagerTest, PlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_EQ(2, UserActivityCount());
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));
}

TEST_F(BatAdsTabManagerTest, PlayingMediaUserActivityEvent) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  const int count =
      UserActivityCountForEventType(UserActivityEventType::kPlayedMedia);

  EXPECT_EQ(1, count);
}

TEST_F(BatAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnMediaPlaying(1);

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_EQ(2, UserActivityCount());
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));
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

TEST_F(BatAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, "https://foobar.com", true, false);
  TabManager::Get()->OnUpdated(2, "https://brave.com", true, false);

  // Act
  base::Optional<TabInfo> tab = TabManager::Get()->GetVisible();

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
  base::Optional<TabInfo> tab = TabManager::Get()->GetLastVisible();

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
  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(1);

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
  base::Optional<TabInfo> tab = TabManager::Get()->GetForId(2);

  // Assert
  EXPECT_EQ(base::nullopt, tab);
}

}  // namespace ads
