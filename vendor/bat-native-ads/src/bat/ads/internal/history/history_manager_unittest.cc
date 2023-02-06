/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_manager.h"

#include "base/containers/circular_deque.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsHistoryManagerTest : public HistoryManagerObserver,
                                 public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    HistoryManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    HistoryManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnHistoryDidChange() override { history_did_change_ = true; }

  void OnDidLikeAd(const AdContentInfo& /*ad_content*/) override {
    did_like_ad_ = true;
  }

  void OnDidDislikeAd(const AdContentInfo& /*ad_content*/) override {
    did_dislike_ad_ = true;
  }

  void OnDidMarkToNoLongerReceiveAdsForCategory(
      const std::string& /*category*/) override {
    did_mark_to_no_longer_receive_ads_for_category_ = true;
  }

  void OnDidMarkToReceiveAdsForCategory(
      const std::string& /*category*/) override {
    did_mark_to_receive_ads_for_category_ = true;
  }

  void OnDidMarkAdAsInappropriate(
      const AdContentInfo& /*ad_content*/) override {
    did_mark_ad_as_inappropriate_ = true;
  }

  void OnDidMarkAdAsAppropriate(const AdContentInfo& /*ad_content*/) override {
    did_mark_ad_as_appropriate_ = true;
  }

  void OnDidSaveAd(const AdContentInfo& /*ad_content*/) override {
    did_save_ad_ = true;
  }

  void OnDidUnsaveAd(const AdContentInfo& /*ad_content*/) override {
    did_unsave_ad_ = true;
  }

  bool history_did_change_ = false;
  bool did_like_ad_ = false;
  bool did_dislike_ad_ = false;
  bool did_mark_to_no_longer_receive_ads_for_category_ = false;
  bool did_mark_to_receive_ads_for_category_ = false;
  bool did_mark_ad_as_inappropriate_ = false;
  bool did_mark_ad_as_appropriate_ = false;
  bool did_save_ad_ = false;
  bool did_unsave_ad_ = false;
};

TEST_F(BatAdsHistoryManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = HistoryManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsHistoryManagerTest, AddNotificationAd) {
  // Arrange
  const NotificationAdInfo ad;

  // Act
  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
  EXPECT_TRUE(history_did_change_);
}

TEST_F(BatAdsHistoryManagerTest, AddNewTabPageAd) {
  // Arrange
  const NewTabPageAdInfo ad;

  // Act
  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
  EXPECT_TRUE(history_did_change_);
}

TEST_F(BatAdsHistoryManagerTest, AddPromotedContentAd) {
  // Arrange
  const PromotedContentAdInfo ad;

  // Act
  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
  EXPECT_TRUE(history_did_change_);
}

TEST_F(BatAdsHistoryManagerTest, AddInlineContentAd) {
  // Arrange
  const InlineContentAdInfo ad;

  // Act
  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
  EXPECT_TRUE(history_did_change_);
}

TEST_F(BatAdsHistoryManagerTest, AddSearchResultAd) {
  // Arrange
  const SearchResultAdInfo ad;

  // Act
  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemList expected_history = {history_item};

  const HistoryItemList& history =
      ClientStateManager::GetInstance()->GetHistory();

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
  EXPECT_TRUE(history_did_change_);
}

TEST_F(BatAdsHistoryManagerTest, LikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->LikeAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_like_ad_);
  EXPECT_FALSE(did_dislike_ad_);
}

TEST_F(BatAdsHistoryManagerTest, DislikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->DislikeAd(history_item.ad_content);

  // Assert
  EXPECT_FALSE(did_like_ad_);
  EXPECT_TRUE(did_dislike_ad_);
}

TEST_F(BatAdsHistoryManagerTest, MarkToNoLongerReceiveAdsForCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->MarkToNoLongerReceiveAdsForCategory(
      ad.segment, CategoryContentOptActionType::kNone);

  // Assert
  EXPECT_TRUE(did_mark_to_no_longer_receive_ads_for_category_);
  EXPECT_FALSE(did_mark_to_receive_ads_for_category_);
}

TEST_F(BatAdsHistoryManagerTest, MarkToReceiveAdsForCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->MarkToReceiveAdsForCategory(
      ad.segment, CategoryContentOptActionType::kNone);

  // Assert
  EXPECT_FALSE(did_mark_to_no_longer_receive_ads_for_category_);
  EXPECT_TRUE(did_mark_to_receive_ads_for_category_);
}

TEST_F(BatAdsHistoryManagerTest, MarkAdAsInappropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->ToggleMarkAdAsInappropriate(
      history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_mark_ad_as_inappropriate_);
  EXPECT_FALSE(did_mark_ad_as_appropriate_);
}

TEST_F(BatAdsHistoryManagerTest, MarkAdAsAppropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  history_item.ad_content.is_flagged = true;

  // Act
  HistoryManager::GetInstance()->ToggleMarkAdAsInappropriate(
      history_item.ad_content);

  // Assert
  EXPECT_FALSE(did_mark_ad_as_inappropriate_);
  EXPECT_TRUE(did_mark_ad_as_appropriate_);
}

TEST_F(BatAdsHistoryManagerTest, SaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  const HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance()->ToggleSavedAd(history_item.ad_content);

  // Assert
  EXPECT_TRUE(did_save_ad_);
  EXPECT_FALSE(did_unsave_ad_);
}

TEST_F(BatAdsHistoryManagerTest, UnsaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad = BuildCreativeNotificationAd();
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryItemInfo history_item =
      HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);
  history_item.ad_content.is_saved = true;

  // Act
  HistoryManager::GetInstance()->ToggleSavedAd(history_item.ad_content);

  // Assert
  EXPECT_FALSE(did_save_ad_);
  EXPECT_TRUE(did_unsave_ad_);
}

}  // namespace ads
