/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include <tuple>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/test/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/test/conversions_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "testing/gmock/include/gmock/gmock.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsTest : public test::BraveAdsConversionsTestBase {};

TEST_F(BraveAdsConversionsTest, DoNotCapConversionsWithinDifferentCampaigns) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad_1 = test::BuildAd(mojom::AdType::kNotificationAd,
                                    /*use_random_uuids=*/true);
  test::BuildAndSaveCampaignConversion(ad_1.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad_1, {mojom::ConfirmationType::kServedImpression,
                              mojom::ConfirmationType::kViewedImpression});

  const AdInfo ad_2 = test::BuildAd(mojom::AdType::kSearchResultAd,
                                    /*use_random_uuids=*/true);
  test::BuildAndSaveCampaignConversion(ad_2.campaign_id,
                                       test::kAnotherMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad_2, {mojom::ConfirmationType::kServedImpression,
                              mojom::ConfirmationType::kViewedImpression,
                              mojom::ConfirmationType::kClicked});

  // Act & Assert
  base::RunLoop view_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad_1, ConversionActionType::kViewThrough,
                                  view_through_run_loop.QuitClosure());
  base::RunLoop click_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad_2, ConversionActionType::kClickThrough,
                                  click_through_run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  view_through_run_loop.Run();
  click_through_run_loop.Run();
}

TEST_F(BraveAdsConversionsTest, ConvertAdForMatchingCampaignConversion) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  run_loop.Run();
}

TEST_F(BraveAdsConversionsTest, DoNotCapConversionsWithinTheSameCampaign) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"campaign_conversion_cap", "0"}});

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/true);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  base::RunLoop view_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  view_through_run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  view_through_run_loop.Run();

  // Act & Assert
  base::RunLoop click_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  click_through_run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  click_through_run_loop.Run();
}

TEST_F(BraveAdsConversionsTest, CapConversionsWithinTheSameCampaign) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"campaign_conversion_cap", "2"}});

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/true);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression});

  base::RunLoop view_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  view_through_run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  view_through_run_loop.Run();

  base::RunLoop click_through_run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  click_through_run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  click_through_run_loop.Run();

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest, ConvertViewedAdAfterTheSameAdWasDismissed) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kDismissed});

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  run_loop.Run();
}

TEST_F(BraveAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(
      ad, {mojom::ConfirmationType::kDismissed,
           mojom::ConfirmationType::kServedImpression,
           mojom::ConfirmationType::kLanded,
           mojom::ConfirmationType::kMarkAdAsInappropriate,
           mojom::ConfirmationType::kSavedAd, mojom::ConfirmationType::kLikedAd,
           mojom::ConfirmationType::kDislikedAd,
           mojom::ConfirmationType::kConversion});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdIfThereIsNoCampaignConversion) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoMatchingCampaignConversion) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMismatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfAnotherAdHasConvertedWithinTheSameCampaign) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad_1 = test::BuildAd(mojom::AdType::kNotificationAd,
                                    /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad_1.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad_1, {mojom::ConfirmationType::kServedImpression,
                              mojom::ConfirmationType::kViewedImpression,
                              mojom::ConfirmationType::kDismissed});

  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad_1, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  run_loop.Run();

  AdInfo ad_2 = ad_1;
  ad_2.creative_instance_id = test::kAnotherCreativeInstanceId;
  test::RecordAdEvents(ad_2, {mojom::ConfirmationType::kServedImpression,
                              mojom::ConfirmationType::kViewedImpression,
                              mojom::ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdIfUrlPatternDoesNotMatch) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMismatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvents(ad, {mojom::ConfirmationType::kServedImpression,
                            mojom::ConfirmationType::kViewedImpression,
                            mojom::ConfirmationType::kDismissed});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfCampaignConversionIsOnTheCuspOfExpiring) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3) - base::Milliseconds(1));

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  run_loop.Run();
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheCampaignConversionHasExpired) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(kConversionsFeature);

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3));

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfObservationWindowIsOnTheCuspOfTheAdEventsTimeWindow) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"ad_events_time_window", "30d"}});

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(30));
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(30) - base::Milliseconds(1));

  // Act & Assert
  base::RunLoop run_loop;
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough,
                                  run_loop.QuitClosure());
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
  run_loop.Run();
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheAdEventsTimeWindowHasBeenExceeded) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"ad_events_time_window", "1d"}});

  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  // The observation window is longer than the ad events time window, so the
  // ad event should be purged from the ad events fetch before the
  // observation window is ever checked.
  test::BuildAndSaveCampaignConversion(ad.campaign_id,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain());
}

