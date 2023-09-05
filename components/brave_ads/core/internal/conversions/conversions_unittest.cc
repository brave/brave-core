/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_interaction/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/public/ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
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

class BraveAdsConversionsTest : public ConversionsObserver,
                                public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    conversions_ = std::make_unique<Conversions>();
    conversions_->AddObserver(this);
  }

  void TearDown() override {
    conversions_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnDidConvertAd(const ConversionInfo& conversion) override {
    actioned_conversions_.push_back(conversion);
  }

  void LoadConversionResource() {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
    task_environment_.RunUntilIdle();
  }

  void FireAdEventsAdvancingTheClockAfterEach(
      const AdInfo& ad,
      const std::vector<ConfirmationType>& confirmation_types) {
    for (const auto& confirmation_type : confirmation_types) {
      const AdEventInfo ad_event =
          BuildAdEvent(ad, confirmation_type, /*created_at*/ Now());
      FireAdEventForTesting(ad_event);

      AdvanceClockBy(base::Milliseconds(1));
    }
  }

  void DrainConversionQueue() {
    // Conversions are added to the |ConversionQueue|, so if the conversion
    // queue has pending conversions, we must force the processing of those
    // conversions to notify this observer.
    while (HasPendingTasks()) {
      FastForwardClockToNextPendingTask();
    }
  }

  void MaybeConvert(const std::vector<GURL>& redirect_chain,
                    const std::string& html) {
    conversions_->MaybeConvert(redirect_chain, html);

    DrainConversionQueue();
  }

  std::unique_ptr<Conversions> conversions_;

  ConversionList actioned_conversions_;
};

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedInlineContentAdIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedInlineContentAdIfBraveNewsAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedInlineContentAdIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedInlineContentAdIfBraveNewsAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kInlineContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedNewTabPageAdIfNewTabPageAdsAreDisabled) {
  // Arrange
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedNewTabPageAdIfNewTabPageAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedNewTabPageAdIfNewTabPageAdsAreDisabled) {
  // Arrange
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedNewTabPageAdIfNewTabPageAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNewTabPageAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedNotificationAdIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedNotificationAdIfOptedInToNotificationAds) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedNotificationAdIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedNotificationAdIfOptedInToNotificationAds) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertViewedPromotedContentAdIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertViewedPromotedContentAdIfBraveNewsAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertClickedPromotedContentAdIfBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertClickedPromotedContentAdIfBraveNewsAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kPromotedContentAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertViewedSearchResultAdIfAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableNotificationAdsForTesting();
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertViewedSearchResultAdIfAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertClickedSearchResultAdIfAdsAreDisabled) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableNotificationAdsForTesting();
  DisableNewTabPageAdsForTesting();

  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertClickedSearchResultAdIfAdsAreEnabled) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kSearchResultAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, MultipleAdConversions) {
  // Arrange
  const AdInfo ad_1 = BuildAdForTesting(AdType::kInlineContentAd,
                                        /*should_use_random_uuids*/ true);
  BuildAndSaveCreativeSetConversionForTesting(
      ad_1.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));
  FireAdEventsAdvancingTheClockAfterEach(
      ad_1, {ConfirmationType::kServed, ConfirmationType::kViewed});

  const AdInfo ad_2 = BuildAdForTesting(AdType::kSearchResultAd,
                                        /*should_use_random_uuids*/ true);
  BuildAndSaveCreativeSetConversionForTesting(
      ad_2.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window*/ base::Days(3));
  FireAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServed, ConfirmationType::kViewed,
             ConfirmationType::kClicked});

  const AdInfo ad_3 = BuildAdForTesting(AdType::kNewTabPageAd,
                                        /*should_use_random_uuids*/ true);
  FireAdEventsAdvancingTheClockAfterEach(
      ad_3, {ConfirmationType::kServed, ConfirmationType::kViewed,
             ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad_1, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad_2, ConfirmationType::kClicked,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));

  EXPECT_TRUE(
      ContainersEq(expected_actioned_conversions, actioned_conversions_));
}

