/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_matcher.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/test/order_extraction_patterns_ad_event_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     MatchCampaignConversionRequestingOrderValueExtraction) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;

  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id, /*created_at=*/base::Time::Now())};

  // Act
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, ad_events,
          /*redirect_chain=*/{GURL("https://brave.com/thank-you")});

  // Assert
  ASSERT_TRUE(matched_campaign_conversion);
  EXPECT_EQ(campaign_conversion, *matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     MatchCampaignConversionOnCheckoutUrlPattern) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;
  campaign_conversion.checkout_url_pattern = "https://brave.com/checkout";

  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id, /*created_at=*/base::Time::Now())};

  // Act, the tab is still on the checkout page, not yet the thank you page.
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, ad_events,
          /*redirect_chain=*/{GURL("https://brave.com/checkout")});

  // Assert
  EXPECT_TRUE(matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     DoNotMatchCampaignConversionNotRequestingOrderExtraction) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(
          /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
          /*thank_you_url_pattern=*/"https://brave.com/thank-you",
          /*observation_window=*/base::Days(1));

  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id, /*created_at=*/base::Time::Now())};

  // Act, extract_id/extract_value are both unset.
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, ad_events,
          /*redirect_chain=*/{GURL("https://brave.com/thank-you")});

  // Assert
  EXPECT_FALSE(matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     DoNotMatchCampaignConversionWithNoPendingAdEvent) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;

  // Act, no ad event for this campaign at all.
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, /*ad_events=*/{},
          /*redirect_chain=*/{GURL("https://brave.com/thank-you")});

  // Assert
  EXPECT_FALSE(matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     DoNotMatchCampaignConversionWithAdEventOutsideObservationWindow) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;

  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id,
      /*created_at=*/base::Time::Now() - base::Days(2))};

  // Act
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, ad_events,
          /*redirect_chain=*/{GURL("https://brave.com/thank-you")});

  // Assert
  EXPECT_FALSE(matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     DoNotMatchCampaignConversionWithMismatchingRedirectChain) {
  // Arrange
  CampaignConversionInfo campaign_conversion = test::BuildCampaignConversion(
      /*campaign_id=*/"6ee347cb-6b51-4e44-a0e5-c8fc21bca9bf",
      /*thank_you_url_pattern=*/"https://brave.com/thank-you",
      /*observation_window=*/base::Days(1));
  campaign_conversion.extract_value = true;

  const AdEventList ad_events = {test::BuildAdEventForCampaignId(
      campaign_conversion.id, /*created_at=*/base::Time::Now())};

  // Act
  const std::optional<CampaignConversionInfo> matched_campaign_conversion =
      MaybeGetCampaignConversionForOrderExtraction(
          {campaign_conversion}, ad_events,
          /*redirect_chain=*/{GURL("https://brave.com/cart")});

  // Assert
  EXPECT_FALSE(matched_campaign_conversion);
}

TEST(BraveAdsOrderExtractionPatternsMatcherTest,
     DoNotMatchAnyCampaignConversionForEmptyList) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetCampaignConversionForOrderExtraction(
      /*campaign_conversions=*/{}, /*ad_events=*/{},
      /*redirect_chain=*/{GURL("https://brave.com/thank-you")}));
}

}  // namespace brave_ads
