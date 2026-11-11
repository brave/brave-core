/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "url/gurl.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCampaignConversionUtilTest : public test::TestBase {};

TEST_F(BraveAdsCampaignConversionUtilTest, GetMatchingCampaignConversions) {
  // Arrange
  CampaignConversionList campaign_conversions;

  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://foo.com/*",
                                    /*observation_window=*/base::Days(3));
  campaign_conversions.push_back(campaign_conversion_1);

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://*.qux.com/fred",
                                    /*observation_window=*/base::Days(7));
  campaign_conversions.push_back(campaign_conversion_2);

  const CampaignConversionInfo campaign_conversion_3 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://bar.com/foo",
                                    /*observation_window=*/base::Days(30));
  campaign_conversions.push_back(campaign_conversion_3);

  const CampaignConversionInfo campaign_conversion_4 =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://baz.com/",
                                    /*observation_window=*/base::Days(1));
  campaign_conversions.push_back(campaign_conversion_4);

  const std::vector<GURL> redirect_chain = {
      GURL("https://baz.com/" /*matches campaign_conversion_4*/),
      GURL("https://foo.com/bar" /*matches campaign_conversion_1*/),
      GURL("https://www.qux.com/fred" /*matches campaign_conversion_2*/),
      GURL("https://quux.com/corge/grault"), GURL("https://garbly.com/waldo")};

  // Act
  const CampaignConversionList matching_campaign_conversions =
      GetMatchingCampaignConversions(campaign_conversions, redirect_chain);

  // Assert
  const CampaignConversionList expected_matching_campaign_conversions = {
      campaign_conversion_1, campaign_conversion_2, campaign_conversion_4};
  EXPECT_EQ(expected_matching_campaign_conversions,
            matching_campaign_conversions);
}

TEST_F(BraveAdsCampaignConversionUtilTest, GetCampaignConversionCounts) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/true);

  AdEventList ad_events;

  const AdEventInfo ad_event_1 =
      BuildAdEvent(ad, mojom::ConfirmationType::kServedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_1);

  AdEventInfo ad_event_2 =
      BuildAdEvent(ad, mojom::ConfirmationType::kConversion,
                   /*created_at=*/test::Now());
  ad_event_2.campaign_id = test::kAnotherCampaignId;
  ad_events.push_back(ad_event_2);

  const AdEventInfo ad_event_3 =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_3);

  const AdEventInfo ad_event_4 =
      BuildAdEvent(ad, mojom::ConfirmationType::kConversion,
                   /*created_at=*/test::Now());
  ad_events.push_back(ad_event_4);
  ad_events.push_back(ad_event_4);

  // Act
  const CampaignConversionCountMap campaign_conversion_counts =
      GetCampaignConversionCounts(ad_events);

  // Assert
  const CampaignConversionCountMap expected_campaign_conversion_counts = {
      {ad_event_2.campaign_id, 1}, {ad_event_4.campaign_id, 2}};
  EXPECT_EQ(expected_campaign_conversion_counts, campaign_conversion_counts);
}

TEST_F(BraveAdsCampaignConversionUtilTest, SortCampaignConversionsIntoBuckets) {
  // Arrange
  CampaignConversionList campaign_conversions;

  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(
          test::kCampaignId,
          /*url_pattern=*/"https://foo.com/*",
          /*observation_window=*/base::Days(3));  // Bucket #1
  campaign_conversions.push_back(campaign_conversion_1);

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(
          test::kAnotherCampaignId, /*url_pattern=*/"https://www.qux.com/",
          /*observation_window=*/base::Days(7));  // Bucket #2
  campaign_conversions.push_back(campaign_conversion_2);

  const CampaignConversionInfo campaign_conversion_3 =
      test::BuildCampaignConversion(
          test::kCampaignId,
          /*url_pattern=*/"https://baz.com/",
          /*observation_window=*/base::Days(30));  // Bucket #1
  campaign_conversions.push_back(campaign_conversion_3);

  // Act
  const CampaignConversionBucketMap campaign_conversion_buckets =
      SortCampaignConversionsIntoBuckets(campaign_conversions);

  // Assert
  CampaignConversionBucketMap expected_campaign_conversion_buckets;
  expected_campaign_conversion_buckets.insert(  // Bucket #1
      {test::kCampaignId, {campaign_conversion_1, campaign_conversion_3}});
  expected_campaign_conversion_buckets.insert(  // Bucket #2
      {campaign_conversion_2.id, {campaign_conversion_2}});
  EXPECT_EQ(expected_campaign_conversion_buckets, campaign_conversion_buckets);
}

TEST_F(BraveAdsCampaignConversionUtilTest,
       SortEmptyCampaignConversionsIntoBuckets) {
  // Act & Assert
  EXPECT_THAT(SortCampaignConversionsIntoBuckets(/*campaign_conversions=*/{}),
              ::testing::IsEmpty());
}

