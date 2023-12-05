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

  void OpenTab(const int32_t tab_id,
               const std::vector<GURL>& redirect_chain,
               const bool is_visible) {
    const TabInfo tab(tab_id, redirect_chain, /*is_playing_media=*/false);

    if (!is_visible) {
      EXPECT_CALL(observer_mock_, OnTabDidChange(tab));
    } else {
      EXPECT_CALL(observer_mock_, OnDidOpenNewTab(tab));
    }

    NotifyTabDidChange(tab_id, redirect_chain, is_visible);
  }

  void PlayMedia(const int32_t tab_id) {
    EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(tab_id));
    NotifyTabDidStartPlayingMedia(tab_id);
  }

  ::testing::StrictMock<TabManagerObserverMock> observer_mock_;
};

TEST_F(BraveAdsTabManagerTest, IsVisible) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_TRUE(TabManager::GetInstance().IsVisible(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsOccluded) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/false);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().IsVisible(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsPlayingMedia) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  PlayMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsNotPlayingMedia) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, OpenNewTab) {
  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidOpenNewTab(TabInfo{/*id=*/1,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeTab) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeTabFocus) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidChangeFocus(/*tab_id=*/1));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotUpdateExistingOccludedTabIfSameUrl) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/false);

  // Act & Assert
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, UpdateExistingOccludedTabIfDifferentUrl) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, UpdateExistingVisibleTabIfSameUrl) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo(
                  /*id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
                  /*is_playing_media=*/false)));

  NotifyTabDidChange(/*tab_id=*/1,
                     /*redirect_chain=*/{GURL("https://brave.com")},
                     /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotUpdateExistingVisibleTabIfDifferentUrl) {
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo(
                  /*id=*/1,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/false)));

  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, UpdatedExistingVisibleTabIfDifferentUrl) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/false}));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, CloseTab) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidCloseTab(/*tab_id=*/1));
  NotifyDidCloseTab(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, PlayMedia) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidStartPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, AlreadyPlayingMedia) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  PlayMedia(/*tab_id=*/1);

  // Act & Assert
  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  PlayMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnTabDidStopPlayingMedia(/*tab_id=*/1));
  NotifyTabDidStopPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, GetVisible) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  OpenTab(/*tab_id=*/2,
          /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_EQ(TabInfo(
                /*id=*/2,
                /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                /*is_playing_media=*/false),
            TabManager::GetInstance().GetVisible());
}

TEST_F(BraveAdsTabManagerTest, GetForId) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  OpenTab(/*tab_id=*/2,
          /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_EQ(TabInfo(
                /*id=*/2,
                /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                /*is_playing_media=*/false),
            TabManager::GetInstance().MaybeGetForId(2));
}

TEST_F(BraveAdsTabManagerTest, DoNotGetForMissingId) {
  // Arrange
  OpenTab(/*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
          /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(2));
}

}  // namespace brave_ads
