/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history.h"

#include "base/containers/circular_deque.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/history/history_constants.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsHistoryTest : public UnitTestBase {
 protected:
  BatAdsHistoryTest() = default;

  ~BatAdsHistoryTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    AdvanceClockBy(base::Days(history::kDays));
  }
};

TEST_F(BatAdsHistoryTest, AddNotificationAd) {
  // Arrange
  NotificationAdInfo ad;

  // Act
  history::AddNotificationAd(ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddNotificationAdsToHistory) {
  // Arrange
  NotificationAdInfo ad;

  // Act
  history::AddNotificationAd(ad, ConfirmationType::kViewed);
  history::AddNotificationAd(ad, ConfirmationType::kClicked);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddNewTabPageAd) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddNewTabPageAdWithMultipleEvents) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);
  history::AddNewTabPageAd(ad, ConfirmationType::kClicked);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddPromotedContentAd) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddPromotedContentWithMultipleEvents) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);
  history::AddPromotedContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddInlineContentAd) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddInlineContentWithMultipleEvents) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);
  history::AddInlineContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddSearchResultAd) {
  // Arrange
  SearchResultAdInfo ad;

  // Act
  history::AddSearchResultAd(ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddSearchResultWithMultipleEvents) {
  // Arrange
  SearchResultAdInfo ad;

  // Act
  history::AddSearchResultAd(ad, ConfirmationType::kViewed);
  history::AddSearchResultAd(ad, ConfirmationType::kClicked);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsHistoryTest, AddMultipleAdTypesToHistory) {
  // Arrange

  // Act
  NotificationAdInfo notification_ad;
  history::AddNotificationAd(notification_ad, ConfirmationType::kViewed);

  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  InlineContentAdInfo inline_content_ad;
  history::AddInlineContentAd(inline_content_ad, ConfirmationType::kViewed);

  SearchResultAdInfo search_result_ad;
  history::AddSearchResultAd(search_result_ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(5UL, history.size());
}

TEST_F(BatAdsHistoryTest, PurgeHistoryItemsOlderThan30Days) {
  // Arrange
  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  AdvanceClockBy(base::Days(30) + base::Seconds(1));

  // Act
  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsHistoryTest, DoNotPurgeHistoryItemsOnOrBefore30Days) {
  // Arrange
  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  AdvanceClockBy(base::Days(30));

  // Act
  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  // Assert
  const base::circular_deque<HistoryItemInfo> history =
      ClientStateManager::Get()->GetHistory();
  ASSERT_EQ(2UL, history.size());
}

}  // namespace ads
