/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/creative_ad_cache.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeAdCacheTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    creative_ad_cache_ = std::make_unique<CreativeAdCache>();
  }

  void SimulateOpenTab(const int32_t tab_id) {
    NotifyTabDidChange(tab_id, /*redirect_chain=*/{GURL("https://brave.com")},
                       /*is_new_navigation=*/true, /*is_restoring=*/false,
                       /*is_visible=*/true);
  }

  void SimulateCloseTab(const int32_t tab_id) { NotifyDidCloseTab(tab_id); }

  std::unique_ptr<CreativeAdCache> creative_ad_cache_;
};

TEST_F(BraveAdsCreativeAdCacheTest, AddCreativeAd) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  SimulateOpenTab(/*tab_id=*/1);

  // Act
  creative_ad_cache_->MaybeAdd(test::kPlacementId, mojom_creative_ad->Clone());

  // Assert
  EXPECT_EQ(mojom_creative_ad,
            creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddCreativeAd) {
  // Arrange
  SimulateOpenTab(/*tab_id=*/1);

  // Act
  creative_ad_cache_->MaybeAdd(test::kPlacementId,
                               mojom::CreativeSearchResultAdInfoPtr{});

  // Assert
  EXPECT_FALSE(
      creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddInvalidCreativeAd) {
  // Arrange
  SimulateOpenTab(/*tab_id=*/1);

  CreativeAdVariant creative_ad_variant;

  // Act
  creative_ad_cache_->MaybeAdd(test::kPlacementId,
                               std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(
      creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotAddCreativeAdForOccludedTab) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  creative_ad_cache_->MaybeAdd(test::kPlacementId,
                               std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(
      creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          test::kPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, DoNotGetCreativeAdForMissingPlacementId) {
  // Arrange
  CreativeAdVariant creative_ad_variant(
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true));

  // Act
  creative_ad_cache_->MaybeAdd(test::kPlacementId,
                               std::move(creative_ad_variant));

  // Assert
  EXPECT_FALSE(
      creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          test::kMissingPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, GetCreativeAdsAcrossMultipleTabs) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  // Act
  SimulateOpenTab(/*tab_id=*/1);
  creative_ad_cache_->MaybeAdd(test::kPlacementId, mojom_creative_ad->Clone());
  SimulateOpenTab(/*tab_id=*/2);
  creative_ad_cache_->MaybeAdd(test::kAnotherPlacementId,
                               mojom_creative_ad->Clone());

  // Assert
  EXPECT_EQ(mojom_creative_ad,
            creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));
  EXPECT_EQ(mojom_creative_ad,
            creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kAnotherPlacementId));
}

TEST_F(BraveAdsCreativeAdCacheTest, PurgePlacementsOnTabDidClose) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*should_generate_random_uuids=*/true);

  SimulateOpenTab(/*tab_id=*/1);
  creative_ad_cache_->MaybeAdd(test::kPlacementId, mojom_creative_ad->Clone());

  SimulateOpenTab(/*tab_id=*/2);
  creative_ad_cache_->MaybeAdd(test::kAnotherPlacementId,
                               mojom_creative_ad->Clone());

  // Act
  SimulateCloseTab(/*tab_id=*/2);

  // Assert
  EXPECT_EQ(mojom_creative_ad,
            creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
                test::kPlacementId));
  EXPECT_FALSE(
      creative_ad_cache_->MaybeGet<mojom::CreativeSearchResultAdInfoPtr>(
          test::kAnotherPlacementId));
}

}  // namespace brave_ads
