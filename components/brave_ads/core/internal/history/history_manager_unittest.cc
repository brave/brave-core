/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_manager.h"

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
#include "brave/components/brave_ads/core/internal/history/history_manager_observer_unittest_helper.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/history/category_content_info.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsHistoryManagerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    HistoryManager::GetInstance().AddObserver(&observer_);
  }

  void TearDown() override {
    HistoryManager::GetInstance().RemoveObserver(&observer_);

    UnitTestBase::TearDown();
  }

  HistoryManagerObserverForTesting observer_;
};

TEST_F(BraveAdsHistoryManagerTest, AddNotificationAdHistory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.body);
  EXPECT_EQ(expected_history_item, observer_.history_item());
  EXPECT_TRUE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddNotificationAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest, AddNewTabPageAdHistory) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.company_name, ad.alt);
  EXPECT_EQ(expected_history_item, observer_.history_item());
  EXPECT_TRUE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddNewTabPageAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAdForTesting(/*should_use_random_uuids*/ true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest, AddPromotedContentAdHistory) {
  // Arrange
  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAdForTesting(
          /*should_use_random_uuids*/ true);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.description);
  EXPECT_EQ(expected_history_item, observer_.history_item());
  EXPECT_TRUE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddPromotedContentAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  const CreativePromotedContentAdInfo creative_ad =
      BuildCreativePromotedContentAdForTesting(
          /*should_use_random_uuids*/ true);
  const PromotedContentAdInfo ad = BuildPromotedContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest, AddInlineContentAdHistory) {
  // Arrange
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, ad.title, ad.description);
  EXPECT_EQ(expected_history_item, observer_.history_item());
  EXPECT_TRUE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddInlineContentAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAdForTesting(/*should_use_random_uuids*/ true);
  const InlineContentAdInfo ad = BuildInlineContentAd(creative_ad);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest, AddSearchResultAdHistory) {
  // Arrange
  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  const HistoryItemInfo expected_history_item = BuildHistoryItem(
      ad, ConfirmationType::kViewed, ad.headline_text, ad.description);
  EXPECT_EQ(expected_history_item, observer_.history_item());
  EXPECT_TRUE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest,
       DoNotAddSearchResultAdHistoryForNonRewardsUser) {
  // Arrange
  DisableBraveRewardsForTesting();

  const mojom::SearchResultAdInfoPtr ad_mojom =
      BuildSearchResultAdForTesting(/*should_use_random_uuids*/ true);
  const SearchResultAdInfo ad = BuildSearchResultAd(ad_mojom);

  // Act
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Assert
  EXPECT_FALSE(observer_.did_add_history());
}

TEST_F(BraveAdsHistoryManagerTest, LikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  HistoryManager::GetInstance().Add(BuildNotificationAd(creative_ad),
                                    ConfirmationType::kViewed);
  ASSERT_TRUE(observer_.history_item());

  const AdContentInfo& ad_content = observer_.history_item()->ad_content;

  // Act
  HistoryManager::GetInstance().LikeAd(ad_content);

  // Assert
  EXPECT_TRUE(observer_.did_like_ad());
  EXPECT_FALSE(observer_.did_dislike_ad());
}

TEST_F(BraveAdsHistoryManagerTest, DislikeAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().DislikeAd(observer_.history_item()->ad_content);

  // Assert
  EXPECT_FALSE(observer_.did_like_ad());
  EXPECT_TRUE(observer_.did_dislike_ad());
}

TEST_F(BraveAdsHistoryManagerTest, LikeCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().LikeCategory(
      observer_.history_item()->category_content);

  // Assert
  EXPECT_FALSE(observer_.did_dislike_category());
  EXPECT_TRUE(observer_.did_like_category());
}

TEST_F(BraveAdsHistoryManagerTest, DislikeCategory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  // Act
  HistoryManager::GetInstance().DislikeCategory(
      observer_.history_item()->category_content);

  // Assert
  EXPECT_TRUE(observer_.did_dislike_category());
  EXPECT_FALSE(observer_.did_like_category());
}

TEST_F(BraveAdsHistoryManagerTest, SaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  HistoryManager::GetInstance().Add(BuildNotificationAd(creative_ad),
                                    ConfirmationType::kViewed);
  ASSERT_TRUE(observer_.history_item());

  const AdContentInfo& ad_content = observer_.history_item()->ad_content;

  // Act
  HistoryManager::GetInstance().ToggleSaveAd(ad_content);

  // Assert
  EXPECT_TRUE(observer_.did_save_ad());
  EXPECT_FALSE(observer_.did_unsave_ad());
}

TEST_F(BraveAdsHistoryManagerTest, UnsaveAd) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  HistoryManager::GetInstance().Add(BuildNotificationAd(creative_ad),
                                    ConfirmationType::kViewed);
  ASSERT_TRUE(observer_.history_item());

  AdContentInfo ad_content = observer_.history_item()->ad_content;
  ad_content.is_saved = true;

  // Act
  HistoryManager::GetInstance().ToggleSaveAd(ad_content);

  // Assert
  EXPECT_FALSE(observer_.did_save_ad());
  EXPECT_TRUE(observer_.did_unsave_ad());
}

TEST_F(BraveAdsHistoryManagerTest, MarkAdAsInappropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  HistoryManager::GetInstance().Add(BuildNotificationAd(creative_ad),
                                    ConfirmationType::kViewed);
  ASSERT_TRUE(observer_.history_item());

  const AdContentInfo& ad_content = observer_.history_item()->ad_content;

  // Act
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(ad_content);

  // Assert
  EXPECT_TRUE(observer_.did_mark_ad_as_inappropriate());
  EXPECT_FALSE(observer_.did_mark_ad_as_appropriate());
}

TEST_F(BraveAdsHistoryManagerTest, MarkAdAsAppropriate) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      BuildCreativeNotificationAdForTesting(/*should_use_random_uuids*/ true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  AdContentInfo ad_content = observer_.history_item()->ad_content;
  ad_content.is_flagged = true;

  // Act
  HistoryManager::GetInstance().ToggleMarkAdAsInappropriate(ad_content);

  // Assert
  EXPECT_FALSE(observer_.did_mark_ad_as_inappropriate());
  EXPECT_TRUE(observer_.did_mark_ad_as_appropriate());
}

}  // namespace brave_ads