TEST_F(BraveAdsConversionsTest, ConvertViewedAdAfterTheSameAdWasDismissed) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kDismissed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdsIfTheRedirectChainIsEmpty) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kDismissed, ConfirmationType::kServed,
           ConfirmationType::kTransferred, ConfirmationType::kFlagged,
           ConfirmationType::kSaved, ConfirmationType::kUpvoted,
           ConfirmationType::kDownvoted, ConfirmationType::kConversion});

  // Act
  MaybeConvert(/*redirect_chain*/ {}, kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdsIfTheRedirectChainContainsAnUnsupportedUrl) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kDismissed, ConfirmationType::kServed,
           ConfirmationType::kTransferred, ConfirmationType::kFlagged,
           ConfirmationType::kSaved, ConfirmationType::kUpvoted,
           ConfirmationType::kDownvoted, ConfirmationType::kConversion});

  // Act
  MaybeConvert(/*redirect_chain*/ {GURL("foo.bar")}, kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest, DoNotConvertNonViewedOrClickedAds) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kDismissed, ConfirmationType::kServed,
           ConfirmationType::kTransferred, ConfirmationType::kFlagged,
           ConfirmationType::kSaved, ConfirmationType::kUpvoted,
           ConfirmationType::kDownvoted, ConfirmationType::kConversion});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfThereIsNoMatchingCreativeSetConversion) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfAnotherAdHasConvertedWithinTheSameCreativeSet) {
  // Arrange
  const AdInfo ad_1 = BuildAdForTesting(AdType::kNotificationAd,
                                        /*should_use_random_uuids*/ true);
  BuildAndSaveCreativeSetConversionForTesting(
      ad_1.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));
  FireAdEventsAdvancingTheClockAfterEach(
      ad_1, {ConfirmationType::kServed, ConfirmationType::kViewed,
             ConfirmationType::kDismissed});

  MaybeConvert(BuildRedirectChain(), kHtml);

  AdInfo ad_2 = ad_1;
  ad_2.creative_instance_id = "1e945c25-98a2-443c-a7f5-e695110d2b84";
  FireAdEventsAdvancingTheClockAfterEach(
      ad_2, {ConfirmationType::kServed, ConfirmationType::kViewed,
             ConfirmationType::kClicked});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad_1, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, DoNotConvertAdIfUrlPatternDoesNotMatch) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kNonMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed,
           ConfirmationType::kDismissed});

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfCreativeSetConversionIsOnTheCuspOfExpiring) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());
  FireAdEventForTesting(ad_event);

  AdvanceClockBy(base::Days(3) - base::Milliseconds(1));

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       DoNotConvertAdIfTheCreativeSetConversionHasExpired) {
  // Arrange
  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3));

  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now());
  FireAdEventForTesting(ad_event);

  AdvanceClockBy(base::Days(3));

  // Act
  MaybeConvert(BuildRedirectChain(), kHtml);

  // Assert
  EXPECT_TRUE(actioned_conversions_.empty());
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableAdvertiserPublicKeyIsEmpty) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kEmptyVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfResourceIdPatternDoesNotMatchRedirectChain) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id,
      /*url_pattern*/ "https://www.baz.com/*",
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://grault.com/garply"),
                          GURL("https://www.baz.com/bar"),
                          GURL("https://qux.com/quux/plugh")},
      kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableUrlConversionIdDoesNotExist) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://foo.com/bar?qux=quux")}, kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableUrlConversionIdExists) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{/*id*/ "xyzzy",
                               kVerifiableConversionAdvertiserPublicKey}));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       FallbackToDefaultConversionIfVerifiableHtmlConversionIdDoesNotExist) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://foo.com/bar")}, kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest, ConvertAdIfVerifiableHtmlConversionIdExists) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(BuildRedirectChain(),
               /*html*/ R"(<html><div id="xyzzy-id">waldo</div></html>)");

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{/*id*/ "waldo",
                               kVerifiableConversionAdvertiserPublicKey}));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(
    BraveAdsConversionsTest,
    FallbackToDefaultConversionIfVerifiableHtmlMetaTagConversionIdDoesNotExist) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://qux.com/quux/corge")}, kHtml);

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(
      BuildConversion(BuildAdEvent(ad, ConfirmationType::kViewed,
                                   /*created_at*/ Now()),
                      /*verifiable_conversion*/ absl::nullopt));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

TEST_F(BraveAdsConversionsTest,
       ConvertAdIfVerifiableHtmlMetaTagConversionIdExists) {
  // Arrange
  LoadConversionResource();

  const AdInfo ad = BuildAdForTesting(AdType::kNotificationAd,
                                      /*should_use_random_uuids*/ true);

  BuildAndSaveVerifiableCreativeSetConversionForTesting(
      ad.creative_set_id, kAnotherMatchingUrlPattern,
      /*observation_window*/ base::Days(3),
      kVerifiableConversionAdvertiserPublicKey);

  FireAdEventsAdvancingTheClockAfterEach(
      ad, {ConfirmationType::kServed, ConfirmationType::kViewed});

  // Act
  MaybeConvert(
      /*redirect_chain*/ {GURL("https://qux.com/quux/corge")},
      /*html*/ R"(<html><meta name="ad-conversion-id" content="fred"></html>)");

  // Assert
  ConversionList expected_actioned_conversions;
  expected_actioned_conversions.push_back(BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed,
                   /*created_at*/ Now()),
      VerifiableConversionInfo{/*id*/ "fred",
                               kVerifiableConversionAdvertiserPublicKey}));
  EXPECT_EQ(expected_actioned_conversions, actioned_conversions_);
}

}  // namespace brave_ads
