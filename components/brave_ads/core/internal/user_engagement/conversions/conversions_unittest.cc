/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsTest : public test::BraveAdsConversionsTestBase {};

TEST_F(BraveAdsConversionsTest,
       DoNotCapConversionsWithinDifferentCreativeSets) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kInlineContentAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_1.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_1, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression});

  const AdInfo ad_2 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_2.creative_set_id,
                                          test::kAnotherMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad_1, ConversionActionType::kViewThrough);
  VerifyOnDidConvertAdExpectation(ad_2, ConversionActionType::kClickThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, DoNotCapConversionsWithinTheSameCreativeSet) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"creative_set_conversion_cap", "0"}});

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, CapConversionsWithinTheSameCreativeSet) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"creative_set_conversion_cap", "2"}});

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");

  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, ConvertViewedAdAfterTheSameAdWasDismissed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kDismissed});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kDismissed, ConfirmationType::kServedImpression,
           ConfirmationType::kLanded, ConfirmationType::kMarkAdAsInappropriate,
           ConfirmationType::kSavedAd, ConfirmationType::kLikedAd,
           ConfirmationType::kDislikedAd, ConfirmationType::kConversion});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoCreativeSetConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoMatchingCreativeSetConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMismatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfAnotherAdHasConvertedWithinTheSameCreativeSet) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad_1.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_1,
      {ConfirmationType::kServedImpression, ConfirmationType::kViewedImpression,
       ConfirmationType::kDismissed});

  VerifyOnDidConvertAdExpectation(ad_1, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");

  AdInfo ad_2 = ad_1;
  ad_2.creative_instance_id = "1e945c25-98a2-443c-a7f5-e695110d2b84";
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdIfUrlPatternDoesNotMatch) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMismatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kDismissed});

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfCreativeSetConversionIsOnTheCuspOfExpiring) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3) - base::Milliseconds(1));

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheCreativeSetConversionHasExpired) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          test::kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3));

  // Act & Assert
  VerifyOnDidNotConvertAdExpectation();
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableAdvertiserPublicKeyIsEmpty) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      /*verifiable_advertiser_public_key_base64=*/"");
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildVerifiableConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfResourceIdPatternDoesNotMatchRedirectChain) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableUrlConversionIdDoesNotExist) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux=quux")}, /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableUrlConversionIdExists) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertVerifiableAdExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{
          /*id=*/"xyzzy",
          test::kVerifiableConversionAdvertiserPublicKeyBase64});
  conversions_->MaybeConvert(test::BuildVerifiableConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableHtmlConversionIdDoesNotExist) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(test::BuildDefaultConversionRedirectChain(),
                             /*html=*/"");
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableHtmlConversionIdExists) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertVerifiableAdExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{
          /*id=*/"waldo",
          test::kVerifiableConversionAdvertiserPublicKeyBase64});
  conversions_->MaybeConvert(
      test::BuildDefaultConversionRedirectChain(),
      /*html=*/R"(<html><div id="xyzzy-id">waldo</div></html>)");
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfVerifiableHtmlMetaTagConversionIdDoesNotExist) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")}, /*html=*/"");
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfVerifiableHtmlMetaTagConversionIdExists) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyOnDidConvertVerifiableAdExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{
          /*id=*/"fred", test::kVerifiableConversionAdvertiserPublicKeyBase64});
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")},
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

TEST_F(BraveAdsConversionsTest, VerifiableConversion) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidConvertVerifiableAdExpectation(
      ad, ConversionActionType::kClickThrough,
      VerifiableConversionInfo{
          /*id=*/"fred", test::kVerifiableConversionAdvertiserPublicKeyBase64});
  conversions_->MaybeConvert(
      test::BuildDefaultConversionRedirectChain(),
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

TEST_F(BraveAdsConversionsTest, FallbackToDefaultConversionForNonRewardsUser) {
  // Arrange
  const base::test::ScopedFeatureList scoped_feature_list(
      kShouldAlwaysTriggerBraveSearchResultAdEventsFeature);

  test::DisableBraveRewards();

  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, test::kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  // We only record ad clicked and conversion events for non-Rewards users.
  RecordAdEventsAdvancingTheClockAfterEach(ad, {ConfirmationType::kClicked});

  // Act & Assert
  VerifyOnDidConvertAdExpectation(ad, ConversionActionType::kClickThrough);
  conversions_->MaybeConvert(
      test::BuildDefaultConversionRedirectChain(),
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

}  // namespace brave_ads
