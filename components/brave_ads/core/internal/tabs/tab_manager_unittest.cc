/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTabManagerTest : public TabManagerObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    TabManager::GetInstance().AddObserver(this);
  }

  void TearDown() override {
    TabManager::GetInstance().RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnTabDidChangeFocus(const int32_t /*tab_id*/) override {
    tab_did_change_focus_ = true;
  }

  void OnTabDidChange(const TabInfo& /*tab*/) override {
    tab_did_change_ = true;
  }

  void OnDidOpenNewTab(const TabInfo& /*tab*/) override {
    did_open_new_tab_ = true;
  }

  void OnDidCloseTab(const int32_t /*tab_id*/) override {
    did_close_tab_ = true;
  }

  void OnTabDidStartPlayingMedia(const int32_t /*tab_id*/) override {
    tab_did_start_playing_media_ = true;
  }

  void OnTabDidStopPlayingMedia(const int32_t /*tab_id*/) override {
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

TEST_F(BraveAdsTabManagerTest, IsVisible) {
  // Arrange

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance().IsVisible(/*id*/ 1));
}

TEST_F(BraveAdsTabManagerTest, IsTabOccluded) {
  // Arrange

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().IsVisible(/*id*/ 1));
}

TEST_F(BraveAdsTabManagerTest, OpenNewTab) {
  // Arrange

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Assert
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);

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

TEST_F(BraveAdsTabManagerTest, ChangeTabFocus) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false);
  ResetObserver();

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Assert
  EXPECT_TRUE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, DoNotUpdateExistingOccludedTabWithSameUrl) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false);
  ResetObserver();

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false);

  // Assert
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);
  ASSERT_TRUE(tab);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, UpdateExistingOccludedTabWithDifferentUrl) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false);
  ResetObserver();

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com/about")},
      /*is_visible*/ false);

  // Assert
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);
  ASSERT_TRUE(tab);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com/about")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, DoNotUpdateExistingTabWithSameUrl) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);
  ResetObserver();

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Assert
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);
  ASSERT_TRUE(tab);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, UpdatedExistingTabWithDifferentUrl) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);
  ResetObserver();

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com/about")},
      /*is_visible*/ true);

  // Assert
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);
  ASSERT_TRUE(tab);

  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com/about")};
  expected_tab.is_playing_media = false;
  EXPECT_EQ(expected_tab, tab);

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_TRUE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, CloseTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);
  ResetObserver();

  // Act
  NotifyDidCloseTab(/*id*/ 1);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_TRUE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, PlayMedia) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true);
  ResetObserver();

  // Act
  NotifyTabDidStartPlayingMedia(/*tab_id*/ 1);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance().IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_TRUE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true);
  NotifyTabDidStartPlayingMedia(/*tab_id*/ 1);
  ResetObserver();

  // Act
  NotifyTabDidStartPlayingMedia(/*tab_id*/ 1);

  // Assert
  EXPECT_TRUE(TabManager::GetInstance().IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_FALSE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);
  NotifyTabDidStartPlayingMedia(/*tab_id*/ 1);
  ResetObserver();

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().IsPlayingMedia(1));

  EXPECT_FALSE(tab_did_change_focus_);
  EXPECT_FALSE(tab_did_change_);
  EXPECT_FALSE(did_open_new_tab_);
  EXPECT_FALSE(did_close_tab_);
  EXPECT_FALSE(tab_did_start_playing_media_);
  EXPECT_TRUE(tab_did_stop_playing_media_);
}

TEST_F(BraveAdsTabManagerTest, GetVisible) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true);
  NotifyTabDidChange(
      /*id*/ 2, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act
  const absl::optional<TabInfo> tab = TabManager::GetInstance().GetVisible();
  ASSERT_TRUE(tab);

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 2;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BraveAdsTabManagerTest, GetLastVisible) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true);
  NotifyTabDidChange(
      /*id*/ 2, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().GetLastVisible();
  ASSERT_TRUE(tab);

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://foobar.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BraveAdsTabManagerTest, GetForId) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act
  const absl::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(1);
  ASSERT_TRUE(tab);

  // Assert
  TabInfo expected_tab;
  expected_tab.id = 1;
  expected_tab.redirect_chain = {GURL("https://brave.com")};
  expected_tab.is_playing_media = false;

  EXPECT_EQ(expected_tab, tab);
}

TEST_F(BraveAdsTabManagerTest, DoNotGetTabForMissingId) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(2));
}

}  // namespace brave_ads
