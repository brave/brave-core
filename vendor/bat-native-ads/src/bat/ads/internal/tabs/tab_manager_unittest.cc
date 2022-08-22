/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tabs/tab_manager.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/tabs/tab_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTabManagerTest : public TabManagerObserver, public UnitTestBase {
 protected:
  BatAdsTabManagerTest() = default;

  ~BatAdsTabManagerTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    TabManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    TabManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnTabDidChangeFocus(const int32_t tab_id) override {
    tab_did_change_focus_ = true;
  }

  void OnTabDidChange(const TabInfo& tab) override { tab_did_change_ = true; }

  void OnDidOpenNewTab(const TabInfo& tab) override {
    did_open_new_tab_ = true;
  }

  void OnDidCloseTab(const int32_t tab_id) override { did_close_tab_ = true; }

  void OnTabDidStartPlayingMedia(const int32_t tab_id) override {
    tab_did_start_playing_media_ = true;
  }

  void OnTabDidStopPlayingMedia(const int32_t tab_id) override {
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
  const bool has_instance = TabManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsTabManagerTest, IsTabVisible) {
  // Arrange

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance()->IsTabVisible(1));
}

TEST_F(BatAdsTabManagerTest, IsTabOccluded) {
  // Arrange

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, false,
                                          false);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance()->IsTabVisible(1));
}

TEST_F(BatAdsTabManagerTest, OpenNewTab) {
  // Arrange

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);

  // Assert
  absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
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
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, false,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);

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
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          true);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance()->GetTabForId(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, DoNotUpdateExistingOccludedTabWithSameUrl) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, false,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, false,
                                          false);

  // Assert
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, UpdateExistingOccludedTabWithDifferentUrl) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, false,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com/about")},
                                          false, false);

  // Assert
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com/about")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, DoNotUpdateExistingTabWithSameUrl) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);

  // Assert
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, UpdatedExistingTabWithDifferentUrl) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com/about")},
                                          true, false);

  // Assert
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com/about")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, *tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, CloseTab) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnTabClosed(1);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance()->GetTabForId(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_TRUE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, PlayMedia) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://foobar.com")}, true,
                                          false);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_TRUE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://foobar.com")}, true,
                                          false);
  TabManager::GetInstance()->OnMediaPlaying(1);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnMediaPlaying(1);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  TabManager::GetInstance()->OnMediaPlaying(1);
  ResetObserver();

  // Act
  TabManager::GetInstance()->OnMediaStopped(1);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance()->IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_TRUE(tab_did_stop_playing_media_);
}

TEST_F(BatAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://foobar.com")}, true,
                                          false);
  TabManager::GetInstance()->OnTabUpdated(2, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance()->GetVisibleTab();

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 2;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, GetLastVisibleTab) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://foobar.com")}, true,
                                          false);
  TabManager::GetInstance()->OnTabUpdated(2, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance()->GetLastVisibleTab();

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://foobar.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, GetTabForId) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act
  const absl::optional<TabInfo> tab = TabManager::GetInstance()->GetTabForId(1);

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, *tab);
}

TEST_F(BatAdsTabManagerTest, DoNotGetTabForMissingId) {
  // Arrange
  TabManager::GetInstance()->OnTabUpdated(1, {GURL("https://brave.com")}, true,
                                          false);
  ResetObserver();

  // Act

  // Assert
  EXPECT_FALSE(TabManager::GetInstance()->GetTabForId(2));
}

}  // namespace ads
