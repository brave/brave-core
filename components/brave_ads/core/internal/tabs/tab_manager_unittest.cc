/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"

#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/tabs/test/tab_manager_observer_mock.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTabManagerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    observation_.Observe(&TabManager::GetInstance());
  }

  ::testing::StrictMock<TabManagerObserverMock> tab_manager_observer_mock_;
  base::ScopedObservation<TabManager, TabManagerObserver> observation_{
      &tab_manager_observer_mock_};
};

TEST_F(BraveAdsTabManagerTest, OpenNewTab) {
  // Act & Assert
  EXPECT_CALL(
      tab_manager_observer_mock_,
      OnDidOpenNewTab(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_playing_media=*/false}));
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotChangeOccludedTabIfMatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange).Times(0);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, DoNotChangeVisibleTabIfMatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange).Times(0);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, DoNotNotifyForRestoredTabs) {
  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab).Times(0);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange).Times(0);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus).Times(0);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/true, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest,
       DoNotNotifyTabDidChangeIfReturningToPreviouslyCommittedNavigation) {
  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange).Times(0);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/false, /*is_restoring=*/true, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeTabFocusToOccluded) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, ChangeTabFocusToVisible) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, ChangeOccudedTabIfMismatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*is_visible=*/false,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/
                  false}));
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/false);
}

TEST_F(BraveAdsTabManagerTest, ChangeVisibleTabIfMismatchingRedirectChain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidChange(TabInfo{
                  /*id=*/1,
                  /*is_visible=*/true,
                  /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
                  /*is_playing_media=*/
                  false}));
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsTabManagerTest, CloseTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_, OnDidCloseTab(/*tab_id=*/1));
  ads_client_notifier_.NotifyDidCloseTab(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, FailToLoadTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(
      tab_manager_observer_mock_,
      OnTabDidFailToLoad(TabInfo{/*id=*/1,
                                 /*is_visible=*/true,
                                 /*redirect_chain=*/{GURL("https://brave.com")},
                                 /*is_playing_media=*/false}));
  ads_client_notifier_.NotifyTabDidFailToLoad(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, IsPlayingMedia) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidStartPlayingMedia(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, IsNotPlayingMedia) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().IsPlayingMedia(/*tab_id=*/1));
}

TEST_F(BraveAdsTabManagerTest, StartPlayingMedia) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidStartPlayingMedia(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, DoNotStartPlayingMediaIfAlreadyPlaying) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false,
      /*is_visible=*/true);
  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidStartPlayingMedia(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, StopPlayingMedia) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidStartPlayingMedia(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_CALL(tab_manager_observer_mock_,
              OnTabDidStopPlayingMedia(/*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStopPlayingMedia(/*tab_id=*/1);
}

TEST_F(BraveAdsTabManagerTest, GetVisibleTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  const TabInfo tab{/*id=*/1, /*is_visible=*/true,
                    /*redirect_chain=*/{GURL("https://brave.com")},
                    /*is_playing_media=*/
                    false};
  EXPECT_EQ(tab, TabManager::GetInstance().MaybeGetVisible());
}

TEST_F(BraveAdsTabManagerTest, DoNotGetVisibleTabIfNoTabs) {
  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetVisible());
}

TEST_F(BraveAdsTabManagerTest, GetTabForId) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/2,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  const TabInfo tab{
      /*id=*/2, /*is_visible=*/true,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_playing_media=*/
      false};
  EXPECT_EQ(tab, TabManager::GetInstance().MaybeGetForId(2));
}

TEST_F(BraveAdsTabManagerTest, DoNotGetIfMissingTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(2));
}

TEST_F(BraveAdsTabManagerTest, DoNotGetIfNoTabs) {
  // Act & Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1));
}

TEST_F(BraveAdsTabManagerTest, SetOrderIdExtractedForTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Assert
  EXPECT_EQ("336121537", TabManager::GetInstance().MaybeGetForId(1)->order_id);
}

TEST_F(BraveAdsTabManagerTest, SetOrderValueExtractedForTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act
  ads_client_notifier_.NotifyOrderValueExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*value=*/"4.98", /*currency=*/"EUR");

  // Assert
  EXPECT_EQ("4.98", TabManager::GetInstance().MaybeGetForId(1)->order_value);
  EXPECT_EQ("EUR", TabManager::GetInstance().MaybeGetForId(1)->order_currency);
}

TEST_F(BraveAdsTabManagerTest, ClearOrderIdExtractedForTabIfNotFound) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/std::nullopt);

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1)->order_id);
}

TEST_F(BraveAdsTabManagerTest,
       SettingOrderIdExtractedForTabDoesNotClearOrderValue) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
  ads_client_notifier_.NotifyOrderValueExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*value=*/"4.98", /*currency=*/"EUR");

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Assert
  EXPECT_EQ("4.98", TabManager::GetInstance().MaybeGetForId(1)->order_value);
}

TEST_F(BraveAdsTabManagerTest,
       DoNotSetOrderIdExtractedForTabIfNavigatedToDifferentDomain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1)->order_id);
}

TEST_F(BraveAdsTabManagerTest,
       SetOrderIdExtractedForTabIfExtractedForEarlierPageOnSameDomain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnDidOpenNewTab);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/checkout")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/thank-you")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com/checkout"),
      /*id=*/"336121537");

  // Assert
  EXPECT_EQ("336121537", TabManager::GetInstance().MaybeGetForId(1)->order_id);
}

TEST_F(BraveAdsTabManagerTest, DoNotSetOrderIdExtractedForUnknownTab) {
  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1));
}

TEST_F(BraveAdsTabManagerTest,
       DoNotSetOrderIdExtractedForTabWithNoKnownRedirectChain) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidStartPlayingMedia(
                                              /*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act
  ads_client_notifier_.NotifyOrderIdExtracted(
      /*tab_id=*/1, /*extracted_for_url=*/GURL("https://brave.com"),
      /*id=*/"336121537");

  // Assert
  EXPECT_FALSE(TabManager::GetInstance().MaybeGetForId(1)->order_id);
}

TEST_F(BraveAdsTabManagerTest,
       DoNotCrashOnFirstNavigationAfterMediaStartedPlayingForUnknownTab) {
  // Arrange
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidStartPlayingMedia(
                                              /*tab_id=*/1));
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  //
  // The tab already exists, so this is a change, not a new tab.
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChange);
  EXPECT_CALL(tab_manager_observer_mock_, OnTabDidChangeFocus);
  ads_client_notifier_.NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_new_navigation=*/true, /*is_restoring=*/false, /*is_visible=*/true);
}

}  // namespace brave_ads
