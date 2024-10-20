/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/creative_ad_cache.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

constexpr char kAnotherPlacementId[] = "b1f50335-5b9f-4139-a8cd-dd3d16337096";

class BraveAdsCreativeAdCacheTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    cache_ = std::make_unique<CreativeAdCache>();
  }

  void SimulateOpenTab(const int32_t tab_id) {
    NotifyTabDidChange(tab_id, /*redirect_chain=*/{GURL("https://brave.com")},
                       /*is_new_navigation=*/true, /*is_restoring=*/false,
                       /*is_visible=*/true);
  }

  void SimulateCloseTab(const int32_t tab_id) { NotifyDidCloseTab(tab_id); }

  std::unique_ptr<CreativeAdCache> cache_;
};

TEST_F(BraveAdsCreativeAdCacheTest, AddCreativeAd) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr expected_mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  SimulateOpenTab(/*tab_id=*/1);

  // Act
  cache_->MaybeAdd(test::kPlacementId, expected_mojom_creative_ad->Clone());

  // Assert
  EXPECT_EQ(expected_mojom_creative_ad,
            cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddCreativeAd) {
  // Arrange
  SimulateOpenTab(/*tab_id=*/1);

  // Act
  cache_->MaybeAdd(test::kPlacementId, mojom::CreativeSearchResultAdInfoPtr{});

  // Assert
  EXPECT_FALSE(cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
      test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddInvalidCreativeAd) {
  // Arrange
  SimulateOpenTab(/*tab_id=*/1);

  CreativeAdVariant creative_ad_variant;

  // Act
  cache_->MaybeAdd(test::kPlacementId, std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
      test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddCreativeAdForOccludedTab) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  cache_->MaybeAdd(test::kPlacementId, std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
      test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotGetCreativeAdForMissingPlacementId) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  cache_->MaybeAdd(test::kPlacementId, std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
      test::kMissingPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, GetCreativeAdsForMultipleTabs) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr expected_mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  SimulateOpenTab(/*tab_id=*/1);
  cache_->MaybeAdd(test::kPlacementId, expected_mojom_creative_ad->Clone());
  SimulateOpenTab(/*tab_id=*/2);
  cache_->MaybeAdd(kAnotherPlacementId, expected_mojom_creative_ad->Clone());

  // Assert
  EXPECT_EQ(expected_mojom_creative_ad,
            cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));

  EXPECT_EQ(expected_mojom_creative_ad,
            cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                kAnotherPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, PurgePlacementsOnTabDidClose) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr expected_mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  SimulateOpenTab(/*tab_id=*/1);
  cache_->MaybeAdd(test::kPlacementId, expected_mojom_creative_ad->Clone());

  SimulateOpenTab(/*tab_id=*/2);
  cache_->MaybeAdd(kAnotherPlacementId, expected_mojom_creative_ad->Clone());

  // Act
  SimulateCloseTab(/*tab_id=*/2);

  // Assert
  EXPECT_EQ(expected_mojom_creative_ad,
            cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));

  EXPECT_FALSE(cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
      kAnotherPlacementId));
}

}  // namespace brave_ads
