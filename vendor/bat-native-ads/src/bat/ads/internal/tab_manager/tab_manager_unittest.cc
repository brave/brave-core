/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tab_manager/tab_manager.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/tab_manager/tab_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTabManagerTest : public TabManagerObserver, public UnitTestBase {
 protected:
  BatAdsTabManagerTest() = default;

  ~BatAdsTabManagerTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    TabManager::Get()->AddObserver(this);
  }

  void TearDown() override {
    TabManager::Get()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnTabDidChangeFocus(const int32_t id) override {
    tab_did_change_focus_ = true;
  }

  void OnTabDidChange(const int32_t id) override { tab_did_change_ = true; }

  void OnDidOpenNewTab(const int32_t id) override { did_open_new_tab_ = true; }

  void OnDidCloseTab(const int32_t id) override { did_close_tab_ = true; }

  void OnTabDidStartPlayingMedia(const int32_t id) override {
    tab_did_start_playing_media_ = true;
  }

  void OnTabDidStopPlayingMedia(const int32_t id) override {
    tab_did_stop_playing_media_ = true;
  }

  void ResetObserver() {
    tab_did_change_focus_ = false;
    tab_did_change_ = false;
    did_open_new_tab_ = false;
    did_close_tab_ = false;
    tab_did_start_playing_media_ = false;
    tab_did_stop_playing_media_ = false;
  }

  bool tab_did_change_focus_ = false;
  bool tab_did_change_ = false;
  bool did_open_new_tab_ = false;
  bool did_close_tab_ = false;
  bool tab_did_start_playing_media_ = false;
  bool tab_did_stop_playing_media_ = false;
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
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsVisible(1));
}

TEST_F(BatAdsTabManagerTest, IsTabOccluded) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), false, false);

  // Assert
  EXPECT_FALSE(TabManager::Get()->IsVisible(1));
}

TEST_F(BatAdsTabManagerTest, OpenNewTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com");
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_TRUE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, ChangeTabFocus) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), false, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);

  // Assert
  EXPECT_TRUE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, DoNotUpdateIncognitoTab) {
  // Arrange

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, true);

  // Assert
  EXPECT_FALSE(TabManager::Get()->GetForId(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, DoNotUpdateExistingOccludedTabWithSameUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), false, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), false, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com");
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab.value());

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, UpdateExistingOccludedTabWithDifferentUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), false, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com/about"), false,
                               false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com/about");
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab.value());

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, DoNotUpdateExistingTabWithSameUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com");
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab.value());

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, UpdatedExistingTabWithDifferentUrl) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com/about"), true, false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::Get()->GetForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com/about");
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab.value());

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, CloseTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnClosed(1);

  // Assert
  EXPECT_FALSE(TabManager::Get()->GetForId(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_TRUE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, PlayMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"), true, false);
  ResetObserver();

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_TRUE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"), true, false);
  TabManager::Get()->OnMediaPlaying(1);
  ResetObserver();

  // Act
  TabManager::Get()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::Get()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  TabManager::Get()->OnMediaPlaying(1);
  ResetObserver();

  // Act
  TabManager::Get()->OnMediaStopped(1);

  // Assert
  EXPECT_FALSE(TabManager::Get()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_TRUE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"), true, false);
  TabManager::Get()->OnUpdated(2, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo>& tab_optional = TabManager::Get()->GetVisible();

  // Assert
  const TabInfo& tab = tab_optional.value();

  TabInfo expected_tab;
  expected_tab.id = 2;
  expected_tab.url = GURL("https://brave.com");
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, GetLastVisibleTab) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"), true, false);
  TabManager::Get()->OnUpdated(2, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo>& tab_optional =
      TabManager::Get()->GetLastVisible();

  // Assert
  const TabInfo& tab = tab_optional.value();

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://foobar.com");
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, GetTabForId) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo>& tab_optional = TabManager::Get()->GetForId(1);

  // Assert
  const TabInfo& tab = tab_optional.value();

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.url = GURL("https://brave.com");
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BatAdsTabManagerTest, DoNotGetTabForMissingId) {
  // Arrange
  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"), true, false);
  ResetObserver();

  // Act

  // Assert
  EXPECT_FALSE(TabManager::Get()->GetForId(2));
}

}  // namespace ads