TEST_F(BraveAdsCampaignConversionUtilTest,
       FilterCampaignConversionBucketsThatExceedTheCap) {
  // Arrange
  CampaignConversionList campaign_conversions;

  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(
          test::kCampaignId,
          /*url_pattern=*/"https://foo.com/*",
          /*observation_window=*/base::Days(3));  // Bucket #1
  campaign_conversions.push_back(campaign_conversion_1);
  campaign_conversions.push_back(campaign_conversion_1);

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(
          test::kAnotherCampaignId, /*url_pattern=*/"https://baz.com/",
          /*observation_window=*/base::Days(30));  // Bucket #1
  campaign_conversions.push_back(campaign_conversion_2);
  campaign_conversions.push_back(campaign_conversion_2);
  campaign_conversions.push_back(campaign_conversion_2);

  const CampaignConversionCountMap campaign_conversion_counts = {
      {campaign_conversion_1.id, 2}, {campaign_conversion_2.id, 3}};

  CampaignConversionBucketMap campaign_conversion_buckets =
      SortCampaignConversionsIntoBuckets(campaign_conversions);

  // Act
  FilterCampaignConversionBucketsThatExceedTheCap(campaign_conversion_counts,
                                                  /*campaign_conversion_cap=*/2,
                                                  campaign_conversion_buckets);

  // Assert
  const CampaignConversionBucketMap expected_campaign_conversion_buckets = {
      {campaign_conversion_1.id,
       {campaign_conversion_1, campaign_conversion_1}}};
  EXPECT_EQ(expected_campaign_conversion_buckets, campaign_conversion_buckets);
}

TEST_F(BraveAdsCampaignConversionUtilTest,
       DoNotFilterCampaignConversionBucketsIfCapIsZero) {
  // Arrange
  CampaignConversionList campaign_conversions;

  const CampaignConversionInfo campaign_conversion_1 =
      test::BuildCampaignConversion(
          test::kCampaignId,
          /*url_pattern=*/"https://foo.com/*",
          /*observation_window=*/base::Days(3));  // Bucket #1
  campaign_conversions.push_back(campaign_conversion_1);
  campaign_conversions.push_back(campaign_conversion_1);

  const CampaignConversionInfo campaign_conversion_2 =
      test::BuildCampaignConversion(
          test::kAnotherCampaignId, /*url_pattern=*/"https://baz.com/",
          /*observation_window=*/base::Days(30));  // Bucket #2
  campaign_conversions.push_back(campaign_conversion_2);
  campaign_conversions.push_back(campaign_conversion_2);
  campaign_conversions.push_back(campaign_conversion_2);

  const CampaignConversionCountMap campaign_conversion_counts = {
      {campaign_conversion_1.id, 2}, {campaign_conversion_2.id, 3}};

  CampaignConversionBucketMap campaign_conversion_buckets =
      SortCampaignConversionsIntoBuckets(campaign_conversions);

  // Act
  FilterCampaignConversionBucketsThatExceedTheCap(campaign_conversion_counts,
                                                  /*campaign_conversion_cap=*/0,
                                                  campaign_conversion_buckets);

  // Assert
  const CampaignConversionBucketMap expected_campaign_conversion_buckets = {
      {campaign_conversion_1.id,
       {campaign_conversion_1, campaign_conversion_1}},
      {campaign_conversion_2.id,
       {campaign_conversion_2, campaign_conversion_2, campaign_conversion_2}}};
  EXPECT_EQ(expected_campaign_conversion_buckets, campaign_conversion_buckets);
}

TEST_F(BraveAdsCampaignConversionUtilTest,
       HasCampaignConversionWithinObservationWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kConversion,
                   /*created_at=*/test::Now());

  AdvanceClockBy(base::Days(3) + base::Milliseconds(1));

  const CampaignConversionList campaign_conversions = {
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"https://foo.com/*",
                                    /*observation_window=*/base::Days(7))};

  // Act & Assert
  EXPECT_TRUE(HasCampaignConversionWithinObservationWindow(campaign_conversions,
                                                           ad_event));
}

TEST_F(BraveAdsCampaignConversionUtilTest,
       DoesNotHaveCampaignConversionWithinObservationWindow) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kConversion,
                   /*created_at=*/test::Now());

  AdvanceClockBy(base::Days(3) + base::Milliseconds(1));

  const CampaignConversionList campaign_conversions = {
      test::BuildCampaignConversion(test::kAnotherCampaignId,
                                    /*url_pattern=*/"https://www.qux.com/",
                                    /*observation_window=*/base::Days(3))};

  // Act & Assert
  EXPECT_FALSE(HasCampaignConversionWithinObservationWindow(
      campaign_conversions, ad_event));
}

}  // namespace brave_ads
