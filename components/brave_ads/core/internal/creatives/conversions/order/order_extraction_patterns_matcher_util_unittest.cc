/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_matcher_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/test/order_extraction_patterns_ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest, HasPendingAdEvent) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));
  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id, /*created_at=*/base::Time::Now())};

  // Act & Assert
  EXPECT_TRUE(HasPendingAdEvent(campaign_conversion, ad_events));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     DoNotHavePendingAdEventForMismatchingCampaignId) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));
  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      /*campaign_id=*/"a-different-campaign-id",
      /*created_at=*/base::Time::Now())};

  // Act & Assert
  EXPECT_FALSE(HasPendingAdEvent(campaign_conversion, ad_events));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     DoNotHavePendingAdEventOutsideObservationWindow) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));
  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id,
      /*created_at=*/base::Time::Now() - base::Days(2))};

  // Act & Assert
  EXPECT_FALSE(HasPendingAdEvent(campaign_conversion, ad_events));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     DoNotHavePendingAdEventForEmptyAdEvents) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));

  // Act & Assert
  EXPECT_FALSE(HasPendingAdEvent(campaign_conversion,
                                 /*ad_events=*/{}));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     IsOrderExtractionEnabledForId) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_id = true;

  // Act & Assert
  EXPECT_TRUE(IsOrderExtractionEnabled(campaign_conversion));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     IsOrderExtractionEnabledForValue) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;

  // Act & Assert
  EXPECT_TRUE(IsOrderExtractionEnabled(campaign_conversion));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     DoNotHaveOrderExtractionEnabledWhenNeitherIdNorValueSet) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));

  // Act & Assert
  EXPECT_FALSE(IsOrderExtractionEnabled(campaign_conversion));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     MatchRedirectChainOnThankYouUrlPattern) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));

  // Act & Assert
  EXPECT_TRUE(MatchesRedirectChain(
      campaign_conversion,
      /*redirect_chain=*/{GURL("https://brave.com/thank-you")}));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     MatchRedirectChainOnCheckoutUrlPattern) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.checkout_url_pattern = "https://brave.com/checkout";

  // Act & Assert
  EXPECT_TRUE(MatchesRedirectChain(
      campaign_conversion,
      /*redirect_chain=*/{GURL("https://brave.com/checkout")}));
}

TEST(BraveAdsOrderExtractionPatternsMatcherUtilTest,
     DoNotMatchRedirectChainForUnrelatedUrl) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));

  // Act & Assert
  EXPECT_FALSE(MatchesRedirectChain(
      campaign_conversion,
      /*redirect_chain=*/{GURL("https://brave.com/cart")}));
}

}  // namespace brave_ads
