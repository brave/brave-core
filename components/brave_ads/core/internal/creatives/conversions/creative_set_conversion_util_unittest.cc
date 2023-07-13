/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_unittest_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeSetConversionUtilTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeSetConversionUtilTest,
       FilterConvertedAndNonMatchingCreativeSetConversions) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ true);

  AdEventList ad_events;
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kConversion, /*created_at*/ Now());
  ad_events.push_back(ad_event);

  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      BuildCreativeSetConversion(kCreativeSetId,
                                 /*url_pattern*/ "https://foo.com/*",
                                 /*observation_window*/ base::Days(3));
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      BuildCreativeSetConversion(ad_event.creative_set_id,
                                 /*url_pattern*/ "https://www.qux.com/",
                                 /*observation_window*/ base::Days(7));
  creative_set_conversions.push_back(creative_set_conversion_2);

  const CreativeSetConversionInfo creative_set_conversion_3 =
      BuildCreativeSetConversion(kCreativeSetId,
                                 /*url_pattern*/ "https://bar.com/foo",
                                 /*observation_window*/ base::Days(30));
  creative_set_conversions.push_back(creative_set_conversion_3);

  const CreativeSetConversionInfo creative_set_conversion_4 =
      BuildCreativeSetConversion(kCreativeSetId,
                                 /*url_pattern*/ "https://baz.com/",
                                 /*observation_window*/ base::Days(1));
  creative_set_conversions.push_back(creative_set_conversion_4);

  const std::vector<GURL> redirect_chain = {
      GURL("https://baz.com/"), GURL("https://foo.com/bar"),
      GURL("https://www.qux.com/"), GURL("https://quux.com/corge/grault"),
      GURL("https://garbly.com/waldo")};

  // Act

  // Assert
  const CreativeSetConversionList expected_creative_set_conversions = {
      creative_set_conversion_1, creative_set_conversion_4};
  EXPECT_EQ(expected_creative_set_conversions,
            FilterConvertedAndNonMatchingCreativeSetConversions(
                creative_set_conversions, ad_events, redirect_chain));
}

TEST_F(BraveAdsCreativeSetConversionUtilTest,
       SortCreativeSetConversionsIntoBuckets) {
  // Arrange
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      BuildCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern*/ "https://foo.com/*",
          /*observation_window*/ base::Days(3));  // Bucket #1
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      BuildCreativeSetConversion(
          /*creative_set_id*/ "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8",
          /*url_pattern*/ "https://www.qux.com/",
          /*observation_window*/ base::Days(7));  // Bucket #2
  creative_set_conversions.push_back(creative_set_conversion_2);

  const CreativeSetConversionInfo creative_set_conversion_3 =
      BuildCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern*/ "https://baz.com/",
          /*observation_window*/ base::Days(30));  // Bucket #1
  creative_set_conversions.push_back(creative_set_conversion_3);

  // Act
  const CreativeSetConversionBuckets buckets =
      SortCreativeSetConversionsIntoBuckets(creative_set_conversions);

  // Assert
  CreativeSetConversionBuckets expected_buckets;
  expected_buckets.insert(  // Bucket #1
      {kCreativeSetId, {creative_set_conversion_1, creative_set_conversion_3}});
  expected_buckets.insert(  // Bucket #2
      {creative_set_conversion_2.id, {creative_set_conversion_2}});
  EXPECT_EQ(expected_buckets, buckets);
}

TEST_F(BraveAdsCreativeSetConversionUtilTest,
       SortEmptyCreativeSetConversionsIntoBuckets) {
  // Arrange

  // Act
  const CreativeSetConversionBuckets buckets =
      SortCreativeSetConversionsIntoBuckets({});

  // Assert
  EXPECT_TRUE(buckets.empty());
}

TEST_F(BraveAdsCreativeSetConversionUtilTest,
       FindNonExpiredCreativeSetConversion) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kConversion, /*created_at*/ Now());

  AdvanceClockBy(base::Days(3) + base::Milliseconds(1));

  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion_1 =
      BuildVerifiableCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern*/ "https://foo.com/*",
          /*observation_window*/ base::Days(7),
          /*should_extract_verifiable_id*/ true,
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions.push_back(creative_set_conversion_1);

  const CreativeSetConversionInfo creative_set_conversion_2 =
      BuildCreativeSetConversion(
          /*creative_set_id*/ "4e83a23c-1194-40f8-8fdc-2f38d7ed75c8",
          /*url_pattern*/ "https://www.qux.com/",
          /*observation_window*/ base::Days(3));
  creative_set_conversions.push_back(creative_set_conversion_2);

  // Act

  // Assert
  EXPECT_EQ(creative_set_conversion_1, FindNonExpiredCreativeSetConversion(
                                           creative_set_conversions, ad_event));
}

TEST_F(BraveAdsCreativeSetConversionUtilTest,
       DoNotFindNonExpiredCreativeSetConversion) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kConversion, /*created_at*/ Now());

  AdvanceClockBy(base::Days(7) + base::Milliseconds(1));

  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion =
      BuildVerifiableCreativeSetConversion(
          kCreativeSetId,
          /*url_pattern*/ "https://foo.com/*",
          /*observation_window*/ base::Days(7),
          /*should_extract_verifiable_id*/ true,
          kVerifiableConversionAdvertiserPublicKey);
  creative_set_conversions.push_back(creative_set_conversion);

  // Act

  // Assert
  EXPECT_FALSE(
      FindNonExpiredCreativeSetConversion(creative_set_conversions, ad_event));
}

}  // namespace brave_ads
