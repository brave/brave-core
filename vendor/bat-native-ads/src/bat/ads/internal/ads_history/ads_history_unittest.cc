/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history.h"

#include <deque>

#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsHistoryTest : public UnitTestBase {
 protected:
  BatAdsAdsHistoryTest() = default;

  ~BatAdsAdsHistoryTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    AdvanceClock(base::Days(history::kForDays));
  }
};

TEST_F(BatAdsAdsHistoryTest, AddAdNotification) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddAdNotificationsToHistory) {
  // Arrange
  AdNotificationInfo ad;

  // Act
  history::AddAdNotification(ad, ConfirmationType::kViewed);
  history::AddAdNotification(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddNewTabPageAd) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddNewTabPageAdWithMultipleEvents) {
  // Arrange
  NewTabPageAdInfo ad;

  // Act
  history::AddNewTabPageAd(ad, ConfirmationType::kViewed);
  history::AddNewTabPageAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddPromotedContentAd) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddPromotedContentWithMultipleEvents) {
  // Arrange
  PromotedContentAdInfo ad;

  // Act
  history::AddPromotedContentAd(ad, ConfirmationType::kViewed);
  history::AddPromotedContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddInlineContentAd) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddInlineContentWithMultipleEvents) {
  // Arrange
  InlineContentAdInfo ad;

  // Act
  history::AddInlineContentAd(ad, ConfirmationType::kViewed);
  history::AddInlineContentAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddSearchResultAd) {
  // Arrange
  SearchResultAdInfo ad;

  // Act
  history::AddSearchResultAd(ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddSearchResultWithMultipleEvents) {
  // Arrange
  SearchResultAdInfo ad;

  // Act
  history::AddSearchResultAd(ad, ConfirmationType::kViewed);
  history::AddSearchResultAd(ad, ConfirmationType::kClicked);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, AddMultipleAdTypesToHistory) {
  // Arrange

  // Act
  AdNotificationInfo ad_notification;
  history::AddAdNotification(ad_notification, ConfirmationType::kViewed);

  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  InlineContentAdInfo inline_content_ad;
  history::AddInlineContentAd(inline_content_ad, ConfirmationType::kViewed);

  SearchResultAdInfo search_result_ad;
  history::AddSearchResultAd(search_result_ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(5UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, PurgedHistoryEntriesOnOrAfter30Days) {
  // Arrange
  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  AdvanceClock(base::Days(30) + base::Seconds(1));

  // Act
  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(1UL, history.size());
}

TEST_F(BatAdsAdsHistoryTest, DoNotPurgedHistoryEntriesBefore30Days) {
  // Arrange
  NewTabPageAdInfo new_tab_page_ad;
  history::AddNewTabPageAd(new_tab_page_ad, ConfirmationType::kViewed);

  AdvanceClock(base::Days(30));

  // Act
  PromotedContentAdInfo promoted_content_ad;
  history::AddPromotedContentAd(promoted_content_ad, ConfirmationType::kViewed);

  // Assert
  const std::deque<AdHistoryInfo> history = Client::Get()->GetAdsHistory();
  ASSERT_EQ(2UL, history.size());
}

}  // namespace ads
