/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer_mock.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kMatchingUrlPattern[] = "https://foo.com/*";
constexpr char kAnotherMatchingUrlPattern[] = "https://qux.com/*/corge";
constexpr char kNonMatchingUrlPattern[] = "https://www.corge.com/grault";

std::vector<GURL> BuildRedirectChain() {
  return {GURL("https://foo.com/bar"), GURL("https://www.baz.com"),
          GURL("https://qux.com/quux/corge")};
}

}  // namespace

class BraveAdsConversionsTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    conversions_ = std::make_unique<Conversions>();
    conversions_->AddObserver(&conversions_observer_mock_);
  }

  void TearDown() override {
    conversions_->RemoveObserver(&conversions_observer_mock_);

    UnitTestBase::TearDown();
  }

  void RecordAdEventsAdvancingTheClockAfterEach(
      const AdInfo& ad,
      const std::vector<ConfirmationType>& confirmation_types) {
    for (const auto& confirmation_type : confirmation_types) {
      test::RecordAdEvent(ad, confirmation_type);

      AdvanceClockBy(base::Milliseconds(1));
    }
  }

  void VerifyConversionExpectation(const AdInfo& ad,
                                   const ConversionActionType action_type) {
    EXPECT_CALL(conversions_observer_mock_,
                OnDidConvertAd(/*conversion*/ ::testing::FieldsAre(
                    ad.type, ad.creative_instance_id, ad.creative_set_id,
                    ad.campaign_id, ad.advertiser_id, ad.segment, action_type,
                    /*verifiable*/ std::nullopt)));
  }

  void VerifyVerifiableConversionExpectation(
      const AdInfo& ad,
      const ConversionActionType action_type,
      const VerifiableConversionInfo& verifiable_conversion) {
    EXPECT_CALL(conversions_observer_mock_,
                OnDidConvertAd(/*conversion*/ ::testing::FieldsAre(
                    ad.type, ad.creative_instance_id, ad.creative_set_id,
                    ad.campaign_id, ad.advertiser_id, ad.segment, action_type,
                    verifiable_conversion)));
  }

  std::unique_ptr<Conversions> conversions_;

  ::testing::StrictMock<ConversionsObserverMock> conversions_observer_mock_;
};

TEST_F(BraveAdsConversionsTest,
       DoNotCapConversionsWithinDifferentCreativeSets) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kInlineContentAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_1.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_1, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression});

  const AdInfo ad_2 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_2.creative_set_id,
                                          kAnotherMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyConversionExpectation(ad_1, ConversionActionType::kViewThrough);
  VerifyConversionExpectation(ad_2, ConversionActionType::kClickThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, DoNotCapConversionsWithinTheSameCreativeSet) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"creative_set_conversion_cap", "0"}});

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, CapConversionsWithinTheSameCreativeSet) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kConversionsFeature, {{"creative_set_conversion_cap", "2"}});

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});

  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, ConvertViewedAdAfterTheSameAdWasDismissed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kDismissed});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kDismissed, ConfirmationType::kServedImpression,
           ConfirmationType::kLanded, ConfirmationType::kMarkAdAsInappropriate,
           ConfirmationType::kSavedAd, ConfirmationType::kLikedAd,
           ConfirmationType::kDislikedAd, ConfirmationType::kConversion});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
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
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoMatchingCreativeSetConversion) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kNonMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfAnotherAdHasConvertedWithinTheSameCreativeSet) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad_1.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_1,
      {ConfirmationType::kServedImpression, ConfirmationType::kViewedImpression,
       ConfirmationType::kDismissed});

  VerifyConversionExpectation(ad_1, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});

  AdInfo ad_2 = ad_1;
  ad_2.creative_instance_id = "1e945c25-98a2-443c-a7f5-e695110d2b84";
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdIfUrlPatternDoesNotMatch) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kNonMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kDismissed});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfCreativeSetConversionIsOnTheCuspOfExpiring) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3) - base::Milliseconds(1));

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheCreativeSetConversionHasExpired) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  AdvanceClockBy(base::Days(3));

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableAdvertiserPublicKeyIsEmpty) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kEmptyVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux_id=xyzzy")},
      /*html=*/{});
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfResourceIdPatternDoesNotMatchRedirectChain) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id,
      /*url_pattern=*/"https://www.baz.com/*",
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://grault.com/garply"),
                          GURL("https://www.baz.com/bar"),
                          GURL("https://qux.com/quux/plugh")},
      /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableUrlConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux=quux")}, /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableUrlConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyVerifiableConversionExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{/*id=*/"xyzzy",
                               kVerifiableConversionAdvertiserPublicKey});
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux_id=xyzzy")},
      /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableHtmlConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar")}, /*html=*/{});
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableHtmlConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyVerifiableConversionExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{/*id=*/"waldo",
                               kVerifiableConversionAdvertiserPublicKey});
  conversions_->MaybeConvert(
      BuildRedirectChain(),
      /*html=*/R"(<html><div id="xyzzy-id">waldo</div></html>)");
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfVerifiableHtmlMetaTagConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kViewThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")}, /*html=*/{});
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfVerifiableHtmlMetaTagConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  VerifyVerifiableConversionExpectation(
      ad, ConversionActionType::kViewThrough,
      VerifiableConversionInfo{/*id=*/"fred",
                               kVerifiableConversionAdvertiserPublicKey});
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")},
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

TEST_F(BraveAdsConversionsTest, VerifiableConversionForRewardsUsers) {
  // Arrange
  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  EXPECT_CALL(
      conversions_observer_mock_,
      OnDidConvertAd(/*conversion*/ ::testing::FieldsAre(
          ad.type, ad.creative_instance_id, ad.creative_set_id, ad.campaign_id,
          ad.advertiser_id, ad.segment, ConversionActionType::kClickThrough,
          VerifiableConversionInfo{/*id=*/"fred",
                                   kVerifiableConversionAdvertiserPublicKey})));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar")},
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultClickThroughConversionForNonRewardsUsers) {
  // Arrange
  test::DisableBraveRewards();

  NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                   kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);
  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);
  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  VerifyConversionExpectation(ad, ConversionActionType::kClickThrough);
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar")},
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

}  // namespace brave_ads
