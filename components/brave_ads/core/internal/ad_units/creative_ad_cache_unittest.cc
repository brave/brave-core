/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/creative_ad_cache.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

constexpr int32_t kTabId = 1;
constexpr int32_t kSecondTabId = 2;
constexpr char kTabUrl[] = "https://brave.com";
constexpr char kTestPlacementId[] = "test-placement-id";
constexpr char kTestSecondPlacementId[] = "test-second-placement-id";
constexpr char kTestNonExistentPlacementId[] = "test-non-existent-placement-id";

class BraveAdsCreativeAdCacheTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    cache_ = std::make_unique<CreativeAdCache>();
  }

  void OpenTab(const int32_t tab_id) {
    NotifyTabDidChange(tab_id, /*redirect_chain=*/{GURL(kTabUrl)},
                       /*is_new_navigation=*/true, /*is_restoring=*/false,
                       /*is_visible=*/true);
  }

  void CloseTab(const int32_t tab_id) { NotifyDidCloseTab(tab_id); }

  std::unique_ptr<CreativeAdCache> cache_;
};

TEST_F(BraveAdsCreativeAdCacheTest, AddNonEmptySearchResultAd) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_expected_search_result_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  OpenTab(kTabId);

  // Act
  cache_->Add(kTestPlacementId, mojom_expected_search_result_ad->Clone());
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);

  // Assert
  ASSERT_TRUE(creative_ad);
  EXPECT_EQ(*creative_ad, mojom_expected_search_result_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, AddEmptySearchResultAd) {
  // Arrange
  OpenTab(kTabId);

  // Act
  cache_->Add(kTestPlacementId, mojom::CreativeSearchResultAdInfoPtr{});
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);

  // Assert
  ASSERT_TRUE(creative_ad);
  EXPECT_FALSE(*creative_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, AddDefaultInitializedCreativeAdVariant) {
  // Arrange
  OpenTab(kTabId);
  CreativeAdVariant creative_ad_variant;

  // Act
  cache_->Add(kTestPlacementId, std::move(creative_ad_variant));
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);

  // Assert
  ASSERT_TRUE(creative_ad);
  EXPECT_FALSE(*creative_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, AddNoVisibleTab) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  cache_->Add(kTestPlacementId, std::move(creative_ad_variant));
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);

  // Assert
  ASSERT_FALSE(creative_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, MaybeGetNonExistent) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  cache_->Add(kTestPlacementId, std::move(creative_ad_variant));
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          kTestNonExistentPlacementId);

  // Assert
  ASSERT_FALSE(creative_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, OpenTwoTabs) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_expected_search_result_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  OpenTab(kTabId);
  cache_->Add(kTestPlacementId, mojom_expected_search_result_ad->Clone());
  OpenTab(kSecondTabId);
  cache_->Add(kTestSecondPlacementId, mojom_expected_search_result_ad->Clone());

  // Assert
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);
  ASSERT_TRUE(creative_ad);
  EXPECT_EQ(*creative_ad, mojom_expected_search_result_ad);

  const std::optional<mojom::CreativeSearchResultAdInfoPtr> second_creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          kTestSecondPlacementId);
  ASSERT_TRUE(second_creative_ad);
  EXPECT_EQ(*second_creative_ad, mojom_expected_search_result_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, OnDidCloseTab) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_expected_search_result_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  OpenTab(kTabId);
  cache_->Add(kTestPlacementId, mojom_expected_search_result_ad->Clone());

  // Act
  CloseTab(kTabId);

  // Assert
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);
  ASSERT_FALSE(creative_ad);
}

TEST_F(BraveAdsCreativeAdCacheTest, OnDidCloseTabWithoutPlacements) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_expected_search_result_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  OpenTab(kTabId);
  cache_->Add(kTestPlacementId, mojom_expected_search_result_ad->Clone());
  OpenTab(kSecondTabId);

  // Act
  CloseTab(kSecondTabId);

  // Assert
  const std::optional<mojom::CreativeSearchResultAdInfoPtr> creative_ad =
      cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(kTestPlacementId);
  ASSERT_TRUE(creative_ad);
  ASSERT_EQ(*creative_ad, mojom_expected_search_result_ad);
}

}  // namespace brave_ads
