/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"

#include "base/scoped_observation.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/test/search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"
#include "brave/components/brave_ads/core/internal/history/test/ad_history_manager_observer_mock.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdHistoryManagerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    observation_.Observe(&AdHistoryManager::GetInstance());
  }

  AdHistoryManagerObserverMock history_manager_observer_mock_;
  base::ScopedObservation<AdHistoryManager, AdHistoryManagerObserver>
      observation_{&history_manager_observer_mock_};
};

TEST_F(BraveAdsAdHistoryManagerTest, AddNotificationAdHistory) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act & Assert
  const AdHistoryItemInfo expected_ad_history_item = BuildAdHistoryItem(
      ad, mojom::ConfirmationType::kViewedImpression, ad.title, ad.body);
  EXPECT_CALL(history_manager_observer_mock_,
              OnDidAddAdHistoryItem(expected_ad_history_item));
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest,
       DoNotAddNotificationAdHistoryForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/true);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);

  // Act & Assert
  EXPECT_CALL(history_manager_observer_mock_, OnDidAddAdHistoryItem).Times(0);
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest, AddNewTabPageAdHistory) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act & Assert
  const AdHistoryItemInfo expected_ad_history_item = BuildAdHistoryItem(
      ad, mojom::ConfirmationType::kViewedImpression, ad.company_name, ad.alt);
  EXPECT_CALL(history_manager_observer_mock_,
              OnDidAddAdHistoryItem(expected_ad_history_item));
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest,
       DoNotAddNewTabPageAdHistoryForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*use_random_uuids=*/true);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad);

  // Act & Assert
  EXPECT_CALL(history_manager_observer_mock_, OnDidAddAdHistoryItem).Times(0);
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest, AddSearchResultAdHistory) {
  // Arrange
  const SearchResultAdInfo ad =
      test::BuildSearchResultAd(/*use_random_uuids=*/true);

  // Act & Assert
  const AdHistoryItemInfo expected_ad_history_item =
      BuildAdHistoryItem(ad, mojom::ConfirmationType::kViewedImpression,
                         ad.headline_text, ad.description);
  EXPECT_CALL(history_manager_observer_mock_,
              OnDidAddAdHistoryItem(expected_ad_history_item));
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest,
       DoNotAddSearchResultAdHistoryForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const SearchResultAdInfo ad =
      test::BuildSearchResultAd(/*use_random_uuids=*/true);

  // Act & Assert
  EXPECT_CALL(history_manager_observer_mock_, OnDidAddAdHistoryItem).Times(0);
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);
}

TEST_F(BraveAdsAdHistoryManagerTest, GetForUIReturnsMojomItems) {
  // Arrange
  const CreativeNotificationAdInfo creative_ad =
      test::BuildCreativeNotificationAd(/*use_random_uuids=*/false);
  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  AdHistoryManager::GetInstance().Add(
      ad, mojom::ConfirmationType::kViewedImpression);

  // Act
  base::test::TestFuture<
      std::optional<std::vector<mojom::AdHistoryItemInfoPtr>>>
      test_future;
  AdHistoryManager::GetForUI(base::Time::Min(), base::Time::Max(),
                             test_future.GetCallback());
  const auto& ad_history = test_future.Get();

  // Assert
  ASSERT_TRUE(ad_history);
  ASSERT_EQ(1U, ad_history->size());
  const mojom::AdHistoryItemInfo& item = *ad_history->at(0);
  EXPECT_EQ(mojom::AdType::kNotificationAd, item.type);
  EXPECT_EQ(mojom::ConfirmationType::kViewedImpression, item.confirmation_type);
  EXPECT_EQ(creative_ad.creative_instance_id, item.creative_instance_id);
  EXPECT_EQ(creative_ad.creative_set_id, item.creative_set_id);
  EXPECT_EQ(creative_ad.campaign_id, item.campaign_id);
  EXPECT_EQ(creative_ad.advertiser_id, item.advertiser_id);
  EXPECT_EQ(creative_ad.segment, item.segment);
  EXPECT_EQ(ad.title, item.title);
  EXPECT_EQ(ad.body, item.description);
  EXPECT_EQ(mojom::ReactionType::kNeutral, item.like_ad_reaction);
  EXPECT_FALSE(item.is_saved);
  EXPECT_FALSE(item.is_flagged);
  EXPECT_EQ(mojom::ReactionType::kNeutral, item.like_segment_reaction);
}

TEST_F(BraveAdsAdHistoryManagerTest, GetForUIReturnsEmptyVectorWithNoHistory) {
  // Act
  base::test::TestFuture<
      std::optional<std::vector<mojom::AdHistoryItemInfoPtr>>>
      test_future;
  AdHistoryManager::GetForUI(base::Time::Min(), base::Time::Max(),
                             test_future.GetCallback());
  const auto& ad_history = test_future.Get();

  // Assert
  ASSERT_TRUE(ad_history);
  EXPECT_TRUE(ad_history->empty());
}

}  // namespace brave_ads