TEST_F(BraveAdsConversionsTest,
       DoNotFlattenOrderPatternsWithoutMatchingCampaignConversion) {
  // Act
  base::test::TestFuture<std::optional<OrderIdExtractionPatternsInfo>,
                         std::optional<OrderValueExtractionPatternsInfo>>
      future;
  conversions_->MaybeGetOrderPatterns(
      /*tab_id=*/1, test::BuildDefaultConversionRedirectChain(),
      /*detected_language=*/std::nullopt, future.GetCallback());
  const auto [id_patterns, value_patterns] = future.Take();

  // Assert
  EXPECT_FALSE(id_patterns);
  EXPECT_FALSE(value_patterns);
}

TEST_F(BraveAdsConversionsTest,
       DoNotFlattenOrderPatternsWithoutPendingAdEvent) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      test::kMatchingUrlPattern, /*observation_window=*/base::Days(3));
  campaign_conversion.extract_value = true;
  database::SaveCampaignConversions({campaign_conversion});

  // Act
  base::test::TestFuture<std::optional<OrderIdExtractionPatternsInfo>,
                         std::optional<OrderValueExtractionPatternsInfo>>
      future;
  conversions_->MaybeGetOrderPatterns(
      /*tab_id=*/1, test::BuildDefaultConversionRedirectChain(),
      /*detected_language=*/std::nullopt, future.GetCallback());
  const auto [id_patterns, value_patterns] = future.Take();

  // Assert
  EXPECT_FALSE(id_patterns);
  EXPECT_FALSE(value_patterns);
}

TEST_F(BraveAdsConversionsTest,
       FlattenOrderPatternsUsingDefaultsIfResourceNotLoaded) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(ad.campaign_id, test::kMatchingUrlPattern,
                                    /*observation_window=*/base::Days(3));
  campaign_conversion.extract_id = true;
  campaign_conversion.extract_value = true;
  database::SaveCampaignConversions({campaign_conversion});
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  // The resource already started loading at `Conversions` construction
  // time in `SetUp`, against the default mock's fallback to the real test
  // fixture, via a background parse task that may still be in flight.
  // Probe it to completion first, discarding the result, so overriding and
  // reloading below cannot race it.
  base::test::TestFuture<std::optional<OrderIdExtractionPatternsInfo>,
                         std::optional<OrderValueExtractionPatternsInfo>>
      initial_load_future;
  conversions_->MaybeGetOrderPatterns(
      /*tab_id=*/1, test::BuildDefaultConversionRedirectChain(),
      /*detected_language=*/std::nullopt, initial_load_future.GetCallback());
  std::ignore = initial_load_future.Take();

  // Override the mock to simulate the resource becoming unavailable, then
  // force a reload so the override actually takes effect.
  ON_CALL(ads_client_mock_,
          LoadResourceComponent(kOrderExtractionPatternsResourceId,
                                /*version=*/::testing::_,
                                /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        std::move(callback).Run(/*file=*/{}, /*exists=*/false);
      });
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act, the order extraction patterns resource is not loaded at this point.
  base::test::TestFuture<std::optional<OrderIdExtractionPatternsInfo>,
                         std::optional<OrderValueExtractionPatternsInfo>>
      future;
  conversions_->MaybeGetOrderPatterns(
      /*tab_id=*/1, test::BuildDefaultConversionRedirectChain(),
      /*detected_language=*/std::nullopt, future.GetCallback());
  const auto [id_patterns, value_patterns] = future.Take();

  // Assert, falls back to the compiled-in default patterns rather than
  // skipping extraction entirely.
  ASSERT_TRUE(id_patterns);
  EXPECT_FALSE(id_patterns->regexes.empty());
  ASSERT_TRUE(value_patterns);
  EXPECT_FALSE(value_patterns->currency_regexes.empty());
}

}  // namespace brave_ads
