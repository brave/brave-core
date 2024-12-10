/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsInlineContentAdTest
    : public test::BraveAdsConversionsTestBase {};

TEST_F(BraveAdsConversionsInlineContentAdTest,
       ConvertViewedAdIfOptedInToInlineContentAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
  run_loop.Run();
}

TEST_F(BraveAdsConversionsInlineContentAdTest,
       DoNotConvertViewedAdIfOptedOutOfInlineContentAdsß) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsInlineContentAdTest,
       ConvertViewedAdForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
  run_loop.Run();
}

TEST_F(BraveAdsConversionsInlineContentAdTest,
       ConvertClickedAdIfOptedInToInlineContentAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kClickThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
  run_loop.Run();
}

TEST_F(BraveAdsConversionsInlineContentAdTest,
       DoNotConvertClickedAdIfOptedOutOfInlineContentAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsInlineContentAdTest,
       ConvertClickedAdForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad = test::BuildAd(mojom::AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kClickThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
  run_loop.Run();
}

}  // namespace brave_ads
