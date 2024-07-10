/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer_mock.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kMatchingUrlPattern[] = "https://foo.com/*";
constexpr char kAnotherMatchingUrlPattern[] = "https://qux.com/*/corge";
constexpr char kNonMatchingUrlPattern[] = "https://www.corge.com/grault";

constexpr char kHtml[] = "<html>Hello World!</html>";

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

  std::unique_ptr<Conversions> conversions_;

  ::testing::StrictMock<ConversionsObserverMock> conversions_observer_mock_;
};

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedInlineContentAdIfOptedOutOFBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedInlineContentAdIfOptedInToBraveNewsAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedInlineContentAdIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedInlineContentAdIfOptedInToBraveNewsAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kInlineContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedNewTabPageAdIfOptedOutOfNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  const AdInfo ad = test::BuildAd(AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedNewTabPageAdIfOptedInToNewTabPageAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedNewTabPageAdIfOptedOutOfNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  const AdInfo ad = test::BuildAd(AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedNewTabPageAdIfOptedInToNewTabPageAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNewTabPageAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedNotificationAdIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedNotificationAdIfOptedInToNotificationAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedNotificationAdIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedNotificationAdIfOptedInToNotificationAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedPromotedContentAdIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedPromotedContentAdIfOptedInToBraveNewsAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedPromotedContentAdIfOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  const AdInfo ad = test::BuildAd(AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedPromotedContentAdIfOptedInToBraveNewsAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kPromotedContentAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest, ConvertViewedSearchResultAdIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNotificationAds();
  test::OptOutOfNewTabPageAds();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest, ConvertViewedSearchResultAdIfOptedInToAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedSearchResultAdIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNotificationAds();
  test::OptOutOfNewTabPageAds();

  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest, ConvertClickedSearchResultAdIfOptedInToAds) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kSearchResultAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotCapAdConversionsWithinDifferentCreativeSets) {
  // Arrange
  const AdInfo ad_1 = test::BuildAd(AdType::kInlineContentAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_1.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_1, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression});

  const ConversionInfo conversion_1 =
      BuildConversion(BuildAdEvent(ad_1, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  const AdInfo ad_2 = test::BuildAd(AdType::kSearchResultAd,
                                    /*should_generate_random_uuids=*/true);
  test::BuildAndSaveCreativeSetConversion(ad_2.creative_set_id,
                                          kAnotherMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  const ConversionInfo conversion_2 =
      BuildConversion(BuildAdEvent(ad_2, ConfirmationType::kClicked,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion_1));
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion_2));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest, DoNotCapAdConversionsWithinTheSameCreativeSet) {
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

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion)).Times(2);
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest, CapAdConversionsWithinTheSameCreativeSet) {
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

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion)).Times(2);
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
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

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
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
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoCreativeSetConversions) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
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
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
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

  const ConversionInfo conversion_1 =
      BuildConversion(BuildAdEvent(ad_1, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion_1));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);

  AdInfo ad_2 = ad_1;
  ad_2.creative_instance_id = "1e945c25-98a2-443c-a7f5-e695110d2b84";
  RecordAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServedImpression,
             ConfirmationType::kViewedImpression, ConfirmationType::kClicked});

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
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
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfCreativeSetConversionIsOnTheCuspOfExpiring) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  AdvanceClockBy(base::Days(3) - base::Milliseconds(1));

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheCreativeSetConversionHasExpired) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::RecordAdEvent(ad, ConfirmationType::kViewedImpression);

  test::BuildAndSaveCreativeSetConversion(ad.creative_set_id,
                                          kMatchingUrlPattern,
                                          /*observation_window=*/base::Days(3));

  AdvanceClockBy(base::Days(3));

  // Act & Assert
  conversions_->MaybeConvert(BuildRedirectChain(), kHtml);
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableAdvertiserPublicKeyIsEmpty) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      /*verifiable_advertiser_public_key_base64=*/"");

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml);
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfResourceIdPatternDoesNotMatchRedirectChain) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id,
      /*url_pattern=*/"https://www.baz.com/*",
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://grault.com/garply"),
                          GURL("https://www.baz.com/bar"),
                          GURL("https://qux.com/quux/plugh")},
      kHtml);
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableUrlConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux=quux")}, kHtml);
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableUrlConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/Now()),
      VerifiableConversionInfo{
          /*id=*/"xyzzy",
          test::kVerifiableConversionAdvertiserPublicKeyBase64});

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml);
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableHtmlConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://foo.com/bar")}, kHtml);
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableHtmlConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/Now()),
      VerifiableConversionInfo{
          /*id=*/"waldo",
          test::kVerifiableConversionAdvertiserPublicKeyBase64});

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      BuildRedirectChain(),
      /*html=*/R"(<html><div id="xyzzy-id">waldo</div></html>)");
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfVerifiableHtmlMetaTagConversionIdDoesNotExist) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion =
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                                   /*created_at=*/Now()),
                      /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")}, kHtml);
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfVerifiableHtmlMetaTagConversionIdExists) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);

  test::BuildAndSaveVerifiableCreativeSetConversion(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window=*/base::Days(3),
      test::kVerifiableConversionAdvertiserPublicKeyBase64);

  RecordAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServedImpression,
           ConfirmationType::kViewedImpression});

  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewedImpression,
                   /*created_at=*/Now()),
      VerifiableConversionInfo{
          /*id=*/"fred", test::kVerifiableConversionAdvertiserPublicKeyBase64});

  // Act & Assert
  EXPECT_CALL(conversions_observer_mock_, OnDidConvertAd(conversion));
  conversions_->MaybeConvert(
      /*redirect_chain=*/{GURL("https://qux.com/quux/corge")},
      /*html=*/R"(<html><meta name="ad-conversion-id" content="fred"></html>)");
}

}  // namespace brave_ads
