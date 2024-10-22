/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/conversion_exclusion_rule.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/exclusion_rule_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionExclusionRuleTest : public test::TestBase {};

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldIncludeIfThereIsNoConversionHistory) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  const ConversionExclusionRule exclusion_rule(/*ad_events=*/{});

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldExcludeIfSameCreativeSetHasAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/false);
  ad_events.push_back(ad_event);

  const ConversionExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldAlwaysIncludeIfConversionCapIsSetToZero) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_creative_set_exceeds_conversion_cap", "0"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/false);
  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);
  ad_events.push_back(ad_event);

  const ConversionExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldIncludeIfNotExceededConversionCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_creative_set_exceeds_conversion_cap", "7"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/false);
  for (int i = 0;
       i < kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get() - 1; ++i) {
    ad_events.push_back(ad_event);
  }

  const ConversionExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldExcludeIfExceededConversionCap) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kExclusionRulesFeature,
      {{"should_exclude_ad_if_creative_set_exceeds_conversion_cap", "7"}});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/false);
  for (int i = 0; i < kShouldExcludeAdIfCreativeSetExceedsConversionCap.Get();
       ++i) {
    ad_events.push_back(ad_event);
  }

  const ConversionExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsConversionExclusionRuleTest,
       ShouldIncludeIfCreativeSetHasNotAlreadyConverted) {
  // Arrange
  CreativeAdInfo creative_ad_1;
  creative_ad_1.creative_set_id = test::kCreativeSetId;

  CreativeAdInfo creative_ad_2;
  creative_ad_2.creative_set_id = test::kAnotherCreativeSetId;

  AdEventList ad_events;
  const AdEventInfo ad_event = test::BuildAdEvent(
      creative_ad_2, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kConversion,
      /*created_at=*/test::Now(), /*should_generate_random_uuids=*/false);
  ad_events.push_back(ad_event);

  const ConversionExclusionRule exclusion_rule(ad_events);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad_1).has_value());
}

}  // namespace brave_ads
