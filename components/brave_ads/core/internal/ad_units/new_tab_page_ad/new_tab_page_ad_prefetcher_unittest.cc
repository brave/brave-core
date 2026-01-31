/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_prefetcher.h"

#include <memory>

#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

mojom::NewTabPageAdInfoPtr BuildNewTabPageAd() {
  mojom::NewTabPageAdInfoPtr mojom_ad = mojom::NewTabPageAdInfo::New();
  mojom_ad->placement_id = "placement_id";
  mojom_ad->creative_instance_id = "creative_instance_id";
  mojom_ad->creative_set_id = "creative_set_id";
  mojom_ad->campaign_id = "campaign_id";
  mojom_ad->advertiser_id = "advertiser_id";
  mojom_ad->segment = "segment";
  mojom_ad->target_url = GURL("https://brave.com");
  mojom_ad->company_name = "company_name";
  mojom_ad->alt = "alt";
  return mojom_ad;
}

}  // namespace

class BraveAdsNewTabPageAdPrefetcherTest : public ::testing::Test {
 public:
  BraveAdsNewTabPageAdPrefetcherTest()
      : ads_service_(std::make_unique<AdsServiceMock>()),
        prefetcher_(std::make_unique<NewTabPageAdPrefetcher>(*ads_service_)) {}

 protected:
  AdsServiceMock& ads_service() const { return *ads_service_; }

  NewTabPageAdPrefetcher& prefetcher() const { return *prefetcher_; }

  void ResetPrefetcher() {
    prefetcher_ = std::make_unique<NewTabPageAdPrefetcher>(*ads_service_);
  }

 private:
  std::unique_ptr<AdsServiceMock> ads_service_;
  std::unique_ptr<NewTabPageAdPrefetcher> prefetcher_;
};

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, NoAdWithoutPrefetch) {
  // Act & Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, Prefetch) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr expected_mojom_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce(
          [&expected_mojom_ad](MaybeServeMojomNewTabPageAdCallback callback) {
            std::move(callback).Run(expected_mojom_ad.Clone());
          });

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, PrefetchFailed) {
  // Arrange
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([](MaybeServeMojomNewTabPageAdCallback callback) {
        std::move(callback).Run(/*ad=*/nullptr);
      });

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldPrefetchAdAfterGettingPrefetchedAd) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr expected_mojom_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .Times(2)
      .WillRepeatedly(
          [&expected_mojom_ad](MaybeServeMojomNewTabPageAdCallback callback) {
            std::move(callback).Run(expected_mojom_ad.Clone());
          });

  prefetcher().Prefetch();
  ASSERT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldNotPrefetchAdWhenAlreadyPrefetched) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr expected_mojom_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce(
          [&expected_mojom_ad](MaybeServeMojomNewTabPageAdCallback callback) {
            std::move(callback).Run(expected_mojom_ad.Clone());
          });

  prefetcher().Prefetch();

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldNotPrefetchAdWhenAnotherPrefetchIsInProgress) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr expected_mojom_ad = BuildNewTabPageAd();

  MaybeServeMojomNewTabPageAdCallback deferred_maybe_serve_ad_callback;
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&deferred_maybe_serve_ad_callback](
                    MaybeServeMojomNewTabPageAdCallback callback) {
        deferred_maybe_serve_ad_callback = std::move(callback);
      });

  prefetcher().Prefetch();

  // Act
  prefetcher().Prefetch();
  std::move(deferred_maybe_serve_ad_callback).Run(expected_mojom_ad.Clone());

  // Assert
  EXPECT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, ShouldOnlyGetPrefetchedAdOnce) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr expected_mojom_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce(
          [&expected_mojom_ad](MaybeServeMojomNewTabPageAdCallback callback) {
            std::move(callback).Run(expected_mojom_ad.Clone());
          });

  prefetcher().Prefetch();
  ASSERT_EQ(expected_mojom_ad, prefetcher().MaybeGetPrefetchedAd());

  // Act & Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, CancelPrefetch) {
  // Arrange
  MaybeServeMojomNewTabPageAdCallback deferred_maybe_serve_ad_callback;
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&deferred_maybe_serve_ad_callback](
                    MaybeServeMojomNewTabPageAdCallback callback) {
        deferred_maybe_serve_ad_callback = std::move(callback);
      });

  // Act
  prefetcher().Prefetch();
  ResetPrefetcher();

  // Assert
  // Run the deferred callback and make sure no crash occurs
  std::move(deferred_maybe_serve_ad_callback).Run(/*ad=*/nullptr);
}

}  // namespace brave_ads
