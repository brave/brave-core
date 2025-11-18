/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/new_tab_page_ad_prefetcher.h"

#include <memory>
#include <optional>

#include "brave/components/brave_ads/core/browser/service/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

NewTabPageAdInfo BuildNewTabPageAd() {
  NewTabPageAdInfo ad;
  ad.type = mojom::AdType::kNewTabPageAd;
  ad.placement_id = "placement_id";
  ad.creative_instance_id = "creative_instance_id";
  ad.creative_set_id = "creative_set_id";
  ad.campaign_id = "campaign_id";
  ad.advertiser_id = "advertiser_id";
  ad.segment = "segment";
  ad.target_url = GURL("https://brave.com");
  ad.company_name = "company_name";
  ad.alt = "alt";
  return ad;
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
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&expected_ad](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(expected_ad);
      });

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, PrefetchFailed) {
  // Arrange
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(/*ad=*/std::nullopt);
      });

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, PrefetchInvalidAd) {
  // Arrange
  const NewTabPageAdInfo invalid_ad;
  ASSERT_FALSE(invalid_ad.IsValid());

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&invalid_ad](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(invalid_ad);
      });

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldPrefetchAdAfterGettingPrefetchedAd) {
  // Arrange
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .Times(2)
      .WillRepeatedly([&expected_ad](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(expected_ad);
      });

  prefetcher().Prefetch();
  ASSERT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldNotPrefetchAdWhenAlreadyPrefetched) {
  // Arrange
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&expected_ad](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(expected_ad);
      });

  prefetcher().Prefetch();

  // Act
  prefetcher().Prefetch();

  // Assert
  EXPECT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest,
       ShouldNotPrefetchAdWhenAnotherPrefetchIsInProgress) {
  // Arrange
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();

  MaybeServeNewTabPageAdCallback deferred_maybe_serve_ad_callback;
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&deferred_maybe_serve_ad_callback](
                    MaybeServeNewTabPageAdCallback callback) {
        deferred_maybe_serve_ad_callback = std::move(callback);
      });

  prefetcher().Prefetch();

  // Act
  prefetcher().Prefetch();
  std::move(deferred_maybe_serve_ad_callback).Run(expected_ad);

  // Assert
  EXPECT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, ShouldOnlyGetPrefetchedAdOnce) {
  // Arrange
  const NewTabPageAdInfo expected_ad = BuildNewTabPageAd();

  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&expected_ad](MaybeServeNewTabPageAdCallback callback) {
        std::move(callback).Run(expected_ad);
      });

  prefetcher().Prefetch();
  ASSERT_EQ(expected_ad, prefetcher().MaybeGetPrefetchedAd());

  // Act & Assert
  EXPECT_FALSE(prefetcher().MaybeGetPrefetchedAd());
}

TEST_F(BraveAdsNewTabPageAdPrefetcherTest, CancelPrefetch) {
  // Arrange
  MaybeServeNewTabPageAdCallback deferred_maybe_serve_ad_callback;
  EXPECT_CALL(ads_service(), MaybeServeNewTabPageAd)
      .WillOnce([&deferred_maybe_serve_ad_callback](
                    MaybeServeNewTabPageAdCallback callback) {
        deferred_maybe_serve_ad_callback = std::move(callback);
      });

  // Act
  prefetcher().Prefetch();
  ResetPrefetcher();

  // Assert
  // Run the deferred callback and make sure no crash occurs
  std::move(deferred_maybe_serve_ad_callback).Run(/*ad=*/std::nullopt);
}

}  // namespace brave_ads
