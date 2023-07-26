/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_manager.h"

#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/history/history_item_util.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "brave/components/brave_ads/core/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsHistoryManagerTest : public HistoryManagerObserver,
                                   public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    HistoryManager::GetInstance().AddObserver(this);
  }

  void TearDown() override {
    HistoryManager::GetInstance().RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnDidAddHistory(const HistoryItemInfo& history_item) override {
    history_item_ = history_item;
    did_add_history_ = true;
  }

  void OnDidLikeAd(const AdContentInfo& /*ad_content*/) override {
    did_like_ad_ = true;
  }

  void OnDidDislikeAd(const AdContentInfo& /*ad_content*/) override {
    did_dislike_ad_ = true;
  }

  void OnDidLikeCategory(const std::string& /*category*/) override {
    did_like_category_ = true;
  }

  void OnDidDislikeCategory(const std::string& /*category*/) override {
    did_dislike_category_ = true;
  }

  void OnDidSaveAd(const AdContentInfo& /*ad_content*/) override {
    did_save_ad_ = true;
  }

  void OnDidUnsaveAd(const AdContentInfo& /*ad_content*/) override {
    did_unsave_ad_ = true;
  }

  void OnDidMarkAdAsAppropriate(const AdContentInfo& /*ad_content*/) override {
    did_mark_ad_as_appropriate_ = true;
  }

  void OnDidMarkAdAsInappropriate(
      const AdContentInfo& /*ad_content*/) override {
    did_mark_ad_as_inappropriate_ = true;
  }

  HistoryItemInfo history_item_;
  bool did_add_history_ = false;

  bool did_like_ad_ = false;
  bool did_dislike_ad_ = false;

  bool did_like_category_ = false;
  bool did_dislike_category_ = false;

  bool did_save_ad_ = false;
  bool did_unsave_ad_ = false;

  bool did_mark_ad_as_inappropriate_ = false;
  bool did_mark_ad_as_appropriate_ = false;
};

TEST_F(BraveAdsHistoryManagerTest, AddNotificationAdHistory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  EXPECT_EQ(expected_history_item, history_item_);
  EXPECT_TRUE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddNotificationAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest, AddNewTabPageAdHistory) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_uuids*/ true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.company_name, ad.alt);
  EXPECT_EQ(expected_history_item, history_item_);
  EXPECT_TRUE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddNewTabPageAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_uuids*/ true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest, AddPromotedContentAdHistory) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_uuids*/ true);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.description);
  EXPECT_EQ(expected_history_item, history_item_);
  EXPECT_TRUE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddPromotedContentAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAd(/*should_use_random_uuids*/ true);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest, AddInlineContentAdHistory) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_uuids*/ true);
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.description);
  EXPECT_EQ(expected_history_item, history_item_);
  EXPECT_TRUE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddInlineContentAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(/*should_use_random_uuids*/ true);
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest, AddSearchResultAdHistory) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_uuids*/ true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item = BuildHistoryItem(
      ad, ConfirmationType::kViewed, ad.headline_text, ad.description);
  EXPECT_EQ(expected_history_item, history_item_);
  EXPECT_TRUE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddSearchResultAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAd(/*should_use_random_uuids*/ true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(did_add_history_);
}

TEST_F(BraveAdsHistoryManagerTest, LikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().LikeAd(history_item_.ad_content);

  // Assert
  EXPECT_TRUE(did_like_ad_);
  EXPECT_FALSE(did_dislike_ad_);
}

TEST_F(BraveAdsHistoryManagerTest, DislikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().DislikeAd(history_item_.ad_content);

  // Assert
  EXPECT_FALSE(did_like_ad_);
  EXPECT_TRUE(did_dislike_ad_);
}

TEST_F(BraveAdsHistoryManagerTest, LikeCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().LikeCategory(history_item_.category_content);

  // Assert
  EXPECT_FALSE(did_dislike_category_);
  EXPECT_TRUE(did_like_category_);
}

TEST_F(BraveAdsHistoryManagerTest, DislikeCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().DislikeCategory(history_item_.category_content);

  // Assert
  EXPECT_TRUE(did_dislike_category_);
  EXPECT_FALSE(did_like_category_);
}

TEST_F(BraveAdsHistoryManagerTest, SaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().ToggleSaveAd(history_item_.ad_content);

  // Assert
  EXPECT_TRUE(did_save_ad_);
  EXPECT_FALSE(did_unsave_ad_);
}

TEST_F(BraveAdsHistoryManagerTest, UnsaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  history_item_.ad_content.is_saved = true;

  // Act
  HistoryManager::GetInstance().ToggleSaveAd(history_item_.ad_content);

  // Assert
  EXPECT_FALSE(did_save_ad_);
  EXPECT_TRUE(did_unsave_ad_);
}

TEST_F(BraveAdsHistoryManagerTest, MarkAdAsInappropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
      history_item_.ad_content);

  // Assert
  EXPECT_TRUE(did_mark_ad_as_inappropriate_);
  EXPECT_FALSE(did_mark_ad_as_appropriate_);
}

TEST_F(BraveAdsHistoryManagerTest, MarkAdAsAppropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAd(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);
  history_item_.ad_content.is_flagged = true;

  // Act
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(
      history_item_.ad_content);

  // Assert
  EXPECT_FALSE(did_mark_ad_as_inappropriate_);
  EXPECT_TRUE(did_mark_ad_as_appropriate_);
}

}  // namespace brave_ads
