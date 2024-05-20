/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer_mock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTabManagerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    TabManager::GetInstance().AddObserver(&observer_mock_);
  }

  void TearDown() override {
    TabManager::GetInstance().RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  ::testing::StrictMock<TabManagerObserverMock> observer_mock_;
};

TEST_F(BraveAdsTabManagerTest, IsVisible) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(/*tab_id=*/1,
                     /*redirect_chain=*/{GURL("https://brave.com")},
                     /*is_new_navigation=*/true, /*is_restoring=*/false,
                     /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_TRUE(TabManager::GetInstance().IsVisible(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsOccluded) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(/*tab_id=*/1,
                     /*redirect_chain=*/{GURL("https://brave.com")},
                     /*is_new_navigation=*/true, /*is_restoring=*/false,
                     /*is_error_page=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().IsVisible(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, OpenNewTab) {
  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidOpenNewTab(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false}));
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotChangeOccludedTabIfMatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChange).Times(0);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, DoNotChangeVisibleTabIfMatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChange).Times(0);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotNotifyForRestoredTabs) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab).Times(0);
  EXPECT_CALL(observer_mock_, OnTabDidChange).Times(0);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus).Times(0);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/true,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest,
       DoNotNotifyTabDidChangeIfReturningToPreviouslyCommittedNavigation) {
  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChange).Times(0);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/false, /*is_restoring=*/true,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeTabFocusToOccluded) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus(/*tab_id=*/1));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, ChangeTabFocusToVisible) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus(/*tab_id=*/1));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeOccudedTabIfMismatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*is_visible=*/false,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_error_page=*/false,
                  /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, ChangeVisibleTabIfMismatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*is_visible=*/true,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_error_page=*/false,
                  /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, CloseTab) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidCloseTab(/*tab_id=*/1));
  NotifyDidCloseTab(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, IsPlayingMedia) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsNotPlayingMedia) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, StartPlayingMedia) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, DoNotStartPlayingMediaIfAlreadyPlaying) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);
  EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidStopPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStopPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  const TabInfo tab{/*id=*/1, /*is_visible=*/true,
                    /*redirect_chain=*/{GURL("https://brave.com")},
                    /*is_error_page=*/false,
                    /*is_playing_media=*/false};
  EXPECT_EQ(tab, TabManager::GetInstance().GetVisible());
}

TEST_F(BraveAdsTabManagerTest, DoNotGetVisibleTabIfNoTabs) {
  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().GetVisible());
}

TEST_F(BraveAdsTabManagerTest, GetTabForId) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/2,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  const TabInfo tab{
      /*id=*/2, /*is_visible=*/true,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_error_page=*/false,
      /*is_playing_media=*/false};
  EXPECT_EQ(tab, TabManager::GetInstance().MaybeGetForId(2));
}

TEST_F(BraveAdsTabManagerTest, DoNotGetIfMissingTab) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus);
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(2));
}

TEST_F(BraveAdsTabManagerTest, DoNotGetIfNoTabs) {
  // Arrange

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1));
}

}  // namespace brave_ads
